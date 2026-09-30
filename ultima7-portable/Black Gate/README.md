# Ultima VII: The Black Gate, portable

A port of the reconstructed Ultima VII: The Black Gate source (originally 16-bit Borland C++ for
DOS) to modern systems. The game code talks to the machine only through a small platform layer;
the first backend uses [libmatoya](https://github.com/snowcone-ltd/libmatoya) for the window,
input, video, audio and files, and [Munt](https://github.com/munt/munt) to emulate the Roland
MT-32 for music and sound effects.

Everything needed to build is in this folder. You supply the game data and the MT-32 ROMs.

## Layout

```text
ultima7-portable/Black Gate/
  CMakeLists.txt         the build
  cmake/Libmatoya.cmake  builds libmatoya with its own makefiles
  game/                  the game: .c files are C, .cpp files are C++, as originally written
  platform/plat.h        everything the game needs from the host
  platform/matoya/       the libmatoya backend (window, input, audio, files, timing)
  third_party/libmatoya/ libmatoya, vendored at a pinned commit (MIT)
  third_party/munt/      Munt's mt32emu library, vendored at a pinned tag (LGPL 2.1+)
  tools/u7_new_game.py   sets up a new game with a named Avatar
```

## Requirements

| | macOS (tested) | Linux x64 (tested under WSL2, Ubuntu 24.04) | Windows x64 (tested) |
| --- | --- | --- | --- |
| Compiler | Xcode command line tools (clang) | GCC 11+ or clang 14+ | clang-cl, from Visual Studio 2022 with "C++ Clang tools for Windows" |
| Build tools | CMake 3.20+, make | CMake 3.20+, make | CMake 3.20+, Ninja and nmake (both come with Visual Studio) |
| Extra | | `glslangValidator` (Ubuntu/Debian: `glslang-tools`), for libmatoya's shaders | |

On Linux, libmatoya opens X11, OpenGL/Vulkan and ALSA at run time, so a desktop session with
those libraries installed is needed to play. Under WSL, WSLg provides the window, but its sound
server is PulseAudio only: install `libasound2-plugins` and route ALSA to it with a `~/.asoundrc`
holding `pcm.!default { type pulse }` and `ctl.!default { type pulse }`, or the game starts with
"no audio output".

## Build

```sh
cmake -S "ultima7-portable/Black Gate" -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target u7
```

On Windows, build from an "x64 Native Tools Command Prompt for VS 2022", with clang-cl (MSVC's
own `cl` rejects some of the game's C++):

```bat
cmake -S "ultima7-portable/Black Gate" -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl
cmake --build build --target u7
```

libmatoya's makefile relies on `cmd`'s `mkdir`. If Git's `usr\bin` is on `PATH`, nmake runs
Git's `mkdir` instead and the libmatoya step fails, so build from a prompt without it.

The first build also builds libmatoya (through its own makefile, into
`third_party/libmatoya/bin/`) and mt32emu (as a CMake subproject). The program is `build/u7`
(`build\u7.exe` on Windows).

Options:

- `-DU7_MT32_ROM_DIR=/path/to/roms` bakes in a default ROM folder (see below).
- `-DCMAKE_BUILD_TYPE=Debug` for debugging. A sanitizer build is useful for finding memory bugs:
  `-DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-sanitize=alignment"` (same for
  `CMAKE_CXX_FLAGS`), then run with
  `ASAN_OPTIONS=handle_segv=0:handle_sigbus=0:detect_leaks=0`.

## What you need to supply

**Game data.** A folder holding an installed copy of Ultima VII: The Black Gate, with its
`STATIC` folder (and `GAMEDAT`, if you have a game in progress). File names are matched in any
letter case. The game writes to this folder, so use a copy.

**MT-32 ROMs.** Munt needs the Roland MT-32 control and PCM ROM images
(`MT32_CONTROL.ROM`, `MT32_PCM.ROM`; CM-32L images also work). They are not included. Point the
game at their folder with the `U7_MT32_ROMS` environment variable or the `U7_MT32_ROM_DIR` build
option. Without them the game runs with no music or MIDI sound effects.

## Run

```sh
build/u7 --data /path/to/ultima7 -p
```

- `--data` is the game data folder (or set `U7_DATA`; the default is the current folder).
- `-p` is the flag the original launcher passed; the game will not start without it.

**Starting a new game.** The original main menu isn't part of this port yet. This sets one up,
the way the menu did, and the game builds it on its next start:

```sh
python3 "ultima7-portable/Black Gate/tools/u7_new_game.py" /path/to/ultima7 --name Jamie --female
```

It copies the current `GAMEDAT` next to the data folder first, because the game deletes it.

Other environment variables:

| Variable | Use |
| --- | --- |
| `U7_MT32_ROMS` | folder holding the MT-32 ROMs |
| `U7_AUDIO_DUMP` | write the mixed audio to this file (raw 16-bit stereo, 48 kHz) |
| `U7_TEST_INPUT` | scripted input for testing, e.g. `"4000 key i; 6000 shot frame.ppm; 7000 quit"` (see `platform/matoya/testinput.c`); the window stays hidden and no sound plays |

## Dependencies

Both libraries are vendored as plain source at fixed versions (see `third_party/README.md`), so
no network access or git submodules are needed.

**libmatoya** has no CMake project of its own. `cmake/Libmatoya.cmake` runs its own makefile
(`GNUmakefile` on macOS and Linux, `makefile` with nmake on Windows) and links the resulting
static library, adding the system frameworks or libraries it needs. To build it by hand:

```sh
cd third_party/libmatoya
make -f GNUmakefile TARGET=macosx ARCH=arm64    # or TARGET=linux ARCH=x86_64 / aarch64
```

**mt32emu** is built from `third_party/munt/mt32emu` as a CMake subproject, as a static
library with its C interface. The ROMs are loaded at run time, never linked in. Munt is under the
GNU Lesser General Public License 2.1 or later: if you distribute a binary, you must also make it
possible to relink it against a modified mt32emu (for example, by offering the game's object
files or building mt32emu as a shared library with `-Dlibmt32emu_SHARED=ON`), and include
`third_party/munt/mt32emu/COPYING.LESSER.txt`.

## Known limitations

- **Null-pointer reads** that the DOS game relied on are served by a fault handler that exists
  only for arm64 macOS, x64 Windows and x64 Linux so far (`platform/matoya/nullpage.c`).
  Elsewhere (arm64 Linux or Windows, for one) they crash.
- **AdLib** music is not supported: the AdLib driver inside `U7STRAX.DRV` has not been recovered.
  MT-32 music and speech work.
- **The main menu, intro and endgame** are separate programs in the original and are not
  ported. Quitting ends the program; restart and endgame are not handled yet.
- **Game speed** follows the community frame-limiter patch: about 10 world updates a second.
