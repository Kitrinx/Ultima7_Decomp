# Recovered Black Gate routines

Each admitted source recompiles under Borland C++ 2.0 to the shipped bytes exactly.
RELOC means every relocated operand was also checked by name.
EXACT requires separate evidence for bindings the gate cannot resolve.
Complete compiled units follow the original source layout; compound and library segments remain qualified.
The top-level folders are the directories BG paths name (`common/`, `inter/`, `sound/`, `zevent/`,
`usecode/`), plus `main/` (the build directory), `include/` and `runtime/` (identified library code).
A file joins a folder by a BG path, an SI link group, a compiler-flag group, or its subsystem; each
source header and the layout registry say which. The shipped paths name no sub-folder, so
`main/combat/`, `main/schedule/`, `main/palette/`, `main/overlay/`, `main/xmm/`, `main/dos/`,
`main/mem/`, `main/video/` and `sound/drv/` are inferred groupings.
Inferred 8.3 filenames are marked in the layout registry; assembly remains `.asm`.
Partial units land in `staging/`, unverified ones in `staging/pending/`; both exist only while in use.
A module's header has its name and sits beside it; `include/` holds only headers for shared types.

| file | segment | file offset | bytes | form | flags | status |
| --- | --- | --- | --- | --- | --- | --- |
| `mainmenu.c` | 1 | 0x007652 | 10703 | whole segment | `-O -1 -P -d` | RELOC |
| `credits.c` | 2 | 0x00a021 | 5509 | whole segment | `-O -1 -P -d` | RELOC |
| `xmsflat.asm` | 3 | 0x00b5a6 | 99 | whole segment | `/mx` | RELOC |
| `fontprn.c` | 4 | 0x00b609 | 463 | whole segment | `-O -1 -P` | RELOC |
| `flexprn.c` | 5 | 0x00b7d8 | 382 | whole segment | `-O -1 -P` | RELOC |
| `menu.c` | 6 | 0x00b956 | 1359 | whole segment | `-O -1 -P` | RELOC |
| `menuent.c` | 7 | 0x00bea5 | 179 | whole segment | `-O -1 -P` | RELOC |
| `main/controls.c` | 8 | 0x00bf58 | 5335 | whole segment | `-O -1 -P` | RELOC |
| `zevent/u7point.c` | 9 | 0x00d42f | 832 | whole segment | `-O -1 -P` | RELOC |
| `textfld.c` | 10 | 0x00d76f | 746 | whole segment | `-O -1 -P` | RELOC |
| `textbox.c` | 11 | 0x00da59 | 420 | whole segment | `-O -1 -P` | RELOC |
| `device.c` | 12 | 0x00dbfd | 563 | whole segment | `-O -1 -P` | RELOC |
| `sounddev.c` | 13 | 0x00de30 | 237 | whole segment | `-O -1 -P` | RELOC |
| `timerdev.c` | 14 | 0x00df1d | 53 | whole segment | `-O -1 -P` | RELOC |
| `keyqueue.c` | 15 | 0x00df52 | 520 | whole segment | `-O -1 -P` | RELOC |
| `textspr.c` | 16 | 0x00e15a | 1805 | whole segment | `-O -1 -P` | RELOC |
| `shapebnd.c` | 17 | 0x00e867 | 177 | whole segment | `-O -1 -P` | RELOC |
| `scroll.c` | 18 | 0x00e918 | 962 | whole segment | `-O -1 -P` | RELOC |
| `setdac.asm` | 19 | 0x00ee2e | 93 | whole segment | `/mx` | RELOC |
| `dosmem.c` | 20 | 0x00ee8b | 165 | whole segment | `-O -1 -P` | RELOC |
| `install.c` | 21 | 0x00ef30 | 1011 | whole segment | `-O -1 -P` | RELOC |
| `dirdelta.c` | 29 | 0x0107e4 | 0 | whole segment | `-O -1 -P` | RELOC |
| `main/chkfile.c` | 32 | 0x0110e9 | 1419 | whole segment | `-O -1 -P -b-` | RELOC |
| `zevent/dbgfont.c` | 32 | 0x011674 | 408 | whole segment | `-O -1 -P` | RELOC |
| `zevent/mouse.c` | 34 | 0x011ace | 826 | whole segment | `-O -1 -P` | RELOC |
| `zevent/cursor.c` | 34 | 0x011e08 | 0 | whole segment | `-O -1 -P` | RELOC |
| `zevent/systimer.c` | 37 | 0x011ee4 | 735 | whole segment | `-O -1 -P` | RELOC |
| `dlist.c` | 41 | 0x0123c8 | 694 | whole segment | `-O -1 -P` | RELOC |
| `sound/specache.c` | 48 | 0x014f07 | 1287 | whole segment | `-O -1 -P` | RELOC |
| `sound/drv/specard.c` | 49 | 0x01540e | 2096 | whole segment | `-O -1 -P` | RELOC |
| `main/xmm/drawtile.asm` | 60 | 0x016f74 | 161 | whole segment | `/mx` | RELOC |
| `cpumode.asm` | 61 | 0x018b22 | 42 | whole segment | `/mx` | RELOC |
| `main/video/vidmode.asm` | 72 | 0x019432 | 0 | whole segment | `/mx` | RELOC |
