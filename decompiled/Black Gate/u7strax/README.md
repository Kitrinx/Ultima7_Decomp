# U7STRAX.DRV

Black Gate's music drivers, a Flex file loaded by `LoadSoundDriver` (U7.EXE, INTRO.EXE and
MAINMENU.EXE all read `u7strax.drv`). Record `MUSIC_DEVICE - 1` is copied to a paragraph and
called through the offsets in its first 26 bytes: describe, init, shutdown, timer, note, control,
pitch, program, port, sysex, release voice, claim voice.

| file | record | bytes | what |
| --- | --- | --- | --- |
| `U7STRAX.ASM` | header | 144 | Flex header and record table |
| `MT32DRV.ASM` | 0 | 1,207 | Roland MT-32 through an MPU-401 in UART mode |
| `ADLIBDRV.ASM` | 1 | 4,028 | AdLib (OPL2), melodic or rhythm mode |

Both records were hand-written assembly: single-pass TASM's `jmp` + `nop` forward jumps and
symbol displacements throughout match it exactly. The timbre banks are separate: `U7VOICE.FLX`
(game), `U7INTRO.TIM` (intro and menu), records 0 and 1.

Serpent Isle's `MAINMENU.DRV` is the same file, byte for byte (same SHA-256).
