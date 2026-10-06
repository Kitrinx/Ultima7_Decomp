# Recovered Serpent Isle routines

Each admitted source recompiles under Borland C++ 2.0 to the shipped bytes exactly.
Land a routine with `agents/tools/land.py --target si_intro`; check the whole corpus with
`agents/tools/verify_corpus.py --target si_intro`. RELOC means every relocated operand was also checked by name.
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
| `flatmode.asm` | 0 | 0x002240 | 2801 | whole segment | `/mx` | RELOC |
| `intro.c` | 1 | 0x005f8b | 12276 | whole segment | `-1 -G -P -vi-` | RELOC |
| `main/xmm/vooalloc.c` | 2 | 0x008f7f | 192 | whole segment | `-1 -G -P` | RELOC |
| `display.c` | 3 | 0x00903f | 538 | whole segment | `-1 -G -P -vi-` | RELOC |
| `main/video/flic.c` | 4 | 0x009259 | 1626 | whole segment | `-1 -G -P -vi-` | RELOC |
| `common/easyfile.c` | 5 | 0x0098b3 | 620 | whole segment | `-1 -G -P -vi-` | RELOC |
| `common/strfmt.c` | 6 | 0x009b1f | 165 | whole segment | `-1 -G -P -vi-` | RELOC |
| `dirdelta.c` | 6 | 0x009bc4 | 0 | whole segment | `-O -1 -P` | RELOC |
| `sound/sounddrv.c` | 7 | 0x009bc4 | 2071 | whole segment | `-1 -G -P -vi-` | RELOC |
| `sound/xmidi.c` | 8 | 0x00a3db | 1275 | whole segment | `-1 -G -P -vi-` | RELOC |
| `sound/digital.c` | 9 | 0x00a8d6 | 1123 | whole segment | `-1 -G -P -vi-` | RELOC |
| `sound/speech.c` | 10 | 0x00ad39 | 1692 | whole segment | `-1 -G -P -vi-` | RELOC |
| `common/chunknam.c` | 11 | 0x00b3d5 | 242 | whole segment | `-1 -G -P -vi-` | RELOC |
| `sound/gsound.c` | 12 | 0x00b4c7 | 912 | whole segment | `-1 -G -P -vi- -d` | RELOC |
| `main/video/shapefnt.c` | 13 | 0x00b857 | 974 | whole segment | `-1 -G -P -vi-` | RELOC |
| `main/video/textwin.c` | 14 | 0x00bc25 | 2163 | whole segment | `-1 -G -P -vi-` | RELOC |
| `main/endstats.c` | 14 | 0x00c498 | 347 | whole segment | `-1 -G -P -vi- -d` | RELOC |
| `main/options.c` | 15 | 0x00c5f3 | 674 | whole segment | `-1 -G -P -vi- -d` | RELOC |
| `main/xmm/framebnd.asm` | 16 | 0x00c896 | 136 | whole segment | `/mx` | RELOC |
| `main/video/crtport.asm` | 17 | 0x00c91e | 20 | whole segment | `/mx` | RELOC |
| `main/videomd.c` | 18 | 0x00c932 | 81 | whole segment | `-1` | RELOC |
| `vidrest.c` | 19 | 0x00c983 | 14 | whole segment | `-O -1 -P` | RELOC |
| `main/video/vretrace.asm` | 20 | 0x00c992 | 15 | whole segment | `/mx` | RELOC |
| `rowtable.c` | 21 | 0x00c9a1 | 145 | whole segment | `-O -1 -P` | RELOC |
| `main/video/view.c` | 22 | 0x00ca32 | 521 | whole segment | `-O -1 -P -d` | RELOC |
| `main/video/scrview.c` | 23 | 0x00cc3b | 82 | whole segment | `-O -1 -P -d` | RELOC |
| `main/palette/palstep.c` | 24 | 0x00cc8d | 352 | whole segment | `-O -1 -Z -P` | RELOC |
| `main/palette/dacwrite.asm` | 25 | 0x00cdee | 86 | whole segment | `/mx` | RELOC |
| `dacread.asm` | 26 | 0x00ce44 | 107 | whole segment | `/mx` | RELOC |
| `box.c` | 26 | 0x00ceaf | 0 | whole segment | `-O -1 -P -d` | RELOC |
| `main/palette/palette.c` | 27 | 0x00ceaf | 1534 | whole segment | `-O -1 -P -d` | RELOC |
| `zevent/systimer.c` | 28 | 0x00d4ad | 490 | whole segment | `-O -1 -P` | RELOC |
| `main/mem/memsys.c` | 29 | 0x00d697 | 1208 | whole segment | `-O -1 -P -d` | RELOC |
| `emsalloc.asm` | 30 | 0x00db50 | 530 | whole segment | `/mx /m2` | RELOC |
| `main/mem/farmem.c` | 31 | 0x00dd62 | 492 | whole segment | `-O -1 -P -d` | RELOC |
| `main/mem/emsmem.c` | 32 | 0x00df4e | 417 | whole segment | `-O -1 -P -d` | RELOC |
| `emsstat.asm` | 33 | 0x00e0f0 | 356 | whole segment | `/mx /m2` | RELOC |
| `emsinit.asm` | 34 | 0x00e254 | 295 | whole segment | `/mx` | RELOC |
| `emsmap.asm` | 35 | 0x00e37c | 267 | whole segment | `/mx` | RELOC |
| `main/dos/memmove.asm` | 36 | 0x00e488 | 175 | whole segment | `/mx` | RELOC |
| `main/dos/ptraddr.asm` | 37 | 0x00e538 | 33 | whole segment | `/mx` | RELOC |
| `main/dos/addrptr.asm` | 38 | 0x00e55a | 36 | whole segment | `/mx` | RELOC |
| `main/mem/memrept.c` | 39 | 0x00e57e | 255 | whole segment | `-O -1 -P -d` | RELOC |
| `main/mem/memsnap.c` | 40 | 0x00e67d | 282 | whole segment | `-O -1 -P -d` | RELOC |
| `main/mem/nearmem.c` | 41 | 0x00e797 | 357 | whole segment | `-O -1 -P -d` | RELOC |
| `main/mem/memhand.c` | 42 | 0x00e8fc | 830 | whole segment | `-O -1 -P -d` | RELOC |
| `main/mem/memmgr.c` | 43 | 0x00ec3a | 4405 | whole segment | `-O -1 -P` | RELOC |
| `main/mem/cachenod.c` | 44 | 0x00fd6f | 436 | whole segment | `-O -1 -P -d` | RELOC |
| `main/mem/cachelst.c` | 45 | 0x00ff23 | 499 | whole segment | `-O -1 -P -d` | RELOC |
| `main/mem/cache.c` | 46 | 0x010116 | 3273 | whole segment | `-O -1 -P -d` | RELOC |
| `main/reporter.c` | 47 | 0x010ddf | 128 | whole segment | `-O -1 -P` | RELOC |
| `biostext.c` | 48 | 0x010e5f | 447 | whole segment | `-O -1 -P` | RELOC |
| `main/shutdown.c` | 49 | 0x01101e | 136 | whole segment | `-O -1 -P` | RELOC |
| `dlist.c` | 50 | 0x0110a6 | 638 | whole segment | `-O -1 -P` | RELOC |
| `main/fatal.c` | 51 | 0x011324 | 116 | whole segment | `-O -1 -P` | RELOC |
| `strops.c` | 52 | 0x011398 | 459 | whole segment | `-O -1 -P` | RELOC |
| `main/message.c` | 53 | 0x011563 | 49 | whole segment | `-O -1 -P` | RELOC |
| `strbuf.c` | 54 | 0x011594 | 243 | whole segment | `-O -1 -P` | RELOC |
| `common/lstrtok.asm` | 55 | 0x011688 | 144 | whole segment | `/mx` | RELOC |
| `common/fileio.c` | 56 | 0x011718 | 250 | whole segment | `-O -1 -P` | RELOC |
| `main/dos/dosread.asm` | 57 | 0x011812 | 253 | whole segment | `/mx` | RELOC |
| `main/dos/doscreat.asm` | 58 | 0x011910 | 43 | whole segment | `/mx` | RELOC |
| `main/dos/dosfile.asm` | 59 | 0x01193c | 76 | whole segment | `/mx` | RELOC |
| `main/dos/dosseek.asm` | 60 | 0x011988 | 56 | whole segment | `/mx` | RELOC |
| `main/dos/doswrite.asm` | 61 | 0x0119c0 | 168 | whole segment | `/mx` | RELOC |
| `common/linefile.c` | 62 | 0x011a68 | 364 | whole segment | `-O -1 -P` | RELOC |
| `dosattr.asm` | 63 | 0x011bd4 | 62 | whole segment | `/mx` | RELOC |
| `common/memfile.c` | 64 | 0x011c12 | 1285 | whole segment | `-O -1 -P` | RELOC |
| `main/xmm/fillrect.asm` | 65 | 0x012118 | 143 | whole segment | `/mx` | RELOC |
| `ds1482.c` | 65 | 0x0121a8 | 0 | whole segment | `-O -1 -P` | RELOC |
| `main/xmm/clrview.asm` | 66 | 0x0121a8 | 77 | whole segment | `/mx` | RELOC |
| `main/xmm/drawxlat.asm` | 67 | 0x0121f6 | 790 | whole segment | `/mx` | RELOC |
| `main/xmm/copyview.asm` | 68 | 0x01250c | 138 | whole segment | `/mx` | RELOC |
| `main/xmm/drawfram.asm` | 69 | 0x012596 | 647 | whole segment | `/mx` | RELOC |
| `ail.asm` | 70 | 0x01281e | 3247 | whole segment | `/m /w+ /ml` | RELOC |
| `common/file.c` | 71 | 0x0134cd | 2493 | whole segment | `-1 -G -P -vi-` | RELOC |
| `doshandl.c` | 72 | 0x013e8a | 440 | whole segment | `-1 -G -P -vi-` | RELOC |
| `common/iff.c` | 73 | 0x014042 | 2996 | whole segment | `-1 -G -P -vi-` | RELOC |
| `main/mem/memhook.c` | 74 | 0x014bf6 | 65 | whole segment | `-O -G -P -d` | RELOC |
| `main/mem/a20gate.asm` | 75 | 0x014c38 | 82 | whole segment | `/mx` | RELOC |
| `main/xmm/linear.asm` | 76 | 0x014c8c | 2060 | whole segment | `/mx` | RELOC |
| `flatflag.asm` | 77 | 0x015498 | 0 | whole segment | `/mx` | RELOC |
| `main/xmm/xmmcheck.c` | 77 | 0x015498 | 21 | whole segment | `-O -G` | RELOC |
| `main/xmm/freexmm.c` | 78 | 0x0154ae | 219 | whole segment | `-O -G` | RELOC |
| `main/xmm/xmminit.c` | 79 | 0x015589 | 296 | whole segment | `-O -G` | RELOC |
| `xmsflat.asm` | 80 | 0x0156b2 | 99 | whole segment | `/mx` | RELOC |
| `main/xmm/xmmstale.c` | 81 | 0x015715 | 63 | whole segment | `-O -G` | RELOC |
| `main/xmm/emscheck.asm` | 82 | 0x015754 | 52 | whole segment | `/mx` | RELOC |
| `main/xmm/xmmhand.c` | 83 | 0x015788 | 299 | whole segment | `-O -G` | RELOC |
| `main/xmm/xmmblock.asm` | 84 | 0x0158b4 | 890 | whole segment | `/mx` | RELOC |
| `flicbrun.asm` | 85 | 0x015c2e | 79 | whole segment | `/mx` | RELOC |
| `main/video/flicdlta.asm` | 86 | 0x015c7e | 94 | whole segment | `/mx` | RELOC |
| `flicclr.asm` | 87 | 0x015cdc | 56 | whole segment | `/mx` | RELOC |
| `main/mem/errors.c` | 88 | 0x015d14 | 414 | whole segment | `-O -G -P` | RELOC |
| `main/mem/fatalerr.c` | 89 | 0x015eb2 | 86 | whole segment | `-O -G -P` | RELOC |
