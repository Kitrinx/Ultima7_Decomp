# Ultima VII Modern

One program that plays both Ultima VII: The Black Gate and Ultima VII Part Two: Serpent Isle, built
on the Serpent Isle port in `ultima7-portable/`. Unlike the portable ports it is not held to the
original's exact behaviour: it fixes bugs, brings Serpent Isle's conveniences to Black Gate, and
adds wide views, a start menu and its own save folder. Core gameplay stays as the originals had it.

## Folders

Both games are needed. By default they are read from `Documents/Ultima7` (Black Gate) and
`Documents/Serpent` (Serpent Isle), or from folders of those names next to the program; name them
with `--bg-data <dir>` and `--si-data <dir>`. The game folders are only read, never written.

Saved games, settings and everything else the games write go to the user data folder:

| System  | Folder |
|---------|--------|
| Windows | `%APPDATA%\Ultima7Modern` |
| macOS   | `~/Library/Application Support/Ultima7Modern` |
| Linux   | `$XDG_DATA_HOME/ultima7modern`, else `~/.local/share/ultima7modern` |

```text
<user data>/settings.json             screen shape, sound, townsfolk pace, ...
<user data>/Saves/Black Gate/         GAMEDAT, GAMEnn.U7 and the rest, by their DOS names
<user data>/Saves/Serpent Isle/
```

When folders named `Ultima7` and `Serpent` holding the games sit next to the program (on macOS, next
to `Ultima7Modern.app`), the settings and saves are kept next to the program too, so the whole set-up
is one folder. `U7_USER_DATA` overrides either place.

`--saves <dir>` puts the saves elsewhere. Saves of the original games are not imported. MT-32
ROMs are looked for in both game folders and the user data folder (or `U7_MT32_ROMS`).

**Frame limit** in the start menu caps gameplay in both games. It offers whole values from
2 to 10 FPS and defaults to 10 FPS. Lower values slow game turns and world animation.

The start menu's **Sound** setting offers **Best available** (default), **Adlib**, **MT32** and
**OPL3**. Best available uses MT32 with loaded ROMs, otherwise OPL3; MT32 cannot be selected without
ROMs. Adlib preserves the original OPL2 sound. OPL3 keeps FM instruments and envelopes, adds
smooth stereo panning and more voices, and borrows verified pan and missing notes from matching
MT32 music. It uses the optional stereo extension, retaining all 128 pan positions. Music notes
keep their starting pan; later pan commands affect new notes. Adlib keeps the classic chip behavior.
Positional effects use their world location; continuous merged ambience stays centered.
Original game files remain unchanged. The choice covers music and effects, including intros, menus
and endings. Volume controls either device and speech; native sound toggles still apply.
`--adlib`, `--roland` and `--opl3` override the choice for one run.

## Black Gate speech

Modern plays high quality speech from an optional `u7hq.spc` in Black Gate's `STATIC` folder.
Slots 0–33 use the clip numbers from `U7SPEECH.SPC` for gameplay and menus. Slot 34 plays the
Guardian's intro, and slots 35–37 play the three ending cues. For a missing or unusable entry
the game plays the corresponding original audio through the legacy player.

`u7hq.spc` is a FLEX archive with PCM WAV entries. Each populated entry holds little-endian
48 kHz, 16-bit stereo PCM (WAV format 1). Modern mixes those frames at their native rate and
uses the usual speech toggle and volume setting.

## Layout

```text
ultima7-modern/
  CMakeLists.txt         the build
  cmake/                 libmatoya build and the reset list
  game/                  the game: .c files are C, .cpp files are C++, as originally written
  game/serpent/          Serpent Isle's launcher and the program table
  game/blackgate/        Black Gate's launcher and programs
  game/startmenu/        the start menu and settings screen
  game/mainmenu/, intro/, endgame/, shared/   Serpent Isle's other programs
  game/usecode/          Serpent Isle's usecode source (reference, not compiled)
  platform/              the host layer and its libmatoya backend
  packaging/             macOS bundle text and the README that ships in release archives
  assets/                the program icons and other art
  tools/                 helper scripts
```

It uses `ultima7-portable/third_party/` (libmatoya, Munt's mt32emu and Nuked OPL3) from beside it.
Most of Black Gate's own paperdoll pictures (companions' heads, clothing, armour, weapons) are
Grant W. Chapman's art (github.com/GrantWChapman/Ultima-VII-Artwork), used with his permission.

## Build

```sh
cmake -S ultima7-modern -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target modern
```

On Windows, from an "x64 Native Tools Command Prompt for VS 2022":

```bat
cmake -S ultima7-modern -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl
cmake --build build --target modern
```

The program is `build/Ultima7Modern` (`build\Ultima7Modern.exe` on Windows, the app
`build/Ultima7Modern.app` on macOS). Run it with `--help` for its options; `--game bg|si` skips the
start menu. Don't build it at the same time as the portable ports:
they share libmatoya's build folder.
