# brainfuck-compiler

Brainfuck compiler built with LLVM infrastructure. It actually compiles

## Installation

Visit the [Latest Release](releases/latest) page

Download the `bfc-x86_64-linux.tar.gz` archive, extract it, and place the `bfc` binary somewhere in your `PATH`.

```sh
tar -xzf bfc-x86_64-linux.tar.gz
chmod +x bfc
sudo mv bfc /usr/local/bin/
```

Verify the installation:

```sh
bfc --version
```

Alternatively, install it only for the current user:

```sh
mkdir -p ~/.local/bin
mv bfc ~/.local/bin/
```

Make sure `~/.local/bin` is included in your `PATH`.

The release binary itself is a fully static `x86_64` Linux executable and does not require LLVM, LLD, Clang, or other compiler dependencies to run.

Some compilation targets may still require external tools or runtime libraries.
Targets supported by the bundled runtime and embedded LLD work without external compiler toolchains;
see [Bundled runtime targets](#bundled-runtime-targets) for details.

## Usage

```text
bfc [--ir|--asm|--obj|--exe] [-o FILE] [-t|--target TARGET] [PROGRAM]
```

`PROGRAM` is optional. Without it, or when it is `-`, `bfc` reads the
Brainfuck program from standard input.

Output goes to standard output by default. This includes binary object files
and executables. Use `-o FILE` to write the artifact to a file.

Output formats:

- `--ir` emits LLVM IR
- `--asm` emits target assembly
- `--obj` emits an object file
- `--exe` emits a fully static executable

The format flags are mutually exclusive. Without a format flag, `bfc` emits an
executable.

When `-o` is present, its extension can select the output format:

- `.ll` selects LLVM IR
- `.s` and `.asm` select assembly
- `.o` and `.obj` select an object file
- `.out`, `.exe`, and no extension select an executable

An explicit format flag must match the output extension. Unknown extensions
are rejected.

`-t TARGET` and `--target TARGET` select an LLVM target triple. The host target
is used by default. Assembly and object generation support all targets built
into LLVM.

Examples:

```sh
bfc --ir hello.bf
bfc -o hello.o hello.bf
bfc --target aarch64-unknown-linux-gnu -o hello.s hello.bf
bfc -o hello hello.bf
bfc --target x86_64-w64-windows-gnu -o hello.exe hello.bf
bfc --version
```

## Bundled runtime targets

Some targets work completely isolated without relying on external linkers or C
runtimes. These targets use a bundled platform runtime and embedded LLD. No
external compiler, LLVM tool, C runtime, startup object, SDK, or linker is
invoked.

Linux targets use a syscall runtime and embedded `lldELF`:

- `x86_64-unknown-linux-gnu`
- `x86_64-pc-linux-gnu`
- `x86_64-unknown-linux-musl`

The runtime accepts any `x86_64-*-linux-*` triple except GNU X32.

Windows targets use a Kernel32 runtime and embedded `lldCOFF`:

- `x86_64-w64-windows-gnu`
- `x86_64-pc-windows-msvc`
- `x86_64-pc-windows-itanium`

The runtime accepts any `x86_64-*-windows-*` triple. Generated PE32+
executables import only `KERNEL32.dll`; the required import library is created
internally as a temporary artifact. Cross-compiling from Linux and compiling
natively with the Windows build both work without Clang, a Windows SDK, MinGW
runtime libraries, or an external linker.

Other target triples fall back to `clang` through `PATH` and require a matching
static C runtime, startup objects, and linker.

## Author's note

This is my pet project with goal to learn compilers building,
and deepen/consolidate my C++ knowledge.

Enjoy this unholy creation :D

---

Even having that Brainfuck is a fun language for fun excercise, but this compiler is actually very real.
Unlike a lot of other "compilers" that just transpile it to other language
(like my other brainfuck python "compiler" as well :P)
this one actually parsers brainfuck and produces LLVM module that can produce
LLVM IR/ELF obj/Asm or even exe file directly

And I'm planning to add native support to more and more compile targets soon with goal
to enable cross-compilation. Thanks to LLVM it is way easier than it might be

But even having power of statically linked LLVM and LLD I still was very challanged with
not being able to produce statically linked brainfuck programs due to reliance on libc
for reading/outputting characters (putchar/getchar). The bundled runtimes solve this by
using syscalls directly on x86_64 Linux and Kernel32 I/O on x86_64 Windows.

So, in order to compile to a bundled x86_64 Linux or Windows target using this compiler,
you don't need external compiler software and the produced executable stays tiny.

Other targets for now require having `clang`, a linker, and C runtime libraries in order to compile.
I'll try to extend native target support over time.


## Development

### Requirements

- CMake 3.28 or newer
- Ninja
- pkg-config
- Clang 18.1
- clang-format 18.1
- clang-tidy 18.1
- vcpkg with CLI11 installed for the `x64-linux` triplet
- `VCPKG_ROOT` pointing to the vcpkg installation
- MinGW-w64 GCC toolchain for Windows `x86_64` cross-builds
- vcpkg support for the `x64-mingw-static-release` community triplet
- Wine for running Windows cross-builds on Linux (optional)
- LLVM 18.1 development files and static libraries
- Polly 18.1 development files and static library
- LLD 18.1 development files, static libraries, and linker
- zlib development files and static library
- zstd development files and static library
- ncurses development files and static library
- libxml2 development files and static library
- ICU development files and static libraries
- LZMA development files and static library
- C and C++ static runtime libraries
- C runtime startup objects
- System linker with fully static linking support
- GoogleTest development files
- pre-commit 3.5 or newer

### Debug build and tests

Debug builds link LLVM and LLD dynamically.

```sh
make test
```

### Release build

Release builds produce a fully static `bfc` executable and link LLVM and LLD
statically.

```sh
make build-release
```

### Windows x86-64 cross-build

Windows builds use MinGW-w64 GCC. LLVM, LLD, libstdc++, and libgcc are linked
statically; the resulting executable depends only on Windows system DLLs.

```sh
make gcc-release-w64
```

The executable is written to `build/gcc-release-w64/bfc.exe`.

The first build installs CLI11 and LLVM for the `x64-mingw-static-release`
triplet through the isolated vcpkg manifest. This can take significant time
when no matching binary-cache entries exist. Later builds reuse the build tree
and vcpkg binary cache.

Run the cross-built compiler through Wine:

```sh
wine build/gcc-release-w64/bfc.exe --version
```

The Windows build uses its native target by default. It can produce a Windows
executable without external Clang or a Windows SDK:

```sh
wine build/gcc-release-w64/bfc.exe -o hello.exe hello.bf
wine hello.exe
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
