# Recovered Serpent Isle routines

Each admitted source recompiles under Borland C++ 2.0 to the shipped bytes exactly.
Land a routine with `agents/tools/land.py --target si_mainmenu`; check the whole corpus with
`agents/tools/verify_corpus.py --target si_mainmenu`. RELOC means every relocated operand was also checked by name.
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
| `flatmode.asm` | 0 | 0x002a40 | 2801 | whole segment | `/mx` | RELOC |
| `mainmenu.c` | 1 | 0x007e46 | 11604 | whole segment | `-O -1 -P -d -vi-` | RELOC |
| `credits.c` | 2 | 0x00ab9a | 3909 | whole segment | `-O -1 -P -d -vi-` | RELOC |
| `xmsflat.asm` | 3 | 0x00bae0 | 99 | whole segment | `/mx` | RELOC |
| `main/fontprn.c` | 4 | 0x00bb43 | 631 | whole segment | `-O -1 -P -vi-` | RELOC |
| `main/flexprn.c` | 5 | 0x00bdba | 630 | whole segment | `-O -1 -P -vi-` | RELOC |
| `menu.c` | 6 | 0x00c030 | 1576 | whole segment | `-O -1 -P -vi-` | RELOC |
| `menuent.c` | 7 | 0x00c658 | 273 | whole segment | `-O -1 -P -vi-` | RELOC |
| `main/controls.c` | 8 | 0x00c769 | 5257 | whole segment | `-O -1 -P -vi-` | RELOC |
| `zevent/u7point.c` | 9 | 0x00dbf2 | 884 | whole segment | `-O -1 -P -vi-` | RELOC |
| `textfld.c` | 10 | 0x00df66 | 730 | whole segment | `-O -1 -P -vi-` | RELOC |
| `textbox.c` | 11 | 0x00e240 | 477 | whole segment | `-O -1 -P -vi-` | RELOC |
| `main/device.c` | 12 | 0x00e41d | 582 | whole segment | `-O -1 -P` | RELOC |
| `sounddev.c` | 13 | 0x00e663 | 237 | whole segment | `-O -1 -P` | RELOC |
| `main/timerdev.c` | 14 | 0x00e750 | 53 | whole segment | `-O -1 -P` | RELOC |
| `keyqueue.c` | 15 | 0x00e785 | 520 | whole segment | `-O -1 -P` | RELOC |
| `main/textspr.c` | 16 | 0x00e98d | 1887 | whole segment | `-O -1 -P -vi-` | RELOC |
| `main/shapebnd.c` | 17 | 0x00f0ec | 345 | whole segment | `-O -1 -P -vi-` | RELOC |
| `main/scroll.c` | 18 | 0x00f245 | 1055 | whole segment | `-O -1 -P -vi-` | RELOC |
| `main/palette/palfade.asm` | 19 | 0x00f664 | 146 | whole segment | `/mx` | RELOC |
| `common/fadecol.c` | 19 | 0x00f6f6 | 187 | whole segment | `-O -1 -P -vi-` | RELOC |
| `setdac.asm` | 19 | 0x00f7b2 | 93 | whole segment | `/mx` | RELOC |
| `dosmem.c` | 20 | 0x00f80f | 165 | whole segment | `-O -1 -P` | RELOC |
| `main/install.c` | 21 | 0x00f8b4 | 1011 | whole segment | `-O -1 -P -vi-` | RELOC |
| `common/farbuf.c` | 22 | 0x00fca7 | 397 | whole segment | `-O -1 -P -vi-` | RELOC |
| `main/videomd.c` | 23 | 0x00fe34 | 76 | whole segment | `-O -1` | RELOC |
| `common/textfile.c` | 24 | 0x00fe80 | 865 | whole segment | `-O -1 -P -vi-` | RELOC |
| `common/rgbpal.c` | 25 | 0x0101e1 | 1560 | whole segment | `-O -1 -P -vi-` | RELOC |
| `common/fadepal.c` | 26 | 0x0107f9 | 407 | whole segment | `-O -1 -P -vi-` | RELOC |
| `fade.c` | 27 | 0x010990 | 642 | whole segment | `-O -1 -P` | RELOC |
| `common/easyfile.c` | 28 | 0x010c12 | 1102 | whole segment | `-O -1 -P -vi- -d` | RELOC |
| `dirdelta.c` | 29 | 0x011060 | 0 | whole segment | `-O -1 -P` | RELOC |
| `common/strfmt.c` | 29 | 0x011060 | 138 | whole segment | `-O -1 -P -vi- -d` | RELOC |
| `common/flex.c` | 30 | 0x0110ea | 1589 | whole segment | `-O -1 -P -vi-` | RELOC |
| `sound/gsound.c` | 31 | 0x01171f | 756 | whole segment | `-O -1 -P -vi- -d -b-` | RELOC |
| `zevent/dbgfont.c` | 31 | 0x011a13 | 392 | whole segment | `-O -1 -P` | RELOC |
| `zevent/mevent.c` | 32 | 0x011b9b | 763 | whole segment | `-O -1 -P -vi-` | RELOC |
| `zevent/mouse.c` | 33 | 0x011e96 | 855 | whole segment | `-O -1 -P -vi-` | RELOC |
| `zevent/cursor.c` | 34 | 0x0121ed | 0 | whole segment | `-O -1 -P -vi-` | RELOC |
| `zevent/mouseint.asm` | 34 | 0x0121ee | 198 | whole segment | `/mx` | RELOC |
| `zevent/getfont.asm` | 35 | 0x0122b4 | 22 | whole segment | `/mx` | RELOC |
| `zevent/systimer.c` | 36 | 0x0122ca | 737 | whole segment | `-O -1 -P -vi-` | RELOC |
| `zevent/sysclk.asm` | 37 | 0x0125ac | 96 | whole segment | `/mx` | RELOC |
| `main/itable.c` | 38 | 0x01260c | 319 | whole segment | `-O -1 -P -vi-` | RELOC |
| `common/lstrtok.asm` | 39 | 0x01274c | 144 | whole segment | `/mx` | RELOC |
| `common/dlist.c` | 40 | 0x0127dc | 694 | whole segment | `-O -1 -P` | RELOC |
| `inttrap.c` | 41 | 0x012a92 | 607 | whole segment | `-O -1 -P -vi-` | RELOC |
| `zevent/syskey.asm` | 42 | 0x012cf2 | 69 | whole segment | `/mx` | RELOC |
| `main/oops.c` | 43 | 0x012d37 | 100 | whole segment | `-O -1` | RELOC |
| `sound/drv/midiplay.c` | 44 | 0x012d9b | 7432 | whole segment | `-O -1 -P -d -vi- -y` | RELOC |
| `music.c` | 45 | 0x014aa3 | 1559 | whole segment | `-O -1 -P -vi-` | RELOC |
| `sound/cflxcach.c` | 46 | 0x0150ba | 575 | whole segment | `-O -1 -P -vi-` | RELOC |
| `sound/specache.c` | 47 | 0x0152f9 | 1321 | whole segment | `-O -1 -P -vi-` | RELOC |
| `sound/drv/specard.c` | 48 | 0x015822 | 2183 | whole segment | `-O -1 -P -vi-` | RELOC |
| `sound/drv/vocirq.asm` | 49 | 0x0160aa | 122 | whole segment | `/mx` | RELOC |
| `main/rmhook.c` | 50 | 0x016124 | 96 | whole segment | `-O -1` | RELOC |
| `main/mem/inthook.c` | 51 | 0x016184 | 144 | whole segment | `-O -G` | RELOC |
| `main/mem/memapi.c` | 52 | 0x016214 | 172 | whole segment | `-O -G` | RELOC |
| `main/mem/memmgr.c` | 53 | 0x0162c0 | 3292 | whole segment | `-O -G` | RELOC |
| `main/mem/memstat.c` | 54 | 0x016f9c | 316 | whole segment | `-O -G` | RELOC |
| `main/mem/memhook.c` | 55 | 0x0170d8 | 65 | whole segment | `-O -G -P -d` | RELOC |
| `main/mem/a20gate.asm` | 56 | 0x01711a | 82 | whole segment | `/mx` | RELOC |
| `main/mem/screen.c` | 57 | 0x01716c | 275 | whole segment | `-O -G -P` | RELOC |
| `main/mem/drawbuf.c` | 58 | 0x01727f | 351 | whole segment | `-O -G` | RELOC |
| `main/xmm/drawtile.asm` | 59 | 0x0173e0 | 161 | whole segment | `/mx` | RELOC |
| `main/xmm/rowaddr.asm` | 59 | 0x017484 | 122 | whole segment | `/mx` | RELOC |
| `main/xmm/linear.asm` | 59 | 0x017500 | 2125 | whole segment | `/mx` | RELOC |
| `main/xmm/copyview.asm` | 59 | 0x017d50 | 249 | whole segment | `/mx` | RELOC |
| `main/xmm/framebnd.asm` | 59 | 0x017e4c | 178 | whole segment | `/mx` | RELOC |
| `main/xmm/framecnt.asm` | 59 | 0x017f00 | 80 | whole segment | `/mx` | RELOC |
| `main/xmm/restfram.asm` | 59 | 0x017f50 | 444 | whole segment | `/mx` | RELOC |
| `main/xmm/restrect.asm` | 59 | 0x01810c | 357 | whole segment | `/mx` | RELOC |
| `main/xmm/savefram.asm` | 59 | 0x018274 | 469 | whole segment | `/mx` | RELOC |
| `main/xmm/saverect.asm` | 59 | 0x01844c | 352 | whole segment | `/mx` | RELOC |
| `main/xmm/drawfram.asm` | 59 | 0x0185ac | 1070 | whole segment | `/mx` | RELOC |
| `main/xmm/drawblnd.asm` | 59 | 0x0189dc | 1457 | whole segment | `/mx` | RELOC |
| `cpumode.asm` | 60 | 0x018f8e | 42 | whole segment | `/mx` | RELOC |
| `main/xmm/xmmcheck.c` | 61 | 0x018fb8 | 21 | whole segment | `-O -G` | RELOC |
| `main/xmm/freexmm.c` | 62 | 0x018fce | 219 | whole segment | `-O -G` | RELOC |
| `main/xmm/xmminit.c` | 63 | 0x0190a9 | 296 | whole segment | `-O -G` | RELOC |
| `main/xmm/shapehgt.c` | 64 | 0x0191d1 | 121 | whole segment | `-O -G -P` | RELOC |
| `main/xmm/shapewid.c` | 65 | 0x01924a | 121 | whole segment | `-O -G -P` | RELOC |
| `main/xmm/xmmstale.c` | 66 | 0x0192c3 | 63 | whole segment | `-O -G` | RELOC |
| `main/xmm/vooalloc.c` | 67 | 0x019302 | 189 | whole segment | `-O -G -P -d` | RELOC |
| `main/xmm/emscheck.asm` | 68 | 0x0193c0 | 52 | whole segment | `/mx` | RELOC |
| `main/xmm/xmmhand.c` | 69 | 0x0193f4 | 299 | whole segment | `-O -G` | RELOC |
| `main/xmm/xmmblock.asm` | 70 | 0x019520 | 893 | whole segment | `/mx` | RELOC |
| `main/video/vidmode.asm` | 71 | 0x01989e | 0 | whole segment | `/mx` | RELOC |
| `main/vidmode.c` | 71 | 0x01989e | 275 | whole segment | `-O -G` | RELOC |
| `main/video/crtport.asm` | 72 | 0x0199b2 | 20 | whole segment | `/mx` | RELOC |
| `main/video/vretrace.asm` | 73 | 0x0199c6 | 15 | whole segment | `/mx` | RELOC |
| `main/dos/ptraddr.asm` | 74 | 0x0199d6 | 33 | whole segment | `/mx` | RELOC |
| `main/dos/addrptr.asm` | 75 | 0x0199f8 | 36 | whole segment | `/mx` | RELOC |
| `main/dos/delay.asm` | 76 | 0x019a1c | 286 | whole segment | `/mx` | RELOC |
| `main/dos/memmove.asm` | 77 | 0x019b3a | 175 | whole segment | `/mx` | RELOC |
| `main/chkfile.c` | 78 | 0x019be9 | 1561 | whole segment | `-O -G -P -b-` | RELOC |
| `main/mem/errors.c` | 79 | 0x01a202 | 485 | whole segment | `-O -G -P` | RELOC |
| `main/video/colormap.c` | 80 | 0x01a3e7 | 62 | whole segment | `-O -G` | RELOC |
| `main/video/colorreg.c` | 81 | 0x01a425 | 124 | whole segment | `-O -G -P` | RELOC |
| `main/video/modecolr.c` | 82 | 0x01a4a1 | 95 | whole segment | `-O -G -P` | EXACT |
| `modebyte.c` | 83 | 0x01a500 | 10 | whole segment | `-O -1 -P` | RELOC |
| `emsinit.asm` | 83 | 0x01a50a | 313 | whole segment | `/mx` | RELOC |
| `emsmap.asm` | 84 | 0x01a644 | 267 | whole segment | `/mx` | RELOC |
| `main/dos/doswrite.asm` | 85 | 0x01a750 | 165 | whole segment | `/mx` | RELOC |
| `main/dos/dosread.asm` | 86 | 0x01a7f6 | 253 | whole segment | `/mx` | RELOC |
| `main/dos/dosfile.asm` | 87 | 0x01a8f4 | 76 | whole segment | `/mx` | RELOC |
| `main/dos/doscreat.asm` | 88 | 0x01a940 | 43 | whole segment | `/mx` | RELOC |
| `main/dos/fileread.asm` | 89 | 0x01a96c | 51 | whole segment | `/mx` | RELOC |
| `main/dos/filewrit.asm` | 90 | 0x01a9a0 | 51 | whole segment | `/mx` | RELOC |
| `main/dos/dosseek.asm` | 91 | 0x01a9d4 | 56 | whole segment | `/mx` | RELOC |
