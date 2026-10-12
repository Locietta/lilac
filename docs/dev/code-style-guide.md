This is a concise code style guide for the lilac library. Contributors or agents should follow these guidelines unless there is a compelling reason to deviate.

If not specified here, just follow the code style of the existing code around the code you are writing.

## Formatting

Run `pixi run format <paths...>` to format specific files or directories. Pass `--language <cpp|python|markdown|json|toml>` to override language detection for every input.

## Identifier Naming

See [.clang-tidy](../.clang-tidy).

## Header Inclusion

The headers should be grouped with the following order:

```c++
/// interface header of the current .cpp file
#include "interface.h"

// std headers
#include <cstdlib>
#include <vector>

// other 3rd party headers
#include <proxy/proxy.h>

// other headers from this project
#include <lilac/document.h>
#include <lilac/role.h>
```

## Structs, Custom Types

Always use `struct`, do not use `class` for custom types. This way, we don't need to write `public:` for every struct, and when we need to inherit, we don't need to write `public` in the inheritance list.

## Polymorphism

We generally avoid virtual functions in this project. If you need polymorphism, use `ngcpp-proxy`. It provides a dynamic dispatch mechanism based on fat pointers, much closer to Rust's trait objects. It also supports static dispatch with `PRO_DEF_MEM_DISPATCH` and `PRO_DEF_FREE_DISPATCH`. An example:

```c++
struct RenderContext {};

PRO_DEF_MEM_DISPATCH(DrawDispatch, draw); // generate a templated dispatch funtion draw at global scope

inline constexpr DrawDispatch draw{};

struct DrawableFacade
    : pro::facade_builder
        ::add_convention<
            DrawDispatch,
            void(RenderContext&) const>
        ::build
{};

struct Mesh {
    void draw(RenderContext&) const
    {
        // ...
    }
};

template<class T>
concept Drawable = requires(
    const T& value,
    RenderContext& ctx)
{
    { draw(value, ctx) } -> std::same_as<void>;
};

void render_static(
    const Drawable auto& value,
    RenderContext& ctx)
{
    draw(value, ctx);        // statically dispatched
}

void render_dynamic(
    const pro::proxy<DrawableFacade>& value,
    RenderContext& ctx)
{
    draw(*value, ctx);       // dynamically dispatched by Proxy
}
```

This way we can keep the main structs in our codebase as plain non virtual structs, with better memory layout and data locality, and still have polymorphism when needed.

## Callable Wrappers

Use the standard callable wrappers according to ownership:

- Use `std::copyable_function` for owning callbacks that are copied.
- Use `std::move_only_function` for owning callbacks that are transferred or may capture move-only state.
- Use `std::function_ref` only for synchronous, non-storing callback parameters whose target is guaranteed to outlive the call.

## Basic Types

Use the short convenience types defined in `lilac/types.h`; for example, the project spelling for an unsigned size is `usize`. Add an alias there when a new one is needed.

These types are directly available inside the `lilac` namespace. In another namespace, `using namespace lilac::types;` is generally recommended.

## Assertions and Contracts

Do not use `<cassert>` or `assert()`. Use C++26 contracts for programmer obligations and internal invariants:

- Use `pre` and `post` for interface-level preconditions and postconditions. Place them on the first declaration, normally in the public header, and do not repeat them on an out-of-line definition.
- Use `contract_assert` for internal invariants at a particular point in an implementation.

Production builds use `-fcontract-evaluation-semantic=ignore`, while development and test builds use `enforce`. Consequently, correctness in production must not depend on a contract predicate being evaluated.

## Failure Handling

Lexing never fails: any input, including malformed or truncated source, must be lexed to completion without reading out of bounds, looping forever, or aborting. Unrecognized text gets a style whose role is `TokenRole::UNRECOGNIZED` or `TokenRole::DEFAULT`.

Configuration mistakes (duplicate language aliases, a style without a role) are caught at compile time by throwing from `consteval` code.

## Tests

Tests must be deterministic. A lexer test should state the input and the expected styles or line states, and should fail for a plausible regression; prefer cases that cross line and `append` boundaries, since that is where incremental lexing breaks.

## Commit Messages

The headline of a commit should follow the format: `<type>(<scope>): <subject>`, type or scope can be omitted if not applicable.

If the commit is big enough and you cannot describe all important changes in the headline, you can add a body to the commit message. The body should be separated from the headline by a blank line, and preferably be a list of bullet points.

An example:

```
refactor(tui): centralize slash command parsing

- Lexes REPL input once into either a user prompt or an exact command name and argument string.
- Resolves commands and aliases centrally and keeps command recognition out of individual handlers.
- Reports unknown slash commands as command errors.
- Supports a leading double slash as an escaped slash-prefixed user prompt.
```
