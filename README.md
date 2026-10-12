# lilac

lilac is a small C++26 syntax-highlighting lexer library that I use for my personal projects.

## What it provides

- Lexers for about 90 languages and formats, selected by Markdown fence name
  (`lilac::lexer_for_language("cpp")`). They are ported from the
  [Notepad4](https://github.com/zufuliu/notepad4) / Lexilla lexers.
- A `Document` that keeps one checkpoint state per line, so streamed text can be
  appended and only the unfinished tail is re-lexed (`lilac::append`).
- Language-neutral token roles (`lilac::TokenRole`). Each lexer annotates its
  own style enum with roles, and C++26 reflection turns that into a lookup table
  at compile time.

## Using it from another project

lilac is built with [xmake](https://xmake.io). The package lives in my xmake
package repo ([Locietta/xmake-repo](https://github.com/Locietta/xmake-repo)):

```lua
add_repositories("Locietta git@github.com:Locietta/xmake-repo.git")

add_requires("lilac 0.1.0")

target("app")
    add_packages("lilac")
```

lilac relies on C++26 static reflection and contracts, so for now it needs GCC 16
(`-freflection -fcontracts`); on Windows that means MinGW GCC.

With MinGW, every executable has to define `handle_contract_violation`: libstdc++exp
only provides a weak default, which a static archive cannot supply. Forward to
`std::contracts::invoke_default_contract_violation_handler` if you have nothing better
(see `lilac/tests/support/contract_handler.cpp`). lilac does not define it itself, so
it does not clash with the application's or another library's handler.

## Working on lilac itself

```powershell
pixi run configure     # releasedbg, which is what we develop and test with
pixi run build
pixi run test-all
```

`pixi run test-remote-linux --host <host>` runs the same tests on a Linux machine
over SSH; `test-remote-linux-sanitize` adds ASan/UBSan.

To try lilac changes in a consumer without publishing a release, point the
package at your checkout and rebuild it:

```powershell
$env:LILAC_SOURCE_DIR = "D:\.Project\lilac"
pixi run xmake require -f -y lilac
pixi run build
```

## License

MIT. The lexers derive from upstream code under the licenses listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
