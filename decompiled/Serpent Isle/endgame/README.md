# Recovered Serpent Isle routines

Each admitted source recompiles under Borland C++ 2.0 to the shipped bytes exactly.
Land a routine with `agents/tools/land.py --target si_endgame`; check the whole corpus with
`agents/tools/verify_corpus.py --target si_endgame`. RELOC means every relocated operand was also checked by name.
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
| `flatmode.asm` | 0 | 0x002040 | 2801 | whole segment | `/mx` | RELOC |
| `endgame.c` | 1 | 0x005d8b | 9507 | whole segment | `-1 -G -P -vi-` | RELOC |
| `main/xmm/vooalloc.c` | 2 | 0x0082ae | 192 | whole segment | `-1 -G -P` | RELOC |
| `display.c` | 3 | 0x00836e | 538 | whole segment | `-1 -G -P -vi-` | RELOC |
| `main/video/flic.c` | 4 | 0x008588 | 1626 | whole segment | `-1 -G -P -vi-` | RELOC |
| `common/easyfile.c` | 5 | 0x008be2 | 673 | whole segment | `-1 -G -P -vi-` | RELOC |
| `common/strfmt.c` | 6 | 0x008e83 | 165 | whole segment | `-1 -G -P -vi-` | RELOC |
| `dirdelta.c` | 6 | 0x008f28 | 0 | whole segment | `-O -1 -P` | RELOC |
| `sound/sounddrv.c` | 7 | 0x008f28 | 2071 | whole segment | `-1 -G -P -vi-` | RELOC |
| `sound/xmidi.c` | 8 | 0x00973f | 1275 | whole segment | `-1 -G -P -vi-` | RELOC |
| `sound/digital.c` | 9 | 0x009c3a | 1123 | whole segment | `-1 -G -P -vi-` | RELOC |
| `sound/speech.c` | 9 | 0x00a09d | 1692 | whole segment | `-1 -G -P -vi-` | RELOC |
| `common/chunknam.c` | 10 | 0x00a739 | 242 | whole segment | `-1 -G -P -vi-` | RELOC |
| `sound/gsound.c` | 11 | 0x00a82b | 912 | whole segment | `-1 -G -P -vi- -d` | RELOC |
| `main/video/shapefnt.c` | 12 | 0x00abbb | 974 | whole segment | `-1 -G -P -vi-` | RELOC |
| `main/video/textwin.c` | 13 | 0x00af89 | 2163 | whole segment | `-1 -G -P -vi-` | RELOC |
| `main/endstats.c` | 13 | 0x00b7fc | 347 | whole segment | `-1 -G -P -vi- -d` | RELOC |
| `main/options.c` | 14 | 0x00b957 | 674 | whole segment | `-1 -G -P -vi- -d` | RELOC |
| `main/xmm/framebnd.asm` | 15 | 0x00bbfa | 136 | whole segment | `/mx` | RELOC |
| `main/video/crtport.asm` | 16 | 0x00bc82 | 20 | whole segment | `/mx` | RELOC |
| `main/video/videomd.c` | 17 | 0x00bc96 | 81 | whole segment | `-1` | RELOC |
| `vidrest.c` | 18 | 0x00bce7 | 14 | whole segment | `-O -1 -P` | RELOC |
| `main/video/vretrace.asm` | 19 | 0x00bcf6 | 15 | whole segment | `/mx` | RELOC |
| `rowtable.c` | 20 | 0x00bd05 | 145 | whole segment | `-O -1 -P` | RELOC |
| `main/video/view.c` | 21 | 0x00bd96 | 521 | whole segment | `-O -1 -P -d` | RELOC |
| `main/video/scrview.c` | 22 | 0x00bf9f | 82 | whole segment | `-O -1 -P -d` | RELOC |
| `main/palette/palstep.c` | 23 | 0x00bff1 | 352 | whole segment | `-O -1 -Z -P` | RELOC |
| `main/palette/dacwrite.asm` | 24 | 0x00c152 | 86 | whole segment | `/mx` | RELOC |
| `dacread.asm` | 25 | 0x00c1a8 | 107 | whole segment | `/mx` | RELOC |
| `box.c` | 25 | 0x00c213 | 0 | whole segment | `-O -1 -P -d` | RELOC |
| `main/palette/palette.c` | 26 | 0x00c213 | 1534 | whole segment | `-O -1 -P -d` | RELOC |
| `zevent/systimer.c` | 27 | 0x00c811 | 490 | whole segment | `-O -1 -P` | RELOC |
| `main/mem/memsys.c` | 28 | 0x00c9fb | 1208 | whole segment | `-O -1 -P -d` | RELOC |
| `emsalloc.asm` | 29 | 0x00ceb4 | 530 | whole segment | `/mx /m2` | RELOC |
| `main/mem/farmem.c` | 30 | 0x00d0c6 | 492 | whole segment | `-O -1 -P -d` | RELOC |
| `main/mem/emsmem.c` | 31 | 0x00d2b2 | 417 | whole segment | `-O -1 -P -d` | RELOC |
| `emsstat.asm` | 32 | 0x00d454 | 356 | whole segment | `/mx /m2` | RELOC |
| `emsinit.asm` | 33 | 0x00d5b8 | 295 | whole segment | `/mx` | RELOC |
| `emsmap.asm` | 34 | 0x00d6e0 | 267 | whole segment | `/mx` | RELOC |
| `main/dos/memmove.asm` | 35 | 0x00d7ec | 175 | whole segment | `/mx` | RELOC |
| `main/dos/ptraddr.asm` | 36 | 0x00d89c | 33 | whole segment | `/mx` | RELOC |
| `main/dos/addrptr.asm` | 37 | 0x00d8be | 36 | whole segment | `/mx` | RELOC |
| `main/mem/memrept.c` | 38 | 0x00d8e2 | 255 | whole segment | `-O -1 -P -d` | RELOC |
| `main/mem/memsnap.c` | 39 | 0x00d9e1 | 282 | whole segment | `-O -1 -P -d` | RELOC |
| `main/mem/nearmem.c` | 40 | 0x00dafb | 357 | whole segment | `-O -1 -P -d` | RELOC |
| `main/mem/memhand.c` | 41 | 0x00dc60 | 830 | whole segment | `-O -1 -P -d` | RELOC |
| `main/mem/memmgr.c` | 42 | 0x00df9e | 4405 | whole segment | `-O -1 -P` | RELOC |
| `main/mem/cachenod.c` | 43 | 0x00f0d3 | 436 | whole segment | `-O -1 -P -d` | RELOC |
| `main/mem/cachelst.c` | 44 | 0x00f287 | 499 | whole segment | `-O -1 -P -d` | RELOC |
| `main/mem/cache.c` | 45 | 0x00f47a | 3273 | whole segment | `-O -1 -P -d` | RELOC |
| `main/reporter.c` | 46 | 0x010143 | 128 | whole segment | `-O -1 -P` | RELOC |
| `biostext.c` | 47 | 0x0101c3 | 447 | whole segment | `-O -1 -P` | RELOC |
| `main/shutdown.c` | 48 | 0x010382 | 136 | whole segment | `-O -1 -P` | RELOC |
| `dlist.c` | 49 | 0x01040a | 638 | whole segment | `-O -1 -P` | RELOC |
| `main/fatal.c` | 50 | 0x010688 | 116 | whole segment | `-O -1 -P` | RELOC |
| `strops.c` | 51 | 0x0106fc | 459 | whole segment | `-O -1 -P` | RELOC |
| `main/message.c` | 52 | 0x0108c7 | 49 | whole segment | `-O -1 -P` | RELOC |
| `strbuf.c` | 53 | 0x0108f8 | 243 | whole segment | `-O -1 -P` | RELOC |
| `common/lstrtok.asm` | 54 | 0x0109ec | 144 | whole segment | `/mx` | RELOC |
| `common/fileio.c` | 55 | 0x010a7c | 250 | whole segment | `-O -1 -P` | RELOC |
| `main/dos/dosread.asm` | 56 | 0x010b76 | 253 | whole segment | `/mx` | RELOC |
| `main/dos/doscreat.asm` | 57 | 0x010c74 | 43 | whole segment | `/mx` | RELOC |
| `main/dos/dosfile.asm` | 58 | 0x010ca0 | 76 | whole segment | `/mx` | RELOC |
| `main/dos/dosseek.asm` | 59 | 0x010cec | 56 | whole segment | `/mx` | RELOC |
| `main/dos/doswrite.asm` | 60 | 0x010d24 | 168 | whole segment | `/mx` | RELOC |
| `common/linefile.c` | 61 | 0x010dcc | 364 | whole segment | `-O -1 -P` | RELOC |
| `dosattr.asm` | 62 | 0x010f38 | 62 | whole segment | `/mx` | RELOC |
| `common/memfile.c` | 63 | 0x010f76 | 1285 | whole segment | `-O -1 -P` | RELOC |
| `main/xmm/fillrect.asm` | 64 | 0x01147c | 143 | whole segment | `/mx` | RELOC |
| `ds1482.c` | 64 | 0x01150c | 0 | whole segment | `-O -1 -P` | RELOC |
| `main/xmm/clrview.asm` | 65 | 0x01150c | 77 | whole segment | `/mx` | RELOC |
| `main/xmm/drawxlat.asm` | 66 | 0x01155a | 790 | whole segment | `/mx` | RELOC |
| `main/xmm/copyview.asm` | 67 | 0x011870 | 138 | whole segment | `/mx` | RELOC |
| `main/xmm/drawfram.asm` | 68 | 0x0118fa | 647 | whole segment | `/mx` | RELOC |
| `ail.asm` | 69 | 0x011b82 | 3247 | whole segment | `/m /w+ /ml` | RELOC |
| `common/file.c` | 70 | 0x012831 | 2493 | whole segment | `-1 -G -P -vi-` | RELOC |
| `doshandl.c` | 71 | 0x0131ee | 440 | whole segment | `-1 -G -P -vi-` | RELOC |
| `common/iff.c` | 72 | 0x0133a6 | 2996 | whole segment | `-1 -G -P -vi-` | RELOC |
| `main/mem/memhook.c` | 73 | 0x013f5a | 65 | whole segment | `-O -G -P -d` | RELOC |
| `main/mem/a20gate.asm` | 74 | 0x013f9c | 82 | whole segment | `/mx` | RELOC |
| `main/xmm/linear.asm` | 75 | 0x013ff0 | 2060 | whole segment | `/mx` | RELOC |
| `flatflag.asm` | 76 | 0x0147fc | 0 | whole segment | `/mx` | RELOC |
| `main/xmm/xmmcheck.c` | 76 | 0x0147fc | 21 | whole segment | `-O -G` | RELOC |
| `main/xmm/freexmm.c` | 77 | 0x014812 | 219 | whole segment | `-O -G` | RELOC |
| `main/xmm/xmminit.c` | 78 | 0x0148ed | 296 | whole segment | `-O -G` | RELOC |
| `xmsflat.asm` | 79 | 0x014a16 | 99 | whole segment | `/mx` | RELOC |
| `main/xmm/xmmstale.c` | 80 | 0x014a79 | 63 | whole segment | `-O -G` | RELOC |
| `main/xmm/emscheck.asm` | 81 | 0x014ab8 | 52 | whole segment | `/mx` | RELOC |
| `main/xmm/xmmhand.c` | 82 | 0x014aec | 299 | whole segment | `-O -G` | RELOC |
| `main/xmm/xmmblock.asm` | 83 | 0x014c18 | 890 | whole segment | `/mx` | RELOC |
| `flicbrun.asm` | 84 | 0x014f92 | 79 | whole segment | `/mx` | RELOC |
| `main/video/flicdlta.asm` | 85 | 0x014fe2 | 94 | whole segment | `/mx` | RELOC |
| `flicclr.asm` | 86 | 0x015040 | 56 | whole segment | `/mx` | RELOC |
| `main/mem/errors.c` | 87 | 0x015078 | 414 | whole segment | `-O -G -P` | RELOC |
| `main/mem/fatalerr.c` | 88 | 0x015216 | 86 | whole segment | `-O -G -P` | RELOC |
