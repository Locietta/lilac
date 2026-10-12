# Test audit (lilac 0.1.0, 2026-10-12)

Each test function was judged on whether it would fail for a plausible real
regression. Temporary working notes; bug IDs refer to [lexer.md](lexer.md).

## Verdicts

58 test functions in 19 targets: 56 KEEP, 2 TRIM, 0 MERGE, 0 REMOVE.

| test                                         | verdict        | action                                                                                  |
| -------------------------------------------- | -------------- | --------------------------------------------------------------------------------------- |
| `test-legacy` `test_all_profiles`            | TRIM (applied) | dropped the `language_info().id.empty()` check; `test-registry` checks every id exactly |
| `test-scripting` `test_all_keyword_profiles` | TRIM (applied) | same                                                                                    |

Notes on kept tests:

- `test-core` `test_scanner` is the only user of `Scanner` (LX-D2). If `Scanner`
  is dropped, keep its `paint()` assertion elsewhere.
- `test-cpp` `test_raw_string_state` passes only because it never checks the
  newline on the opening line (LX-3).
- There are 17 copies of `has_role` in five variants; eight return `false`
  without saying which token was missing. Extract one
  `tests/support/has_role.h` templated on `Style` before adding many tests.

## Missing tests

### P1: library-wide invariants

1. **Torture test over every lexer** (needs LX-D6 or a shared name list).
   Lex each input whole and at every prefix length, and assert termination,
   no contract violation, `styles.size() == source.size()` and
   `line_states.size() == line_starts.size()`. Inputs: `""`, `"\n"`, `"\r"`,
   `"\r\n"`, `"a\rb\r"`, lone quotes/backticks/backslashes, triple quotes,
   every comment opener (`/*`, `/*/`, `(*`, `{-`, `#|`, `#=`, `#[`, `<!--`,
   `<![CDATA[`, `<#`), raw/verbatim/long-bracket strings, heredocs, `${`,
   line-block openers (`%{`, `=begin`, `=pod`, `#cs`, `@ignore`), fences, non-ASCII
   bytes (0x80 and above) and NUL, CRLF variants, a 1 MiB single line, a
   50,000-line unterminated `/*` (guards LX-D5). Run it in the ASan lane
   (`pixi run test-remote-linux-sanitize --host cachy`) or with
   `-D_GLIBCXX_ASSERTIONS` so out-of-bounds reads are caught.
2. **Streaming equivalence property test.** For every lexer and corpus, lexing
   whole (`assign`) must equal feeding the text via `append` at every 2-way
   split and in fixed chunk sizes {1, 2, 3, 7}, including empty chunks and a
   CRLF split between `\r` and `\n`. Compare styles and line states. Should
   hold today except for empty chunks (LX-5).
3. **Foreign line state terminates** (LX-9), once the contract is decided.

### P2: regressions for known bugs (all fail today)

LX-1, LX-2 (three inputs), LX-3, LX-4a/b/c, LX-5, LX-6, LX-7, LX-8 with the
inputs given in lexer.md. Add a streamed variant of each (split after the first
`\n`) to exercise the line state.

### P3: dialect coverage

Dialects registered but untested or only keyword-tested:

- cpp: OBJECTIVE_C, RESOURCE_SCRIPT (lexing is identical to C++ today).
- bash: C_SHELL, M4 (m4 `dnl` comments lex as commands).
- javascript: JSX.
- structured_data: CONFIG, TSV; JSON multi-line string/comment streaming.
- brace: D, DART, CANGJIE, GROOVY, GRADLE, HAXE, SCALA, ASYMPTOTE; C# `@"…"`.
- build_script: JAM; multi-line comments in GN/NSIS/Inno, CMake multi-line strings.
- assembly: CIL, WINHEX.
- functional: multi-line OCaml/F# `(* *)`, Haskell `{- -}`, Elixir/F# `"""`.
- scripting: stateful constructs (AutoIt `#cs/#ce`, Perl `=pod`, Ruby `=begin`,
  CoffeeScript `###`, Mathematica `(* *)`, MATLAB `%{ %}`, Nim `#[ ]#`, Lua
  `--[[ ]]`, PowerShell `<# #>` and `@'…'@`, `"""` strings).
- legacy: APDL, POWERBUILDER, VBSCRIPT; Pascal `(* *)`.
- document_markup: Texinfo `@ignore`; Graphviz/blockdiag `/* */`.
- Optional: compile-fail tests for the consteval checks (duplicate alias,
  style without a role, unsorted `WordSet`).
