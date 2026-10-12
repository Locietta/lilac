# Lexer findings

Temporary working notes from the 2026-10-12 review of lighter 0.3.1 (`584438d`),
made before the lexer moved here. Line numbers refer to the code as it was in
lighter's `lighter/lexer/` and may have drifted slightly. Delete this file once
the issues are fixed or tracked elsewhere. IDs are stable; the verification
levels mean: `verified` (traced by hand), `reproduced` (a repro program was run),
`reported` (a reviewer's reading, not re-checked).

Scope: `lilac/**`.

Architecture as reviewed: each language is a value type (`CppLexer{.dialect = ...}`)
satisfying `LexerImplementation`, wrapped in a `pro::proxy<LexerFacade>`.
`registry.cpp` is an explicit constexpr table of fence names → factory.
Reflection is used in `role.h` to build a compile-time Style → TokenRole table
from `[[=token_role(...)]]` annotations, with consteval validation (unique IDs,
exactly one role, style 0 is DEFAULT). `Document` keeps one `u32` state per
line and supports streamed `append()`, re-lexing from the last unfinished line.
Ported from Notepad4/Lexilla: `THIRD_PARTY_NOTICES.md` lists the sources.

## Bugs

### LX-1 Python raw strings end at `\"`

- Severity: medium · Verification: verified
- Where: `lilac/language/python.cpp:133-144` (`string_end` skips escape
  handling entirely when `raw`)
- Problem: in Python a backslash still prevents the quote from closing a raw
  string: `r"a\"b"` is one string. The lexer closes it at `\"`.
- Fix: when `raw`, skip the character after a backslash without decoding it.

### LX-2 Python octal escapes accept hex digits

- Severity: low · Verification: verified
- Where: `lilac/language/python.cpp:126-129`
- Problem: for octal `kind`, the loop condition is `ascii_hex_digit(c) || (...)`,
  so `\18` or `\1a` consume hex digits.

### LX-3 C++ raw string: newline on the opening line painted DEFAULT

- Severity: low · Verification: reported
- Where: `lilac/language/cpp.cpp:441-447`
- Problem: `state == k_raw_string` is compared with the payload bits still set.

### LX-4 Markup issues

- Severity: low · Verification: reported (not re-read)
- Where: `lilac/language/markup.cpp:96, 131, 182, 243, 255`
  - an attribute value spanning lines loses its quote state and the tag never closes;
  - entities are painted inside comments and CDATA;
  - multi-line `<!-- -->` in Markdown is ignored.

None of these hang or read out of bounds; they only produce wrong colours.

## Design

- **LX-D1 The driver loop is duplicated** across the 17 language files.
- **LX-D2 `Scanner` (`lilac/scanner.h`) is unused by the lexers.** Verified: only
  `test-core` uses `struct Scanner`; the language files include `scanner.h` for
  `paint()`. Either adopt it in the language files (with LX-D1) or drop it.
- **LX-D4 MinGW consumers must define `handle_contract_violation`.** libstdc++exp's
  default is weak and does not resolve from a static archive. lilac leaves it to the
  application (lighter defines one in `utils/panic.cpp`); the tests get theirs from
  `lilac/tests/support/contract_handler.cpp`. Documented in the README.
- **LX-D3 Append-only documents**: no mid-document edit, and no early stop once
  line states converge after an edit. Fine for streaming output; a limit for
  an editor.

## Missing tests

- empty input, a lone `\r`, bytes >= 0x80, unterminated constructs at EOF;
- chunked `append()` vs whole-document `assign()` gives identical styles and
  line states (the core promise of streaming), ideally fuzzed over split points;
- per-language regression cases for LX-1..LX-4.
