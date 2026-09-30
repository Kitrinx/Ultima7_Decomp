# Ultima VII usecode tools

Tools to compile, link, decompile and check the usecode of Ultima VII: The Black Gate and
Serpent Isle. The main one, `u7ucproj`, builds a folder of readable `.use` sources into the
game's `USECODE`, `LINKDEP1` and `LINKDEP2` files.

Everything needed to build is in this folder. You supply the game's usecode.

## Requirements

- A C17 compiler (clang or GCC).
- `make`, or CMake 3.16+.
- `sh`, `grep` and `sed`, for the tests.

No other libraries. `u7ucdec` uses POSIX `mkdir` and `strcasecmp`, so Windows needs a POSIX
toolchain such as MinGW or MSYS2.

## Build

```sh
make            # tools land in build/
make test       # unit tests and an edit-and-rebuild test
make install    # copies the tools to $(PREFIX)/bin, /usr/local by default
make clean
```

Or with CMake:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

To also check the games' shipped `USECODE` files, point `-DU7_ROOT=` at a folder holding the
game installs.

## Building a usecode project

A project is a folder with a `project.u7p` manifest, the `.use` sources, a header (`usecode.uh`),
a link order (`usecode.lnk`) and a number map (`usecode.map`).

```sh
make usecode PROJECT=path/to/project.u7p OUT=out   # writes out/USECODE, LINKDEP1, LINKDEP2
make check   PROJECT=path/to/project.u7p OUT=out   # also fails unless USECODE matches the
                                                   # manifest's expected sha256
```

`PROJECT` is required; `OUT` defaults to `build/usecode`. Copy the three output files into the
game's `STATIC` folder to play them.

Directly:

```sh
build/u7ucproj [--check] project.u7p out    # out must already exist
```

To start a project from a game's own usecode:

```sh
build/u7ucdec --project myproject STATIC/USECODE
```

## The tools

| Tool | Does |
| --- | --- |
| `u7ucproj` | Builds a project into `USECODE`, `LINKDEP1` and `LINKDEP2` |
| `u7ucdec` | Decompiles `USECODE` into one source file, or a project with `--project` |
| `u7uccomp` | Compiles one source file into `USECODE` |
| `u7uclink` | Rebuilds `LINKDEP1` and `LINKDEP2` for a `USECODE` |
| `u7ucdis` | Disassembles one function |
| `u7ucverify` | Checks a `USECODE` file's structure |
| `u7ucround` | Decompiles, recompiles and compares, function by function |
| `u7ucflow` | Reports each function's control flow |
| `u7ucsym` | Extracts and transfers names from the Serpent Isle beta's debug records |

Run a tool with no arguments for its options. `u7ucdec` and `u7uccomp --readable` take names
from `U7_SYMBOLS`, `U7_INTRINSIC_NAMES` and `U7_GAME` when set.

## Layout

```text
include/u7/   library headers
src/          the library: reading, decompiling, compiling and linking usecode
tools/        one file per tool
tests/        unit tests, the edit-and-rebuild test, and the shipped-file checks
```
