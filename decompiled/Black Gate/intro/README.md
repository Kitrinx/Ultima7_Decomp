# Recovered Black Gate routines

Each admitted source recompiles under Borland C++ 2.0 to the shipped bytes exactly.
Land a routine with `agents/tools/land.py`; check the whole corpus with
`agents/tools/verify_corpus.py`. RELOC means every relocated operand was also checked by name.
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
| `intro.c` | 1 | 0x005411 | 15902 | whole segment | `-O -1 -P -d` | RELOC |
| `unused.c` | 1 | 0x00922f | 7 | whole segment | `-O -1 -P -d` | RELOC |
| `fixed.asm` | 1 | 0x009238 | 353 | whole segment | `/mx` | RELOC |
| `static.asm` | 2 | 0x00939a | 159 | whole segment | `/mx` | RELOC |
| `synctrk.c` | 3 | 0x009439 | 702 | whole segment | `-O -1 -P` | RELOC |
| `main/controls.c` | 5 | 0x00988d | 3706 | whole segment | `-O -1 -P -d` | RELOC |
| `fadestep.c` | 6 | 0x00a753 | 702 | whole segment | `-O -1 -P` | RELOC |
| `textspr.c` | 7 | 0x00aa11 | 797 | whole segment | `-O -1 -P` | RELOC |
| `vecdump.c` | 9 | 0x00b098 | 141 | whole segment | `-O -1 -P` | RELOC |
| `shapebnd.c` | 10 | 0x00b125 | 165 | whole segment | `-O -1 -P` | RELOC |
| `common/easyfile.c` | 15 | 0x00bceb | 530 | whole segment | `-O -1 -P -d` | RELOC |
| `dirdelta.c` | 16 | 0x00bf61 | 0 | whole segment | `-O -1 -P` | RELOC |
| `common/flex.c` | 17 | 0x00bf61 | 1459 | whole segment | `-O -1 -P` | RELOC |
| `dlist.c` | 25 | 0x00cddc | 469 | whole segment | `-O -1 -P` | RELOC |
| `sound/drv/specard.c` | 28 | 0x00f2e5 | 2086 | whole segment | `-O -1 -P` | RELOC |
| `sound/specache.c` | 29 | 0x00fb0b | 1228 | whole segment | `-O -1 -P` | RELOC |
| `cfilcach.c` | 30 | 0x00ffd7 | 344 | whole segment | `-O -1 -P` | RELOC |
| `zevent/systimer.c` | 33 | 0x01020a | 735 | whole segment | `-O -1 -P` | RELOC |
| `main/chkfile.c` | 38 | 0x0106fd | 1561 | whole segment | `-O -G -P -b-` | RELOC |
| `main/mem/screen.c` | 41 | 0x010f39 | 524 | whole segment | `-O -G -P` | RELOC |
| `main/mem/drawbuf.c` | 42 | 0x011145 | 636 | whole segment | `-O -G` | RELOC |
| `vidpage.c` | 45 | 0x0115af | 303 | whole segment | `-O -G -zCVIDMODE_TEXT` | RELOC |
| `emsalloc.asm` | 48 | 0x011778 | 530 | whole segment | `/mx` | RELOC |
| `emsstat.asm` | 49 | 0x01198a | 356 | whole segment | `/mx` | RELOC |
| `main/mem/memapi.c` | 52 | 0x011d33 | 578 | whole segment | `-O -G` | RELOC |
| `main/xmm/drawfram.asm` | 62 | 0x01304a | 592 | whole segment | `/mx` | RELOC |
| `main/xmm/restfram.asm` | 63 | 0x01329a | 312 | whole segment | `/mx` | RELOC |
| `main/xmm/savefram.asm` | 64 | 0x0133d2 | 312 | whole segment | `/mx` | RELOC |
| `emsdraw.asm` | 65 | 0x01350a | 55 | whole segment | `/mx` | RELOC |
| `emsrest.asm` | 66 | 0x013542 | 61 | whole segment | `/mx` | RELOC |
| `main/xmm/restrect.asm` | 67 | 0x013580 | 230 | whole segment | `/mx` | RELOC |
| `emssave.asm` | 68 | 0x013666 | 61 | whole segment | `/mx` | RELOC |
| `usedcol.asm` | 71 | 0x01377c | 84 | whole segment | `/mx` | RELOC |
| `main/xmm/saverect.asm` | 72 | 0x0137d0 | 223 | whole segment | `/mx` | RELOC |
| `shrturn.asm` | 73 | 0x0138b0 | 4614 | whole segment | `/mx` | RELOC |
| `enlturn.asm` | 74 | 0x014ab6 | 11166 | whole segment | `/mx` | RELOC |
| `shrink.asm` | 75 | 0x017654 | 1518 | whole segment | `/mx` | RELOC |
| `enlarge.asm` | 76 | 0x017c42 | 2043 | whole segment | `/mx` | RELOC |
| `scalfram.asm` | 77 | 0x01843e | 4242 | whole segment | `/mx` | RELOC |
| `shrside.asm` | 78 | 0x0194d0 | 1538 | whole segment | `/mx` | RELOC |
| `enlside.asm` | 79 | 0x019ad2 | 1954 | whole segment | `/mx` | RELOC |
| `main/xmm/shapewid.c` | 80 | 0x01a274 | 86 | whole segment | `-O -G -P` | RELOC |
| `main/xmm/shapehgt.c` | 81 | 0x01a2ca | 86 | whole segment | `-O -G -P` | RELOC |
| `main/xmm/framecnt.asm` | 83 | 0x01a3a8 | 22 | whole segment | `/mx` | RELOC |
| `emswid.c` | 84 | 0x01a3be | 59 | whole segment | `-O -G -P` | RELOC |
| `emshgt.c` | 85 | 0x01a3f9 | 59 | whole segment | `-O -G -P` | RELOC |
| `emsfcnt.asm` | 86 | 0x01a434 | 42 | whole segment | `/mx` | RELOC |
| `main/video/farptrs.asm` | 86 | 0x01a45e | 0 | whole segment | `/mx` | RELOC |
