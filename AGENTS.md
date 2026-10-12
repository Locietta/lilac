This is "lilac", a C++26 syntax-highlighting lexer library (Lexilla/Notepad4 lexers rewritten around C++26 reflection). It was split out of lighter and is consumed by liminal as an xmake package (recipe in `Locietta/xmake-repo`, `packages/l/lilac`).

This project is built with C++26 and xmake. Check the coding style guide at `docs/dev/code-style-guide.md`. Lexers derived from upstream code must keep the attributions in `THIRD_PARTY_NOTICES.md` up to date.

Compiler toolchain and python tools are managed by pixi:

- `pixi run configure`: development configure
- `pixi run build`: build the project
- `pixi run format <paths...>`: format given files
- `pixi run test-all`: run all tests
- `pixi run test-remote-linux --host cachy` (and `-sanitize`): Linux and ASan/UBSan lanes on the remote host

C++ dependencies should be managed by xmake. The only runtime dependency is ngcpp-proxy; keep it that way.

Versions follow semver (0.x for now: breaking changes bump the minor). Consumers pin a released version. To release: bump `version` in `pixi.toml`, tag `vX.Y.Z`, update the recipe's `add_versions` in `Locietta/xmake-repo`, and bump the consumers. Consumers can build against a local checkout with `LILAC_SOURCE_DIR`.

Commit headlines follow `<type>(<scope>): <subject>`.
