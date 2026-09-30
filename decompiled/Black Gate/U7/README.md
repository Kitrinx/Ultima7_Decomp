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
| `runtime/deallocateNearMemory.c` | 0 | 0x0081b5 | 14 | C | `-O` | RELOC |
| `main/u7.c` | 1 | 0x00928c | 89 | whole segment | `-O -G` | RELOC |
| `main/action.c` | 2 | 0x0092e5 | 3431 | whole segment | `-O -G -P` | RELOC |
| `main/actqueue.c` | 3 | 0x00a04c | 3458 | whole segment | `-O -G -P` | EXACT |
| `main/combat/attack.c` | 4 | 0x00adce | 8155 | whole segment | `-O -G -P` | RELOC |
| `main/bltshape.c` | 5 | 0x00cda9 | 2907 | whole segment | `-O -G -P` | RELOC |
| `main/colbuf.c` | 6 | 0x00d904 | 2437 | whole segment | `-O -G -P -Y -b-` | EXACT |
| `main/itable.c` | 7 | 0x00e289 | 4233 | whole segment | `-O -G -P` | EXACT |
| `main/camera.c` | 8 | 0x00f312 | 2258 | whole segment | `-O -G -P` | RELOC |
| `sound/cflxcach.c` | 9 | 0x00fbe4 | 653 | whole segment | `-O -G -P` | EXACT |
| `main/chkfile.c` | 10 | 0x00fe71 | 1569 | whole segment | `-O -G -P -b-` | RELOC |
| `main/collgrid.asm` | 11 | 0x010492 | 405 | whole segment | `/mx` | RELOC |
| `main/collide.c` | 12 | 0x010627 | 5554 | whole segment | `-O -G -P` | RELOC |
| `main/combat/combstat.c` | 13 | 0x011bd9 | 942 | whole segment | `-O -G -P` | RELOC |
| `sound/cspeech.c` | 14 | 0x011f87 | 211 | whole segment | `-O -G -P` | EXACT |
| `zevent/dbgfont.c` | 15 | 0x01205a | 410 | whole segment | `-O -G -P` | RELOC |
| `main/debug.c` | 16 | 0x0121f4 | 855 | whole segment | `-O -G -P` | RELOC |
| `inter/debugint.c` | 17 | 0x01254b | 0 | whole segment | `-O -G` | RELOC |
| `main/endstats.c` | 18 | 0x01254b | 222 | whole segment | `-O -G -P` | RELOC |
| `main/frameflg.c` | 19 | 0x012629 | 722 | whole segment | `-O -G -P` | EXACT |
| `zevent/getfont.asm` | 20 | 0x0128fc | 22 | whole segment | `/mx` | RELOC |
| `inter/ucintrin.c` | 21 | 0x012912 | 0 | whole segment | `-O -G -P` | RELOC |
| `main/gtimer.c` | 22 | 0x012912 | 744 | whole segment | `-O -G -P` | EXACT |
| `main/itemcmd.c` | 23 | 0x012bfa | 4612 | whole segment | `-O -G -P` | RELOC |
| `main/keybios.asm` | 24 | 0x013dfe | 95 | whole segment | `/mx` | RELOC |
| `main/legalmov.c` | 25 | 0x013e5d | 3830 | whole segment | `-O -G -P` | RELOC |
| `main/dacpal.asm` | 27 | 0x014d54 | 199 | whole segment | `/mx` | RELOC |
| `common/depthbuf.asm` | 27 | 0x014e1c | 559 | whole segment | `/mx` | RELOC |
| `main/mem/palrange.asm` | 27 | 0x01504c | 419 | whole segment | `/mx` | RELOC |
| `main/mem/fadeprep.asm` | 27 | 0x0151f0 | 281 | whole segment | `/mx` | RELOC |
| `main/mem/fadestep.asm` | 27 | 0x01530c | 154 | whole segment | `/mx` | RELOC |
| `main/xmm/drawtile.asm` | 27 | 0x0153a8 | 317 | whole segment | `/mx` | RELOC |
| `main/xmm/rowaddr.asm` | 27 | 0x0154e8 | 122 | whole segment | `/mx` | RELOC |
| `main/xmm/linear.asm` | 27 | 0x015564 | 2445 | whole segment | `/mx` | RELOC |
| `main/xmm/copyview.asm` | 27 | 0x015ef4 | 249 | whole segment | `/mx` | RELOC |
| `main/xmm/frameadr.asm` | 27 | 0x015ff0 | 146 | whole segment | `/mx` | RELOC |
| `main/xmm/framebnd.asm` | 27 | 0x016084 | 178 | whole segment | `/mx` | RELOC |
| `main/xmm/framecnt.asm` | 27 | 0x016138 | 80 | whole segment | `/mx` | RELOC |
| `main/xmm/restfram.asm` | 27 | 0x016188 | 444 | whole segment | `/mx` | RELOC |
| `main/xmm/restrect.asm` | 27 | 0x016344 | 357 | whole segment | `/mx` | RELOC |
| `main/xmm/savefram.asm` | 27 | 0x0164ac | 469 | whole segment | `/mx` | RELOC |
| `main/xmm/saverect.asm` | 27 | 0x016684 | 352 | whole segment | `/mx` | RELOC |
| `main/xmm/movflat.asm` | 27 | 0x0167e4 | 182 | whole segment | `/mx` | RELOC |
| `main/xmm/drawfram.asm` | 27 | 0x01689c | 1070 | whole segment | `/mx` | RELOC |
| `main/xmm/drawmirr.asm` | 27 | 0x016ccc | 1103 | whole segment | `/mx` | RELOC |
| `main/xmm/mirrxlat.asm` | 27 | 0x01711c | 1371 | whole segment | `/mx` | RELOC |
| `main/xmm/mirrblnd.asm` | 27 | 0x017678 | 1726 | whole segment | `/mx` | RELOC |
| `main/xmm/drawblnd.asm` | 27 | 0x017d38 | 1457 | whole segment | `/mx` | RELOC |
| `main/xmm/drawxlat.asm` | 27 | 0x0182ec | 1142 | whole segment | `/mx` | RELOC |
| `main/xmm/encframe.asm` | 27 | 0x018764 | 858 | whole segment | `/mx` | RELOC |
| `main/xmm/remapvw.asm` | 27 | 0x018ac0 | 145 | whole segment | `/mx` | RELOC |
| `common/lstrtok.asm` | 28 | 0x018b52 | 144 | whole segment | `/mx` | RELOC |
| `main/lunch.c` | 29 | 0x018be2 | 510 | whole segment | `-O -G -P` | RELOC |
| `main/main.c` | 30 | 0x018de0 | 670 | whole segment | `-O -G -P` | RELOC |
| `main/mainctrl.c` | 31 | 0x01907e | 4027 | whole segment | `-O -G -P` | RELOC |
| `main/makemojo.c` | 32 | 0x01a039 | 607 | whole segment | `-O -G -P` | RELOC |
| `common/manager.c` | 33 | 0x01a298 | 460 | whole segment | `-O -G -P -d` | RELOC |
| `zevent/mevent.c` | 34 | 0x01a464 | 727 | whole segment | `-O -G -P` | RELOC |
| `zevent/mouse.c` | 35 | 0x01a73b | 902 | whole segment | `-O -G -P` | RELOC |
| `zevent/mouseint.asm` | 36 | 0x01aac2 | 198 | whole segment | `/mx` | EXACT |
| `main/movepath.c` | 37 | 0x01ab88 | 5064 | whole segment | `-O -G -P` | RELOC |
| `zevent/msclick.c` | 38 | 0x01bf50 | 964 | whole segment | `-O -G -P` | RELOC |
| `sound/multisnd.c` | 39 | 0x01c314 | 236 | whole segment | `-O -G -P` | RELOC |
| `main/oops.c` | 40 | 0x01c400 | 110 | whole segment | `-O -G` | RELOC |
| `main/partymov.c` | 41 | 0x01c46e | 9784 | whole segment | `-O -G -P` | EXACT |
| `main/placehld.c` | 42 | 0x01eaa6 | 0 | whole segment | `-O -G` | RELOC |
| `common/random.asm` | 43 | 0x01eaa6 | 115 | whole segment | `/mx` | RELOC |
| `main/rmhook.c` | 44 | 0x01eb19 | 97 | whole segment | `-O -G` | RELOC |
| `main/script.c` | 45 | 0x01eb7a | 127 | whole segment | `-O -G -P` | RELOC |
| `main/search.c` | 46 | 0x01ebf9 | 2315 | whole segment | `-O -G -P` | RELOC |
| `sound/sounds.c` | 47 | 0x01f504 | 2663 | whole segment | `-O -G -P` | RELOC |
| `sound/specache.c` | 48 | 0x01ff6b | 1341 | whole segment | `-O -G -P` | EXACT |
| `sound/drv/specard.c` | 49 | 0x0204a8 | 2287 | whole segment | `-O -G -P -Y` | RELOC |
| `main/speed.c` | 50 | 0x020d97 | 324 | whole segment | `-O -G -P` | RELOC |
| `zevent/sysclk.asm` | 51 | 0x020edc | 96 | whole segment | `/mx` | EXACT |
| `zevent/syskey.asm` | 52 | 0x020f3c | 69 | whole segment | `/mx` | EXACT |
| `zevent/systimer.c` | 53 | 0x020f81 | 805 | whole segment | `-O -G -P` | RELOC |
| `main/sysusage.c` | 54 | 0x0212a6 | 258 | whole segment | `-O -G -P` | RELOC |
| `main/u7coords.c` | 55 | 0x0213a8 | 64 | whole segment | `-O -G -P` | RELOC |
| `zevent/u7event.c` | 56 | 0x0213e8 | 2314 | whole segment | `-O -G -P` | RELOC |
| `zevent/u7point.c` | 57 | 0x021cf2 | 2273 | whole segment | `-O -G -P -d` | RELOC |
| `main/u7font.c` | 58 | 0x0225d3 | 349 | whole segment | `-O -G -P` | EXACT |
| `main/u7ibuf.c` | 59 | 0x022730 | 1877 | whole segment | `-O -G -P` | EXACT |
| `main/u7manage.c` | 60 | 0x022e85 | 7974 | whole segment | `-O -G -P -d -b-` | EXACT |
| `main/u7map.c` | 61 | 0x024dab | 564 | whole segment | `-O -G -P` | EXACT |
| `main/u7npc.c` | 62 | 0x024fdf | 3257 | whole segment | `-O -G -P` | EXACT |
| `main/u7reload.c` | 63 | 0x025c98 | 148 | whole segment | `-O -G -P` | RELOC |
| `main/nocando.c` | 64 | 0x025d2c | 173 | whole segment | `-O -G -P` | RELOC |
| `main/verify.c` | 65 | 0x025dd9 | 2885 | whole segment | `-O -G -P -Z` | RELOC |
| `main/videomd.c` | 66 | 0x02691e | 88 | whole segment | `-O -G` | RELOC |
| `sound/drv/vocirq.asm` | 67 | 0x026976 | 122 | whole segment | `/mx` | EXACT |
| `sound/voice.c` | 68 | 0x0269f0 | 610 | whole segment | `-O -G -P` | RELOC |
| `main/voonpc.c` | 69 | 0x026c52 | 694 | whole segment | `-O -G -d -P` | RELOC |
| `main/wihh.c` | 70 | 0x026f08 | 2293 | whole segment | `-O -G -P` | RELOC |
| `sound/u7sound.c` | 71 | 0x0277fd | 3159 | whole segment | `-O -G -P` | EXACT |
| `common/barge.c` | 72 | 0x028454 | 10350 | whole segment | `-O -G -P` | RELOC |
| `common/cacheent.c` | 73 | 0x02acc2 | 329 | whole segment | `-O -G -P` | RELOC |
| `common/chunk.c` | 74 | 0x02ae0b | 558 | whole segment | `-O -G -P` | RELOC |
| `common/coord.c` | 75 | 0x02b039 | 265 | whole segment | `-O -G` | RELOC |
| `common/cullmask.c` | 76 | 0x02b142 | 1528 | whole segment | `-O -G -P -b-` | RELOC |
| `common/datanode.c` | 77 | 0x02b73a | 209 | whole segment | `-O -G -P` | EXACT |
| `common/easyfile.c` | 78 | 0x02b80b | 1166 | whole segment | `-O -G -P -d` | RELOC |
| `common/egg.c` | 79 | 0x02bc99 | 86 | whole segment | `-O -G` | RELOC |
| `common/falloc.c` | 80 | 0x02bcef | 60 | whole segment | `-O -G` | RELOC |
| `common/typeflag.c` | 81 | 0x02bd2b | 0 | whole segment | `-O -G` | RELOC |
| `common/flxcach.c` | 82 | 0x02bd2b | 475 | whole segment | `-O -G -P` | EXACT |
| `common/flxwrite.c` | 83 | 0x02bf06 | 2835 | whole segment | `-O -G -P` | EXACT |
| `common/flex.c` | 84 | 0x02ca19 | 1626 | whole segment | `-O -G -P` | RELOC |
| `common/item.c` | 86 | 0x02d074 | 10580 | whole segment | `-O -G -P -Z` | RELOC |
| `common/mapview.c` | 87 | 0x02f9c8 | 5060 | whole segment | `-O -G -P -Z` | RELOC |
| `common/memfree.c` | 88 | 0x030d8c | 161 | whole segment | `-O -G -P` | RELOC |
| `common/memusage.c` | 89 | 0x030e2d | 139 | whole segment | `-O -G` | RELOC |
| `common/npcref.c` | 90 | 0x030eb8 | 4944 | whole segment | `-O -G -P` | RELOC |
| `common/randseed.c` | 91 | 0x032208 | 241 | whole segment | `-O -G -P` | EXACT |
| `common/maps.c` | 92 | 0x0322f9 | 756 | whole segment | `-O -G -P` | RELOC |
| `common/rescache.c` | 93 | 0x0325ed | 3835 | whole segment | `-O -G -P -d -Z` | RELOC |
| `common/typedim.c` | 94 | 0x0334e8 | 162 | whole segment | `-O -G` | RELOC |
| `common/strfmt.c` | 95 | 0x03358a | 107 | whole segment | `-O -G` | RELOC |
| `common/sortitem.c` | 96 | 0x0335f5 | 10177 | whole segment | `-O -G -P -Z` | EXACT |
| `common/xformtbl.c` | 97 | 0x035db6 | 475 | whole segment | `-O -G -P` | EXACT |
| `common/type.c` | 98 | 0x035f91 | 187 | whole segment | `-O -G` | RELOC |
| `common/fileutil.c` | 99 | 0x03604c | 1342 | whole segment | `-O -G -P -d` | RELOC |
| `common/bitarray.c` | 100 | 0x03658a | 254 | whole segment | `-O -G -P` | RELOC |
| `common/occlude.c` | 101 | 0x036688 | 258 | whole segment | `-O -G -P` | RELOC |
| `main/palette/paldata1.c` | 102 | 0x03678a | 0 | whole segment | `-O -G` | RELOC |
| `main/palette/palctrl.c` | 103 | 0x03678a | 171 | whole segment | `-O -G -P` | RELOC |
| `main/palette/crawpal.c` | 104 | 0x036835 | 914 | whole segment | `-O -G -P` | RELOC |
| `main/palette/rgbstep.c` | 105 | 0x036bc7 | 245 | whole segment | `-O -G -P` | RELOC |
| `main/palette/creeper.c` | 106 | 0x036cbc | 3250 | whole segment | `-O -G -P` | EXACT |
| `main/palette/crgbpal.c` | 107 | 0x03796e | 411 | whole segment | `-O -G -P` | EXACT |
| `main/palette/palfade.c` | 108 | 0x037b09 | 868 | whole segment | `-O -G -P` | RELOC |
| `main/palette/redscrn.c` | 109 | 0x037e6d | 874 | whole segment | `-O -G -P` | EXACT |
| `main/palette/redtimer.asm` | 110 | 0x0381d8 | 276 | whole segment | `/mx` | EXACT |
| `main/palette/paldata2.c` | 111 | 0x0382ec | 0 | whole segment | `-O -G` | RELOC |
| `main/palette/palnull1.c` | 112 | 0x0382ec | 0 | whole segment | `-O -G` | RELOC |
| `main/palette/palnull2.c` | 113 | 0x0382ec | 0 | whole segment | `-O -G` | RELOC |
| `main/palette/worldpal.c` | 114 | 0x0382ec | 5042 | whole segment | `-O -G -P` | RELOC |
| `inter/ucvalue.c` | 115 | 0x03969e | 136 | whole segment | `-O -G -P` | EXACT |
| `main/gumps.c` | 116 | 0x039726 | 2898 | whole segment | `-O -G -P` | EXACT |
| `main/spellini.c` | 117 | 0x03a278 | 860 | whole segment | `-O -G -P` | EXACT |
| `sound/drv/adlib.asm` | 118 | 0x03a5d4 | 5157 | whole segment | `/mx` | RELOC |
| `sound/drv/midiplay.c` | 119 | 0x03b9f9 | 8150 | whole segment | `-O -G -P -Y -Z -d` | EXACT |
| `main/dos/normptr.asm` | 120 | 0x03d9d0 | 56 | whole segment | `/mx` | RELOC |
| `main/dos/ptraddr.asm` | 121 | 0x03da08 | 33 | whole segment | `/mx` | RELOC |
| `main/dos/addrptr.asm` | 122 | 0x03da2a | 36 | whole segment | `/mx` | RELOC |
| `main/dos/delay.asm` | 123 | 0x03da4e | 286 | whole segment | `/mx` | EXACT |
| `main/dos/memfill.asm` | 124 | 0x03db6c | 49 | whole segment | `/mx` | RELOC |
| `main/dos/memmove.asm` | 125 | 0x03db9e | 175 | whole segment | `/mx` | RELOC |
| `main/mem/inthook.c` | 126 | 0x03dc4d | 144 | whole segment | `-O -G` | RELOC |
| `main/mem/errors.c` | 127 | 0x03dcdd | 414 | whole segment | `-O -G -P` | RELOC |
| `main/video/colormap.c` | 128 | 0x03de7b | 62 | whole segment | `-O -G` | RELOC |
| `main/dos/doswrite.asm` | 129 | 0x03deba | 165 | whole segment | `/mx` | RELOC |
| `main/dos/dosread.asm` | 130 | 0x03df60 | 253 | whole segment | `/mx` | RELOC |
| `main/dos/dosfile.asm` | 131 | 0x03e05e | 76 | whole segment | `/mx` | RELOC |
| `main/dos/doscreat.asm` | 132 | 0x03e0aa | 43 | whole segment | `/mx` | RELOC |
| `main/dos/fileread.asm` | 133 | 0x03e0d6 | 41 | whole segment | `/mx` | RELOC |
| `main/dos/filewrit.asm` | 134 | 0x03e100 | 41 | whole segment | `/mx` | RELOC |
| `main/dos/dosseek.asm` | 135 | 0x03e12a | 56 | whole segment | `/mx` | RELOC |
| `main/dos/dosnull.c` | 136 | 0x03e162 | 0 | whole segment | `-O -G` | RELOC |
| `main/mem/memapi.c` | 137 | 0x03e162 | 172 | whole segment | `-O -G` | RELOC |
| `main/mem/memmgr.c` | 138 | 0x03e20e | 3292 | whole segment | `-O -G` | RELOC |
| `main/mem/memstat.c` | 139 | 0x03eeea | 316 | whole segment | `-O -G` | RELOC |
| `main/dos/flatbits.asm` | 140 | 0x03f026 | 115 | whole segment | `/mx` | RELOC |
| `main/dos/shppack.asm` | 141 | 0x03f09a | 315 | whole segment | `/mx` | RELOC |
| `main/mem/memhook.c` | 142 | 0x03f1d5 | 65 | whole segment | `-O -G` | RELOC |
| `main/mem/a20gate.asm` | 143 | 0x03f216 | 82 | whole segment | `/mx` | RELOC |
| `main/mem/screen.c` | 144 | 0x03f268 | 275 | whole segment | `-O -G -P` | RELOC |
| `main/mem/drawbuf.c` | 145 | 0x03f37b | 351 | whole segment | `-O -G` | RELOC |
| `main/xmm/xmmcheck.c` | 149 | 0x03f4da | 21 | whole segment | `-O -G` | RELOC |
| `main/xmm/freexmm.c` | 154 | 0x03f4f0 | 219 | whole segment | `-O -G` | RELOC |
| `main/xmm/xmminit.c` | 156 | 0x03f5cc | 296 | whole segment | `-O -G` | RELOC |
| `main/xmm/xmsnull.asm` | 157 | 0x03f6f4 | 12 | whole segment | `/mx` | RELOC |
| `main/xmm/shapehgt.c` | 158 | 0x03f700 | 121 | whole segment | `-O -G -P` | RELOC |
| `main/xmm/shapewid.c` | 159 | 0x03f779 | 121 | whole segment | `-O -G -P` | RELOC |
| `main/xmm/xmmstale.c` | 160 | 0x03f7f2 | 63 | whole segment | `-O -G` | RELOC |
| `main/xmm/shapehit.c` | 161 | 0x03f831 | 84 | whole segment | `-O -G` | RELOC |
| `main/xmm/fillrect.asm` | 169 | 0x03f886 | 215 | whole segment | `/mx /m2` | RELOC |
| `main/xmm/drawline.asm` | 174 | 0x03f95e | 552 | whole segment | `/mx /m2` | RELOC |
| `main/xmm/vooalloc.c` | 177 | 0x03fb86 | 189 | whole segment | `-O -G` | RELOC |
| `main/xmm/emscheck.asm` | 180 | 0x03fc44 | 52 | whole segment | `/mx` | RELOC |
| `main/xmm/xmmhand.c` | 181 | 0x03fc78 | 299 | whole segment | `-O -G` | RELOC |
| `main/xmm/xmmblock.asm` | 182 | 0x03fda4 | 893 | whole segment | `/mx` | RELOC |
| `main/video/colorreg.c` | 183 | 0x040121 | 124 | whole segment | `-O -G -P` | RELOC |
| `main/video/modecolr.c` | 184 | 0x04019d | 95 | whole segment | `-O -G -P` | RELOC |
| `main/video/vidmode.c` | 185 | 0x0401fc | 275 | whole segment | `-O -G` | RELOC |
| `main/overlay/ovrdump.c` | 186 | 0x04030f | 389 | whole segment | `-O -G -P -vi-` | RELOC |
| `main/overlay/ovrprof.asm` | 187 | 0x040494 | 919 | whole segment | `/mx` | RELOC |
| `main/video/farptrs.asm` | 188 | 0x04082c | 0 | whole segment | `/mx` | EXACT |
| `main/video/crtport.asm` | 189 | 0x04082c | 20 | whole segment | `/mx` | RELOC |
| `main/video/vretrace.asm` | 190 | 0x040840 | 15 | whole segment | `/mx` | RELOC |
| `runtime/overlay.asm` | 191 | 0x04084f | 2269 | code unit | `/mx` | EXACT |
| `main/actitem.c` | 206 | 0x04eec0 | 6294 | whole segment | `-O -G -P` | RELOC |
| `main/actmove.c` | 207 | 0x0508f0 | 4318 | whole segment | `-O -G -P` | RELOC |
| `main/actutil.c` | 208 | 0x051af0 | 3219 | whole segment | `-O -G -P` | RELOC |
| `main/combat/ammo.c` | 209 | 0x052830 | 334 | whole segment | `-O -G -P -Y` | RELOC |
| `main/appts.c` | 210 | 0x0529a0 | 1276 | whole segment | `-O -G -P` | RELOC |
| `main/combat/armor.c` | 211 | 0x052f00 | 311 | whole segment | `-O -G -P -Y` | RELOC |
| `main/bark.c` | 212 | 0x053050 | 2429 | whole segment | `-O -G -P -Y` | RELOC |
| `main/bodies.c` | 213 | 0x053a20 | 201 | whole segment | `-O -G -P` | RELOC |
| `main/bogus.c` | 214 | 0x053af0 | 4029 | whole segment | `-O -G -P` | RELOC |
| `main/cast.c` | 215 | 0x054b60 | 2246 | whole segment | `-O -G -P -Y` | RELOC |
| `main/combat/cbattack.c` | 216 | 0x055470 | 6351 | whole segment | `-O -G -P -Y` | RELOC |
| `main/cheat.c` | 217 | 0x056e90 | 16166 | whole segment | `-O -G -P -d` | RELOC |
| `main/combat/combat.c` | 218 | 0x05b1e0 | 781 | whole segment | `-O -G -P` | RELOC |
| `main/combat/combatai.c` | 219 | 0x05b530 | 17704 | whole segment | `-O -G -P` | RELOC |
| `main/combat/combmode.c` | 220 | 0x05fea0 | 2145 | whole segment | `-O -G -P` | RELOC |
| `main/combat/combpick.c` | 221 | 0x060790 | 6509 | whole segment | `-O -G -P` | RELOC |
| `main/combat/combwpn.c` | 222 | 0x062290 | 5972 | whole segment | `-O -P -Y` | RELOC |
| `main/combat/crime.c` | 223 | 0x063af0 | 7863 | whole segment | `-O -G -P -Y` | EXACT |
| `main/combat/damage.c` | 224 | 0x065ba0 | 5635 | whole segment | `-O -G -P` | RELOC |
| `main/combat/daze.c` | 225 | 0x0672c0 | 3097 | whole segment | `-O -P` | RELOC |
| `common/dcache.c` | 226 | 0x067f90 | 590 | whole segment | `-O -G -P` | EXACT |
| `main/death.c` | 227 | 0x068200 | 6332 | whole segment | `-O -G -P` | RELOC |
| `main/dirpack.c` | 228 | 0x069c10 | 2380 | whole segment | `-O -G -P -d` | EXACT |
| `main/eggspawn.c` | 229 | 0x06a5e0 | 2502 | whole segment | `-O -G -P` | RELOC |
| `main/equip.c` | 230 | 0x06b010 | 2958 | whole segment | `-O -G -P` | RELOC |
| `main/combat/equipdat.c` | 231 | 0x06bc10 | 311 | whole segment | `-O -G -P -Y` | RELOC |
| `main/explode.c` | 232 | 0x06bd60 | 1924 | whole segment | `-O -G -P` | RELOC |
| `main/flexvoo.c` | 233 | 0x06c520 | 507 | whole segment | `-O -G -P` | RELOC |
| `main/getpick.c` | 234 | 0x06c740 | 435 | whole segment | `-O -G -P` | RELOC |
| `sound/gsound.c` | 235 | 0x06c910 | 806 | whole segment | `-O -G -P -d -b-` | RELOC |
| `main/init.c` | 236 | 0x06cc60 | 1353 | whole segment | `-O -G -P -Y` | RELOC |
| `main/initwp.c` | 237 | 0x06d240 | 1619 | whole segment | `-O -G -P` | EXACT |
| `common/itembuf.c` | 238 | 0x06d910 | 351 | whole segment | `-O -G -P` | RELOC |
| `common/itemovr1.c` | 239 | 0x06da80 | 3052 | whole segment | `-O -G -P -Z` | RELOC |
| `common/itemovr2.c` | 240 | 0x06e710 | 3069 | whole segment | `-O -G -P -Z` | RELOC |
| `inter/keywords.c` | 241 | 0x06f3d0 | 1026 | whole segment | `-O -P -Y` | EXACT |
| `common/loadreg.c` | 242 | 0x06f800 | 957 | whole segment | `-O -G -P -Z -d` | RELOC |
| `main/look.c` | 243 | 0x06fbf0 | 1131 | whole segment | `-O -G -P -d` | RELOC |
| `main/combat/missile.c` | 244 | 0x070090 | 5114 | whole segment | `-O -G -P -Y` | EXACT |
| `main/combat/misstrac.c` | 245 | 0x071510 | 4966 | whole segment | `-O -G -P` | RELOC |
| `main/combat/monsters.c` | 246 | 0x072920 | 367 | whole segment | `-O -G -P -Y` | RELOC |
| `main/npcpath.c` | 247 | 0x072ab0 | 8925 | whole segment | `-O -G -P -d -Y` | EXACT |
| `main/operate.c` | 248 | 0x074ec0 | 1406 | whole segment | `-O -G -P` | RELOC |
| `main/party.c` | 249 | 0x075460 | 3881 | whole segment | `-O -G -P -d -Y` | EXACT |
| `main/powder.c` | 250 | 0x076450 | 1951 | whole segment | `-O -G -P` | RELOC |
| `main/preload.c` | 251 | 0x076c60 | 2507 | whole segment | `-O -G -P -d -Y -b-` | RELOC |
| `main/combat/randstat.c` | 252 | 0x077710 | 3730 | whole segment | `-O -G -P` | RELOC |
| `main/combat/ready.c` | 253 | 0x0786a0 | 311 | whole segment | `-O -G -P -Y` | RELOC |
| `main/savegame.c` | 254 | 0x0787f0 | 1195 | whole segment | `-O -G -P -d` | EXACT |
| `common/sche.c` | 255 | 0x078d10 | 1211 | whole segment | `-O -G -P -Y` | RELOC |
| `common/sche_ov1.c` | 256 | 0x0791e0 | 784 | whole segment | `-O -G -P` | RELOC |
| `main/selweap.c` | 257 | 0x079520 | 4724 | whole segment | `-O -G -P` | RELOC |
| `main/slime.c` | 258 | 0x07a870 | 1400 | whole segment | `-O -G -P` | RELOC |
| `main/special.c` | 259 | 0x07ae10 | 715 | whole segment | `-O -G -P` | RELOC |
| `main/spell.c` | 260 | 0x07b100 | 1041 | whole segment | `-O -G -P` | RELOC |
| `main/sprite.c` | 261 | 0x07b520 | 2298 | whole segment | `-O -G -P -Y` | RELOC |
| `common/target.c` | 262 | 0x07be50 | 1258 | whole segment | `-O -G -P` | RELOC |
| `main/text.c` | 263 | 0x07c360 | 397 | whole segment | `-O -G -P -d -Y` | EXACT |
| `main/tools.c` | 264 | 0x07c510 | 1380 | whole segment | `-O -G -P` | RELOC |
| `main/trigger.c` | 265 | 0x07cac0 | 4248 | whole segment | `-O -G -P` | RELOC |
| `main/use.c` | 266 | 0x07dc20 | 3278 | whole segment | `-O -G -P` | RELOC |
| `main/vitem.c` | 267 | 0x07e980 | 2912 | whole segment | `-O -G -P -Y` | EXACT |
| `main/voolook.c` | 268 | 0x07f550 | 1671 | whole segment | `-O -G -P` | RELOC |
| `main/vstring.c` | 269 | 0x07fc30 | 1103 | whole segment | `-O -G -P -d -Y` | RELOC |
| `main/combat/weapons.c` | 270 | 0x0800b0 | 359 | whole segment | `-O -G -P -Y` | RELOC |
| `main/weather.c` | 271 | 0x080240 | 1990 | whole segment | `-O -G -P -Y` | EXACT |
| `common/weight.c` | 272 | 0x080a50 | 710 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheprac.c` | 273 | 0x080d30 | 556 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schetalk.c` | 274 | 0x080f80 | 692 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schedanc.c` | 275 | 0x081270 | 1611 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schefood.c` | 276 | 0x081940 | 739 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schefarm.c` | 277 | 0x081c60 | 1250 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schemine.c` | 278 | 0x0821a0 | 1927 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheloit.c` | 279 | 0x0829c0 | 1600 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schewand.c` | 280 | 0x083090 | 720 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheforg.c` | 281 | 0x0833a0 | 2279 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheslep.c` | 282 | 0x083d20 | 4640 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schekids.c` | 283 | 0x085070 | 455 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schealch.c` | 284 | 0x085260 | 958 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheflee.c` | 285 | 0x085670 | 500 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schepace.c` | 286 | 0x085890 | 1499 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheserv.c` | 287 | 0x085ef0 | 7507 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schewait.c` | 288 | 0x087de0 | 5 | whole segment | `-O -G` | RELOC |
| `main/schedule/schesew.c` | 289 | 0x087df0 | 2755 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schebake.c` | 290 | 0x088990 | 2558 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheeat.c` | 291 | 0x089450 | 704 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schethef.c` | 292 | 0x089740 | 1155 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheprch.c` | 293 | 0x089c20 | 1440 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schepatr.c` | 294 | 0x08a240 | 4796 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schedesk.c` | 295 | 0x08b680 | 948 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schesit.c` | 296 | 0x08ba70 | 731 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schespec.c` | 297 | 0x08bd90 | 5 | whole segment | `-O -G` | RELOC |
| `main/schedule/schegraz.c` | 298 | 0x08bda0 | 880 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheshop.c` | 299 | 0x08c160 | 1163 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schestnd.c` | 300 | 0x08c650 | 111 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schehoun.c` | 301 | 0x08c6d0 | 1543 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schefill.c` | 302 | 0x08cd50 | 1179 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schesit2.c` | 303 | 0x08d240 | 2496 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheread.c` | 304 | 0x08dc80 | 961 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheresp.c` | 305 | 0x08e070 | 661 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schewatr.c` | 306 | 0x08e340 | 884 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schearch.c` | 307 | 0x08e700 | 2703 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schefenc.c` | 308 | 0x08f260 | 551 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schehand.c` | 309 | 0x08f4b0 | 840 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schetag.c` | 310 | 0x08f840 | 1383 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schelght.c` | 311 | 0x08fe00 | 2482 | whole segment | `-O -G -P` | EXACT |
| `main/schedule/scheshut.c` | 312 | 0x090840 | 1582 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schearea.c` | 313 | 0x090ee0 | 867 | whole segment | `-O -G -P` | RELOC |
| `inter/flags.c` | 314 | 0x091290 | 1347 | whole segment | `-O -G -P -Y` | EXACT |
| `inter/usehook.c` | 315 | 0x091830 | 971 | whole segment | `-O -P -Y` | RELOC |
| `inter/uccomm1.c` | 316 | 0x091c50 | 3089 | whole segment | `-O -P` | RELOC |
| `inter/uccomm2.c` | 317 | 0x092960 | 1923 | whole segment | `-O -P` | RELOC |
| `inter/uccomm3.c` | 318 | 0x0931b0 | 1291 | whole segment | `-O -P -d` | RELOC |
| `inter/uccomm4.c` | 319 | 0x093750 | 2366 | whole segment | `-O -P -Y` | EXACT |
| `inter/uccomm5.c` | 320 | 0x094160 | 1262 | whole segment | `-O -P` | RELOC |
| `inter/ucstack.c` | 321 | 0x0946e0 | 895 | whole segment | `-O -G -P` | RELOC |
| `inter/uclist.c` | 322 | 0x094aa0 | 7147 | whole segment | `-O -P -Y` | EXACT |
| `inter/routine.c` | 323 | 0x0967d0 | 2579 | whole segment | `-O -P` | RELOC |
| `inter/inter.c` | 324 | 0x097250 | 3431 | whole segment | `-O -G -P` | RELOC |
| `inter/ucctrl.c` | 325 | 0x098140 | 86 | whole segment | `-O -G` | RELOC |
| `inter/farstr.c` | 326 | 0x0981a0 | 2208 | whole segment | `-O -G -P` | EXACT |
| `inter/uccomm6.c` | 327 | 0x098a80 | 1356 | whole segment | `-O -P` | RELOC |
| `inter/uccomm7.c` | 328 | 0x099040 | 2972 | whole segment | `-O -G -P` | EXACT |
| `inter/uccomm8.c` | 329 | 0x099ca0 | 2913 | whole segment | `-O -P` | RELOC |
| `inter/uccomm9.c` | 330 | 0x09a900 | 1055 | whole segment | `-O -P` | RELOC |
| `inter/uccomm10.c` | 331 | 0x09ad80 | 2478 | whole segment | `-O -P` | RELOC |
| `inter/uccomm11.c` | 332 | 0x09b7d0 | 595 | whole segment | `-O -P` | RELOC |
| `inter/uccomm12.c` | 333 | 0x09ba70 | 382 | whole segment | `-O -P` | RELOC |
| `inter/uccomm13.c` | 334 | 0x09bc10 | 989 | whole segment | `-O -P` | RELOC |
| `inter/uccomm14.c` | 335 | 0x09c040 | 1354 | whole segment | `-O -P` | RELOC |
| `main/convgump.c` | 336 | 0x09c600 | 3456 | whole segment | `-O -P -d` | EXACT |
| `main/convmgr.c` | 337 | 0x09d3f0 | 2890 | whole segment | `-O -P -d -Y` | RELOC |
| `main/gamegump.c` | 338 | 0x09dff0 | 2061 | whole segment | `-O -P` | RELOC |
| `main/controls.c` | 339 | 0x09e860 | 3217 | whole segment | `-O -P` | EXACT |
| `main/gumpmgr.c` | 340 | 0x09f590 | 7909 | whole segment | `-O -P -Y` | EXACT |
| `main/contgump.c` | 341 | 0x0a15f0 | 4158 | whole segment | `-O -P -Z -Y` | EXACT |
| `main/inv_ov2.c` | 342 | 0x0a26c0 | 8754 | whole segment | `-O -P -d -Y` | EXACT |
| `main/itemdrag.c` | 343 | 0x0a4a90 | 1312 | whole segment | `-O -P` | RELOC |
| `main/lsgump.c` | 344 | 0x0a4ff0 | 6086 | whole segment | `-O -P -Y` | EXACT |
| `main/shapegen.c` | 345 | 0x0a6900 | 622 | whole segment | `-O -P` | RELOC |
| `main/signgump.c` | 346 | 0x0a6b90 | 668 | whole segment | `-O -P` | EXACT |
| `main/slider.c` | 347 | 0x0a6e40 | 2145 | whole segment | `-O -P` | EXACT |
| `main/spellbk.c` | 348 | 0x0a7710 | 3280 | whole segment | `-O -P -d` | EXACT |
