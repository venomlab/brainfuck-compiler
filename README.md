# brainfuck-compiler

Brainfuck compiler built with LLVM infrastructure

## Development

### Requirements

- CMake 3.28 or newer
- Ninja
- Clang 18.1
- LLVM 18.1 development files and static libraries
- clang-format 18.1
- clang-tidy 18.1
- LLD 18.1
- GoogleTest development files
- zlib development files and static library
- zstd development files and static library
- ncurses development files and static library
- libxml2 development files and static library
- pre-commit 3.5 or newer

### Debug build and tests

```sh
make test
```

### Release build

```sh
make build-release
```

### Sanitizers and static analysis

Run AddressSanitizer with UndefinedBehaviorSanitizer, then ThreadSanitizer:

```sh
make sanitize
```

Run either sanitizer configuration separately:

```sh
make test-sanitize
make test-tsanitize
```

Run clang-tidy and the test suite:

```sh
make tidy
```

### clangd

The debug preset writes `build/debug/compile_commands.json`. The repository
`.clangd` file points clangd to that directory. Configure the debug preset at
least once before using clangd:

```sh
make build-debug
```

### Pre-commit hooks

Install hooks once per clone:

```sh
pre-commit install
```

The clang-format hook formats C and C++ files in place. Review and stage its
changes, then rerun the hooks. The clang-tidy hook configures the `clang-tidy`
preset, builds the project with clang-tidy enabled, and runs the test suite.
