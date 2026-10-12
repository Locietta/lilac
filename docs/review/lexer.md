# Lexer findings

Temporary working notes from the 2026-10-12 review of lighter 0.3.1 (`584438d`),
made before the lexer moved here, extended by the test audit of lilac 0.1.0.
Line numbers may drift. Delete this file
once the issues are fixed or tracked elsewhere. IDs are stable; the
verification levels mean: `verified` (traced by hand), `reproduced` (a repro
program was run), `reported` (a reviewer's reading, not re-checked).

Scope: `lilac/**`.

Architecture as reviewed: each language is a value type (`CppLexer{.dialect = ...}`)
satisfying `LexerImplementation`, wrapped in a `pro::proxy<LexerFacade>`.
`registry.cpp` is an explicit constexpr table of fence names → factory.
Reflection is used in `role.h` to build a compile-time Style → TokenRole table
from `[[=token_role(...)]]` annotations, with consteval validation (unique IDs,
exactly one role, style 0 is DEFAULT). `Document` keeps one `u32` state per
line and supports streamed `append()`, re-lexing from the last unfinished line.
Ported from Notepad4/Lexilla: `THIRD_PARTY_NOTICES.md` lists the sources.

None of the bugs below read out of bounds; LX-5 aborts under enforced
contracts and LX-9 hangs. The rest produce wrong colours.

## Bugs

### LX-1 Python raw strings end at `\"`

- Severity: medium · Verification: verified
- Where: `lilac/language/python.cpp:133-144` (`string_end` skips escape
  handling entirely when `raw`)
- Problem: in Python a backslash still prevents the quote from closing a raw
  string: `r"a\"b"` is one string. The lexer closes it at `\"`.
- Test: `x = r"a\"b" + c\n`: `r"a\"b"` STRING, `+` OPERATOR, `c` IDENTIFIER.
- Fix: when `raw`, skip the character after a backslash without decoding it.

### LX-2 Python octal escapes consume hex digits and quotes

- Severity: medium · Verification: verified
- Where: `lilac/language/python.cpp:126-129`
- Problem: for an octal escape the loop accepts `ascii_hex_digit(c)` or any
  byte `<= '7'` (no lower bound), so `\18` / `\1a` consume `8` / `a`, and `"\0"`
  swallows its closing quote. Inside `"""…"""` two closing quotes are eaten and
  the string runs on into later lines.
- Tests: `s = "\18"\n` (`8` STRING); `s = "\0" + t\n` (`+` OPERATOR);
  `s = """a\0"""\nx = 1\n` (line 2 `x` IDENTIFIER, `1` NUMBER).
- Fix: accept only `'0'..'7'`, at most three digits.

### LX-3 C++ raw string: newline on the opening line painted DEFAULT

- Severity: low · Verification: verified (test audit)
- Where: `lilac/language/cpp.cpp:378, 441-447`
- Problem: `state` holds `k_raw_string | opener`, but line 444 compares
  `state == k_raw_string`. Passes by accident when `R"` is at offset 0.
- Test: `auto text = R"tag(first\nsecond)tag"\n`: `first\n` STRING.
- Same family (low): assembly, build_script, functional, scripting, legacy and
  document_markup return early on an unterminated comment/string without
  painting `[line_end, end)` (e.g. `assembly.cpp:292`, `functional.cpp:254`,
  `scripting.cpp:449`).

### LX-4 Markup issues

- Severity: low · Verification: verified (test audit)
- Where: `lilac/language/markup.cpp:94-98, 131-136, 182, 243-255`
  - LX-4a: an attribute value spanning lines loses its quote state and the tag
    never closes: `<a title="x\ny">text</a>\n<b>\n` paints `b` as PROPERTY.
  - LX-4b (XML only): entities are painted inside comments and CDATA:
    `<root><!-- a &amp; b --><![CDATA[x &lt; y]]></root>`.
  - LX-4c: multi-line `<!-- -->` in Markdown is ignored; the next line's `#`
    becomes a heading.

### LX-5 `append(document, "")` aborts every lexer under enforced contracts

- Severity: medium · Verification: verified (test audit)
- Where: `lilac/document.cpp:28-41` (`append`), each lexer's
  `contract_assert(context.line_starts[line] == context.range.begin)` (e.g. `cpp.cpp:472`)
- Problem: an empty append on a non-empty last line returns `{size, size}`,
  which is not a line start. Streaming consumers can easily pass empty chunks.
- Fix: return `{line_starts.back(), size}`, or make lexers return early on an
  empty range.

### LX-6 SQL string opened at end of line counts as closed

- Severity: medium · Verification: verified (test audit)
- Where: `lilac/language/sql.cpp:188`
- Problem: the closing check `source[position - 1] != '\''` is satisfied by the
  opening quote itself. `SELECT '\nmulti' FROM t;\nSELECT 1;\n` inverts strings
  for the rest of the document.

### LX-7 D: `/* */` treated as nesting

- Severity: low · Verification: verified (test audit)
- Where: `lilac/language/brace.cpp:138`
- Problem: D nests only `/+ +/` (unsupported); `/* a /* b */ auto x = 1;` stays
  a comment to EOF.

### LX-8 YAML: `|` or `>` inside a plain value starts a block scalar

- Severity: medium · Verification: verified (test audit)
- Where: `lilac/language/structured_data.cpp:312-316`
- Problem: `- run: echo a | grep b` (common in GitHub Actions) or
  `when: x > 1` turns the following lines into STRING.
- Fix: only treat `|` / `>` as a block indicator when it is the whole value
  (optionally followed by indentation/chomping indicators and a comment).

### LX-9 Erlang/Elixir loop forever on a foreign line state

- Severity: medium · Verification: verified (test audit)
- Where: `lilac/language/functional.cpp:186-203` (`continue_comment`)
- Problem: Erlang/Elixir use empty block-comment delimiters, and
  `starts_with("")` is always true, so a line state with the `0x1000'0000` bits
  (state 1 in most other lexers) makes `position += 0` loop. Happens when a
  `Document` is reused with a different lexer without `assign`.
- Fix: decide the contract (line states must come from the same lexer: document
  it as a precondition) and guard the empty-delimiter case anyway.

### Low-severity

- `/*/ c */ x` closes the comment at `/*/` in Go, JS, CSS, JSON (`go.cpp:216`,
  `javascript.cpp:415`, `css.cpp:121`, `structured_data.cpp:138`): `/*` sets the
  state without advancing, then `find("*/", position)` matches at +1.
- LaTeX `\begin{verbatim}x\end{verbatim}` on one line never closes
  (`document_markup.cpp:202-205`).

## Design

- **LX-D1 The driver loop is duplicated** across the 17 language files.
- **LX-D2 `Scanner` (`lilac/scanner.h`) is unused by the lexers.** Verified: only
  `test-core` uses `struct Scanner`; the language files include `scanner.h` for
  `paint()`. Either adopt it in the language files (with LX-D1) or drop it.
- **LX-D3 Append-only documents**: no mid-document edit, and no early stop once
  line states converge after an edit. Fine for streaming output; a limit for
  an editor.
- **LX-D4 MinGW consumers must define `handle_contract_violation`.** libstdc++exp's
  default is weak and does not resolve from a static archive. lilac leaves it to the
  application (lighter defines one in `utils/panic.cpp`); the tests get theirs from
  `lilac/tests/support/contract_handler.cpp`. Documented in the README.
- **LX-D5 Quadratic scans on large unterminated constructs.** Many lexers call
  `source.find(close, position)` without clipping to the line, so each
  continuation line of a huge unterminated comment/string scans to the end of
  input: O(lines × size). Affects cpp, go, javascript, css, json, toml, markup,
  sql, legacy, build_script, assembly, scripting, brace, functional,
  document_markup. Fix: search a view clipped to `line_end`.
- **LX-D6 The registry cannot be enumerated.** Tests that cover all lexers need a
  list; add `std::span<const std::string_view> language_names()` (one
  resolvable alias per configuration), or share the list from `test-registry`.
- **LX-D7 Several lexers rely on `append` zeroing the dirty range** (build_script,
  assembly, functional, scripting, legacy, document_markup never paint DEFAULT
  over the line first). Document it, or paint DEFAULT up front.
- **LX-D8 Usage rule to document:** the range returned by each `append` must be
  lexed before the next `append`.
