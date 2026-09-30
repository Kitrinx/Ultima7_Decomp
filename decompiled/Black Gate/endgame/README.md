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
| `endgame.c` | 1 | 0x005fd1 | 11009 | whole segment | `-1 -G -P -vi-` | RELOC |
| `display.c` | 3 | 0x008b92 | 526 | whole segment | `-1 -G -P -vi-` | RELOC |
| `flic.c` | 4 | 0x008da0 | 1693 | whole segment | `-1 -G -P -vi-` | RELOC |
| `common/easyfile.c` | 5 | 0x00943d | 620 | whole segment | `-1 -G -P -vi-` | RELOC |
| `common/strfmt.c` | 6 | 0x0096a9 | 165 | whole segment | `-1 -G -P -vi-` | RELOC |
| `dirdelta.c` | 6 | 0x00974e | 0 | whole segment | `-O -1 -P` | RELOC |
| `sounddrv.c` | 7 | 0x00974e | 1891 | whole segment | `-1 -G -P -vi-` | RELOC |
| `xmidi.c` | 8 | 0x009eb1 | 1371 | whole segment | `-1 -G -P -vi-` | RELOC |
| `digital.c` | 9 | 0x00a40c | 1091 | whole segment | `-1 -G -P -vi-` | RELOC |
| `speech.c` | 10 | 0x00a84f | 1692 | whole segment | `-1 -G -P -vi-` | RELOC |
| `chunknam.c` | 11 | 0x00aeeb | 242 | whole segment | `-1 -G -P -vi-` | RELOC |
| `sound/gsound.c` | 12 | 0x00afdd | 880 | whole segment | `-1 -G -P -vi- -d` | RELOC |
| `shapefnt.c` | 13 | 0x00b34d | 927 | whole segment | `-1 -G -P -vi-` | RELOC |
| `textwin.c` | 14 | 0x00b6ec | 2060 | whole segment | `-1 -G -P -vi-` | RELOC |
| `main/endstats.c` | 15 | 0x00bef8 | 347 | whole segment | `-1 -G -P -vi- -d` | RELOC |
| `options.c` | 16 | 0x00c053 | 674 | whole segment | `-1 -G -P -vi- -d` | RELOC |
| `main/videomd.c` | 19 | 0x00c392 | 81 | whole segment | `-1` | RELOC |
| `vidrest.c` | 20 | 0x00c3e3 | 14 | whole segment | `-O -1 -P` | RELOC |
| `rowtable.c` | 22 | 0x00c401 | 145 | whole segment | `-O -1 -P` | RELOC |
| `view.c` | 23 | 0x00c492 | 521 | whole segment | `-O -1 -P -d` | RELOC |
| `scrview.c` | 24 | 0x00c69b | 82 | whole segment | `-O -1 -P -d` | RELOC |
| `palstep.c` | 25 | 0x00c6ed | 352 | whole segment | `-O -1 -Z -P` | RELOC |
| `dacwrite.asm` | 26 | 0x00c84e | 86 | whole segment | `/mx` | RELOC |
| `dacread.asm` | 27 | 0x00c8a4 | 107 | whole segment | `/mx` | RELOC |
| `box.c` | 28 | 0x00c90f | 174 | whole segment | `-O -1 -P -d` | RELOC |
| `palette.c` | 29 | 0x00c9bd | 1534 | whole segment | `-O -1 -P -d` | EXACT |
| `framebox.c` | 30 | 0x00cfbb | 122 | whole segment | `-O -1 -P -d` | RELOC |
| `zevent/systimer.c` | 31 | 0x00d035 | 490 | whole segment | `-O -1 -P` | RELOC |
| `memsys.c` | 32 | 0x00d21f | 1208 | whole segment | `-O -1 -P -d` | RELOC |
| `emsalloc.asm` | 33 | 0x00d6d8 | 530 | whole segment | `/mx /m2` | RELOC |
| `farmem.c` | 34 | 0x00d8ea | 492 | whole segment | `-O -1 -P -d` | RELOC |
| `emsmem.c` | 35 | 0x00dad6 | 417 | whole segment | `-O -1 -P -d` | RELOC |
| `emsstat.asm` | 36 | 0x00dc78 | 356 | whole segment | `/mx /m2` | RELOC |
| `emsinit.asm` | 37 | 0x00dddc | 295 | whole segment | `/mx` | RELOC |
| `memrept.c` | 42 | 0x00e106 | 255 | whole segment | `-O -1 -P -d` | RELOC |
| `memsnap.c` | 43 | 0x00e205 | 282 | whole segment | `-O -1 -P -d` | RELOC |
| `nearmem.c` | 44 | 0x00e31f | 357 | whole segment | `-O -1 -P -d` | RELOC |
| `memhand.c` | 45 | 0x00e484 | 830 | whole segment | `-O -1 -P -d` | RELOC |
| `main/mem/memmgr.c` | 46 | 0x00e7c2 | 4405 | whole segment | `-O -1 -P` | RELOC |
| `cachenod.c` | 47 | 0x00f8f7 | 436 | whole segment | `-O -1 -P -d` | RELOC |
| `cachelst.c` | 48 | 0x00faab | 499 | whole segment | `-O -1 -P -d` | RELOC |
| `cache.c` | 49 | 0x00fc9e | 3273 | whole segment | `-O -1 -P -d` | RELOC |
| `reporter.c` | 50 | 0x010967 | 128 | whole segment | `-O -1 -P` | RELOC |
| `biostext.c` | 51 | 0x0109e7 | 447 | whole segment | `-O -1 -P` | RELOC |
| `shutdown.c` | 52 | 0x010ba6 | 136 | whole segment | `-O -1 -P` | RELOC |
| `dlist.c` | 53 | 0x010c2e | 638 | whole segment | `-O -1 -P` | RELOC |
| `fatal.c` | 54 | 0x010eac | 116 | whole segment | `-O -1 -P` | RELOC |
| `strops.c` | 55 | 0x010f20 | 459 | whole segment | `-O -1 -P` | RELOC |
| `message.c` | 56 | 0x0110eb | 49 | whole segment | `-O -1 -P` | RELOC |
| `strbuf.c` | 57 | 0x01111c | 243 | whole segment | `-O -1 -P` | RELOC |
| `fileio.c` | 59 | 0x0112a0 | 250 | whole segment | `-O -1 -P` | RELOC |
| `main/dos/doswrite.asm` | 64 | 0x011548 | 168 | whole segment | `/mx` | RELOC |
| `linefile.c` | 65 | 0x0115f0 | 364 | whole segment | `-O -1 -P` | RELOC |
| `dosattr.asm` | 66 | 0x01175c | 62 | whole segment | `/mx` | RELOC |
| `memfile.c` | 67 | 0x01179a | 1285 | whole segment | `-O -1 -P` | RELOC |
| `main/xmm/fillrect.asm` | 68 | 0x011ca0 | 143 | whole segment | `/mx` | RELOC |
| `ds1482.c` | 68 | 0x011d30 | 0 | whole segment | `-O -1 -P` | RELOC |
| `main/xmm/drawxlat.asm` | 70 | 0x011d7e | 790 | whole segment | `/mx` | RELOC |
| `outline.asm` | 72 | 0x01211e | 324 | whole segment | `/mx` | RELOC |
| `ds169a.c` | 72 | 0x012262 | 0 | whole segment | `-O -1 -P` | RELOC |
| `main/xmm/drawfram.asm` | 73 | 0x012262 | 647 | whole segment | `/mx` | RELOC |
| `AIL.ASM` | 74 | 0x0124ea | 3247 | whole segment | `/m /w+ /ml` | RELOC |
| `file.c` | 75 | 0x013199 | 2406 | whole segment | `-1 -G -P -vi-` | RELOC |
| `doshandl.c` | 76 | 0x013aff | 440 | whole segment | `-1 -G -P -vi-` | RELOC |
| `iff.c` | 77 | 0x013cb7 | 2996 | whole segment | `-1 -G -P -vi-` | RELOC |
| `flatflag.c` | 79 | 0x014900 | 0 | whole segment | `-O -1 -P` | RELOC |
| `xmsflat.asm` | 84 | 0x015366 | 99 | whole segment | `/mx` | RELOC |
| `flicbrun.asm` | 89 | 0x0158e6 | 79 | whole segment | `/mx` | RELOC |
| `flicdlta.asm` | 90 | 0x015936 | 94 | whole segment | `/mx` | RELOC |
| `flicclr.asm` | 91 | 0x015994 | 56 | whole segment | `/mx` | RELOC |
| `fatalerr.c` | 93 | 0x015b6a | 86 | whole segment | `-O -G -P` | RELOC |
