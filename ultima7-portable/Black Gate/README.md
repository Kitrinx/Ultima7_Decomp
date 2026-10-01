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
  game/ultima7/          the launcher (ULTIMA7.COM)
  game/mainmenu/, intro/, endgame/, shared/   the other programs, and what menu and intro share
  platform/plat.h        everything the game needs from the host
  platform/matoya/       the libmatoya backend (window, input, audio, files, timing)
  third_party/libmatoya/ libmatoya, vendored at a pinned commit (MIT)
  third_party/munt/      Munt's mt32emu library, vendored at a pinned tag (LGPL 2.1+)
  tools/u7_new_game.py   sets up a new game with a named Avatar
  packaging/README.txt   the players' README, shipped in the release archives
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
`third_party/libmatoya/bin/`) and mt32emu (as a CMake subproject). The program is `build/Ultima7`
(`build\Ultima7.exe` on Windows, the app `build/Ultima7.app` on macOS).

Options:

- `-DU7_MT32_ROM_DIR=/path/to/roms` bakes in a default ROM folder (see below).
- `-DCMAKE_BUILD_TYPE=Debug` for debugging. A sanitizer build is useful for finding memory bugs:
  `-DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-sanitize=alignment"` (same for
  `CMAKE_CXX_FLAGS`), then run with
  `ASAN_OPTIONS=handle_segv=0:handle_sigbus=0:detect_leaks=0`.
- `-DU7_RESET_CHECK=ON` (clang, macOS or Linux; for testing only) reports, after each handoff
  between programs, any game global that differs from how the first program found it.

## What you need to supply

**Game data.** A folder holding an installed copy of Ultima VII: The Black Gate, with its
`STATIC` folder (and `GAMEDAT`, if you have a game in progress). File names are matched in any
letter case. The game writes to this folder, so use a copy.

**MT-32 ROMs.** Munt needs the Roland MT-32 control and PCM ROM images (any file names; they
are recognised by content). CM-32L images and split control-ROM halves are not used. They are
not included. The game looks in the `U7_MT32_ROMS` folder, else the `U7_MT32_ROM_DIR` build
option, else the game data folder. Without them the game runs with no music or MIDI sound
effects.

**Sound setup.** `U7.CFG` is the setup the DOS installer wrote. Music here is the MT-32 or
nothing, so when the ROMs are found the game reads `U7.CFG` as saying Roland (`r 330`), and with
no speech line it reads one (`220 7 1`), whatever the file says (`platform/matoya/files.c`). The
file on disk is never changed. The in-game audio options still turn each part off.

## Run

```sh
build/Ultima7 --data /path/to/ultima7
```

On macOS the program inside the app takes the same options:
`build/Ultima7.app/Contents/MacOS/Ultima7 --data /path/to/ultima7`.

- `--data` is the game data folder (or set `U7_DATA`). By default it is the current folder if
  that has `STATIC`, else the folder the program is in (on macOS, the folder holding
  `Ultima7.app`), else `Documents/Ultima7`, so a double-clicked `Ultima7` placed in the game
  folder finds its data on every platform. The game checks for its core files at
  launch and names any that are missing or empty.
- On Windows the program is a windowed one, so no console opens with it. Started from a
  terminal, it prints its messages there; redirected output goes to the file or pipe as usual.
- This is the launcher, as `ULTIMA7.COM` was: it runs the main menu, intro, game and endgame in
  turn, in one process and one window, each starting afresh as its own EXE did. Other arguments
  go on to the game.
- `--program <name> <args>` runs one program alone: `u7 -p` for the game (`-p` is the flag the
  launcher passes; the game will not start without it), `mainmenu v`, `intro ereiamjh`,
  `endgame ereiamjh`.
- `--help` lists every switch.
- Alt-Enter (Option-Return on a Mac keyboard) toggles fullscreen.

The game's own command-line options, as switches (with the letter U7.EXE took):

| Switch | Original | Effect |
| --- | --- | --- |
| `--cheat` | `ABCD` + Alt-255 | cheat keys (F1 lists them, on the game screen as in DOS) |
| `--cheat-start` | `s` | with `--cheat`: move at once, move anything, an Avatar that can't die, debug output |
| `--speech` | `v` | speech on |
| `--adlib[=port]` | `a` | AdLib music (every score plays on the MT-32 here) |
| `--roland[=n]` | `r` | Roland MT-32 music |
| `--shape-pool=KB` | `c` | size of the shape cache |
| `--overlay-size` | `b` | report the DOS overlay buffer size, then stop |
| `--version` | `?` | show the version, then stop |

**Starting a new game.** Use "Start New Game" in the main menu. For scripted runs that skip the
menu, this sets one up the way the menu does, and the game builds it on its next start:

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
