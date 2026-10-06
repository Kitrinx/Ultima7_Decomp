# Recovered Serpent Isle routines

Each admitted source recompiles under Borland C++ 2.0 to the shipped bytes exactly.
Land a routine with `agents/tools/land.py --target si`; check the whole corpus with
`agents/tools/verify_corpus.py --target si`. RELOC means every relocated operand was also checked by name.
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
| `main/u7.c` | 1 | 0x00994f | 112 | whole segment | `-O -G -P` | RELOC |
| `main/action.c` | 2 | 0x0099bf | 3919 | whole segment | `-O -G -P` | RELOC |
| `main/actqueue.c` | 3 | 0x00a90e | 3559 | whole segment | `-O -G -P` | RELOC |
| `main/bltshape.c` | 4 | 0x00b6f5 | 2907 | whole segment | `-O -G -P` | RELOC |
| `main/colbuf.c` | 5 | 0x00c250 | 2482 | whole segment | `-O -G -P -Y -b-` | RELOC |
| `main/itable.c` | 6 | 0x00cc02 | 5946 | whole segment | `-O -G -P` | RELOC |
| `main/camera.c` | 7 | 0x00e33c | 2305 | whole segment | `-O -G -P` | EXACT |
| `main/chkfile.c` | 8 | 0x00ec3d | 1569 | whole segment | `-O -G -P -b-` | RELOC |
| `main/collgrid.asm` | 9 | 0x00f25e | 405 | whole segment | `/mx` | RELOC |
| `main/collide.c` | 10 | 0x00f3f3 | 5885 | whole segment | `-O -G -P` | RELOC |
| `main/debug.c` | 11 | 0x010af0 | 501 | whole segment | `-O -G -P` | RELOC |
| `inter/debugint.c` | 12 | 0x010ce5 | 0 | whole segment | `-O -G` | unrecorded |
| `main/endstats.c` | 13 | 0x010ce5 | 222 | whole segment | `-O -G -P` | RELOC |
| `main/frameflg.c` | 14 | 0x010dc3 | 722 | whole segment | `-O -G -P` | RELOC |
| `main/gtimer.c` | 15 | 0x011095 | 757 | whole segment | `-O -G -P` | RELOC |
| `main/itemcmd.c` | 16 | 0x01138a | 4810 | whole segment | `-O -G -P` | RELOC |
| `main/keybios.asm` | 17 | 0x012654 | 95 | whole segment | `/mx` | RELOC |
| `main/legalmov.c` | 18 | 0x0126b3 | 3830 | whole segment | `-O -G -P` | RELOC |
| `common/lstrtok.asm` | 19 | 0x0135aa | 144 | whole segment | `/mx` | RELOC |
| `main/lunch.c` | 20 | 0x01363a | 514 | whole segment | `-O -G -P` | RELOC |
| `main/main.c` | 21 | 0x01383c | 643 | whole segment | `-O -G -P` | EXACT |
| `main/mainctrl.c` | 22 | 0x013abf | 5155 | whole segment | `-O -G -P` | EXACT |
| `main/makemojo.c` | 23 | 0x014ee2 | 659 | whole segment | `-O -G -P` | RELOC |
| `main/movepath.c` | 24 | 0x015175 | 5431 | whole segment | `-O -G -P` | RELOC |
| `main/oops.c` | 25 | 0x0166ac | 110 | whole segment | `-O -G` | RELOC |
| `main/partymov.c` | 26 | 0x01671a | 9784 | whole segment | `-O -G -P` | RELOC |
| `main/rmhook.c` | 27 | 0x018d52 | 97 | whole segment | `-O -G` | RELOC |
| `main/script.c` | 28 | 0x018db3 | 127 | whole segment | `-O -G -P` | RELOC |
| `main/search.c` | 29 | 0x018e32 | 2298 | whole segment | `-O -G -P` | RELOC |
| `main/speed.c` | 30 | 0x01972c | 502 | whole segment | `-O -G -P` | RELOC |
| `zevent/syskey.asm` | 31 | 0x019922 | 69 | whole segment | `/mx` | RELOC |
| `main/sysusage.c` | 32 | 0x019967 | 258 | whole segment | `-O -G -P` | RELOC |
| `main/u7coords.c` | 33 | 0x019a69 | 57 | whole segment | `-O -G -P` | RELOC |
| `main/u7font.c` | 34 | 0x019aa2 | 349 | whole segment | `-O -G -P` | RELOC |
| `main/u7ibuf.c` | 35 | 0x019bff | 1877 | whole segment | `-O -G -P` | RELOC |
| `main/u7manage.c` | 36 | 0x01a354 | 8179 | whole segment | `-O -G -P -d -b-` | RELOC |
| `main/u7map.c` | 37 | 0x01c347 | 564 | whole segment | `-O -G -P` | RELOC |
| `main/u7npc2.c` | 38 | 0x01c57b | 3451 | whole segment | `-O -G -P` | EXACT |
| `main/u7reload.c` | 39 | 0x01d2f6 | 148 | whole segment | `-O -G -P` | RELOC |
| `main/nocando.c` | 40 | 0x01d38a | 170 | whole segment | `-O -G -P` | RELOC |
| `main/videomd.c` | 41 | 0x01d434 | 88 | whole segment | `-O -G` | RELOC |
| `main/voonpc.c` | 42 | 0x01d48c | 694 | whole segment | `-O -G -d -P` | RELOC |
| `main/wihh.c` | 43 | 0x01d742 | 2232 | whole segment | `-O -G -P` | RELOC |
| `common/barge.c` | 44 | 0x01dffa | 10302 | whole segment | `-O -G -P` | RELOC |
| `common/cacheent.c` | 45 | 0x020838 | 329 | whole segment | `-O -G -P` | RELOC |
| `common/chunk.c` | 46 | 0x020981 | 558 | whole segment | `-O -G -P` | RELOC |
| `common/coord.c` | 47 | 0x020baf | 265 | whole segment | `-O -G` | RELOC |
| `common/cullmask.c` | 48 | 0x020cb8 | 1528 | whole segment | `-O -G -P -b-` | RELOC |
| `common/datanode.c` | 49 | 0x0212b0 | 209 | whole segment | `-O -G -P` | RELOC |
| `common/easyfile.c` | 50 | 0x021381 | 1166 | whole segment | `-O -G -P -d` | RELOC |
| `common/egg.c` | 51 | 0x02180f | 159 | whole segment | `-O -G -Z` | RELOC |
| `common/falloc.c` | 52 | 0x0218ae | 60 | whole segment | `-O -G` | RELOC |
| `common/typeflag.c` | 53 | 0x0218ea | 0 | whole segment | `-O -G` | unrecorded |
| `common/flxcach.c` | 54 | 0x0218ea | 475 | whole segment | `-O -G -P` | RELOC |
| `common/flxwrite.c` | 55 | 0x021ac5 | 2835 | whole segment | `-O -G -P` | RELOC |
| `common/flex.c` | 56 | 0x0225d8 | 1626 | whole segment | `-O -G -P` | RELOC |
| `common/depthbuf.asm` | 58 | 0x022c34 | 559 | whole segment | `/mx` | RELOC |
| `main/mem/palrange.asm` | 58 | 0x022e64 | 419 | whole segment | `/mx` | RELOC |
| `main/mem/fadeprep.asm` | 58 | 0x023008 | 281 | whole segment | `/mx` | RELOC |
| `main/mem/fadestep.asm` | 58 | 0x023124 | 154 | whole segment | `/mx` | RELOC |
| `main/xmm/drawtile.asm` | 58 | 0x0231c0 | 317 | whole segment | `/mx` | RELOC |
| `main/xmm/rowaddr.asm` | 58 | 0x023300 | 122 | whole segment | `/mx` | RELOC |
| `main/xmm/linear.asm` | 58 | 0x02337c | 2445 | whole segment | `/mx` | RELOC |
| `main/xmm/copyview.asm` | 58 | 0x023d0c | 249 | whole segment | `/mx` | RELOC |
| `main/xmm/frameadr.asm` | 58 | 0x023e08 | 146 | whole segment | `/mx` | RELOC |
| `main/xmm/framebnd.asm` | 58 | 0x023e9c | 178 | whole segment | `/mx` | RELOC |
| `main/xmm/framecnt.asm` | 58 | 0x023f50 | 80 | whole segment | `/mx` | RELOC |
| `main/xmm/restfram.asm` | 58 | 0x023fa0 | 444 | whole segment | `/mx` | RELOC |
| `main/xmm/restrect.asm` | 58 | 0x02415c | 357 | whole segment | `/mx` | RELOC |
| `main/xmm/savefram.asm` | 58 | 0x0242c4 | 469 | whole segment | `/mx` | RELOC |
| `main/xmm/saverect.asm` | 58 | 0x02449c | 352 | whole segment | `/mx` | RELOC |
| `main/xmm/movflat.asm` | 58 | 0x0245fc | 182 | whole segment | `/mx` | RELOC |
| `main/xmm/drawfram.asm` | 58 | 0x0246b4 | 1070 | whole segment | `/mx` | RELOC |
| `main/xmm/drawmirr.asm` | 58 | 0x024ae4 | 1103 | whole segment | `/mx` | RELOC |
| `main/xmm/mirrxlat.asm` | 58 | 0x024f34 | 1371 | whole segment | `/mx` | RELOC |
| `main/xmm/mirrblnd.asm` | 58 | 0x025490 | 1726 | whole segment | `/mx` | RELOC |
| `main/xmm/drawblnd.asm` | 58 | 0x025b50 | 1457 | whole segment | `/mx` | RELOC |
| `main/xmm/drawxlat.asm` | 58 | 0x026104 | 1142 | whole segment | `/mx` | RELOC |
| `main/xmm/encframe.asm` | 58 | 0x02657c | 858 | whole segment | `/mx` | RELOC |
| `main/xmm/remapvw.asm` | 58 | 0x0268d8 | 145 | whole segment | `/mx` | RELOC |
| `common/item.c` | 59 | 0x026969 | 10580 | whole segment | `-O -G -P -Z` | RELOC |
| `common/loadreg.c` | 60 | 0x0292bd | 932 | whole segment | `-O -G -P -Z -d` | RELOC |
| `common/mapview.c` | 61 | 0x029661 | 5134 | whole segment | `-O -G -P -Z` | RELOC |
| `main/mem/memfree.c` | 62 | 0x02aa6f | 161 | whole segment | `-O -G -P` | RELOC |
| `common/memusage.c` | 63 | 0x02ab10 | 139 | whole segment | `-O -G` | RELOC |
| `common/npcref.c` | 64 | 0x02ab9b | 6323 | whole segment | `-O -G -P` | RELOC |
| `common/randseed.c` | 65 | 0x02c44e | 241 | whole segment | `-O -G -P` | RELOC |
| `common/maps.c` | 66 | 0x02c53f | 756 | whole segment | `-O -G -P` | RELOC |
| `common/rescache.c` | 67 | 0x02c833 | 3835 | whole segment | `-O -G -P -d -Z` | RELOC |
| `common/typedim.c` | 68 | 0x02d72e | 162 | whole segment | `-O -G` | RELOC |
| `common/strfmt.c` | 69 | 0x02d7d0 | 107 | whole segment | `-O -G` | RELOC |
| `common/sortitem.c` | 70 | 0x02d83b | 10135 | whole segment | `-O -G -P -Z` | RELOC |
| `common/xformtbl.c` | 71 | 0x02ffd2 | 475 | whole segment | `-O -G -P` | RELOC |
| `common/type.c` | 72 | 0x0301ad | 187 | whole segment | `-O -G` | RELOC |
| `common/fileutil.c` | 73 | 0x030268 | 1342 | whole segment | `-O -G -P -d` | RELOC |
| `common/bitarray.c` | 74 | 0x0307a6 | 254 | whole segment | `-O -G -P` | RELOC |
| `common/occlude.c` | 75 | 0x0308a4 | 258 | whole segment | `-O -G -P` | RELOC |
| `common/dcache.c` | 76 | 0x0309a6 | 590 | whole segment | `-O -G -P` | RELOC |
| `common/itembuf.c` | 77 | 0x030bf4 | 351 | whole segment | `-O -G -P` | RELOC |
| `common/itemovr1.c` | 78 | 0x030d53 | 3148 | whole segment | `-O -G -P -Z` | RELOC |
| `common/itemovr2.c` | 79 | 0x03199f | 3069 | whole segment | `-O -G -P -Z` | RELOC |
| `common/sche.c` | 80 | 0x03259c | 1211 | whole segment | `-O -G -P -Y` | RELOC |
| `common/sche_ov1.c` | 81 | 0x032a57 | 761 | whole segment | `-O -G -P` | RELOC |
| `common/target.c` | 82 | 0x032d50 | 1258 | whole segment | `-O -G -P` | RELOC |
| `sound/gsound.c` | 83 | 0x03323a | 841 | whole segment | `-O -G -P -d -b-` | RELOC |
| `common/manager.c` | 84 | 0x033583 | 460 | whole segment | `-O -G -P -d` | RELOC |
| `common/random.asm` | 85 | 0x033750 | 115 | whole segment | `/mx` | RELOC |
| `main/palette/paldata1.asm` | 86 | 0x0337c4 | 0 | whole segment | `/mx` | unrecorded |
| `main/palette/palctrl.c` | 87 | 0x0337c4 | 171 | whole segment | `-O -G -P` | RELOC |
| `main/palette/crawpal.c` | 88 | 0x03386f | 914 | whole segment | `-O -G -P` | RELOC |
| `main/palette/rgbstep.c` | 89 | 0x033c01 | 245 | whole segment | `-O -G -P` | RELOC |
| `main/palette/creeper.c` | 90 | 0x033cf6 | 3247 | whole segment | `-O -G -P` | RELOC |
| `main/palette/crgbpal.c` | 91 | 0x0349a5 | 411 | whole segment | `-O -G -P` | RELOC |
| `main/palette/palfade.c` | 92 | 0x034b40 | 879 | whole segment | `-O -G -P` | RELOC |
| `main/palette/redscrn.c` | 93 | 0x034eaf | 842 | whole segment | `-O -G -P` | RELOC |
| `main/palette/redtimer.asm` | 94 | 0x0351fa | 276 | whole segment | `/mx` | RELOC |
| `main/palette/paldata2.c` | 95 | 0x03530e | 0 | whole segment | `-O -G` | unrecorded |
| `main/palette/palnull1.c` | 96 | 0x03530e | 0 | whole segment | `-O -G` | unrecorded |
| `main/palette/palnull2.c` | 97 | 0x03530e | 0 | whole segment | `-O -G` | unrecorded |
| `main/palette/worldpal.c` | 98 | 0x03530e | 5058 | whole segment | `-O -G -P` | RELOC |
| `inter/keywords.c` | 99 | 0x0366d0 | 1026 | whole segment | `-O -P -Y` | RELOC |
| `inter/ucvalue.c` | 100 | 0x036ad2 | 136 | whole segment | `-O -G -P` | RELOC |
| `inter/ucintrin.c` | 101 | 0x036b5a | 0 | whole segment | `-O -G -P` | unrecorded |
| `main/combgump.c` | 102 | 0x036b5a | 889 | whole segment | `-O -G -P` | RELOC |
| `main/gumps.c` | 103 | 0x036ed3 | 3362 | whole segment | `-O -G -P` | EXACT |
| `main/spellini.c` | 104 | 0x037bf5 | 860 | whole segment | `-O -G -P` | EXACT |
| `main/jawgump.c` | 105 | 0x037f51 | 743 | whole segment | `-O -G -P` | EXACT |
| `main/scrlgump.c` | 106 | 0x038238 | 456 | whole segment | `-O -G -P` | EXACT |
| `main/sound/ail.asm` | 107 | 0x038400 | 6404 | whole segment | `/m /w+ /ml` | RELOC |
| `sound/sounds.c` | 108 | 0x039d04 | 2399 | whole segment | `-O -G -P` | EXACT |
| `sound/u7sound.c` | 109 | 0x03a663 | 3367 | whole segment | `-O -G -P -d -b-` | EXACT |
| `zevent/getfont.asm` | 110 | 0x03b38a | 22 | whole segment | `/mx` | RELOC |
| `zevent/mouseint.asm` | 111 | 0x03b3a0 | 198 | whole segment | `/mx` | RELOC |
| `zevent/dbgfont.c` | 112 | 0x03b466 | 410 | whole segment | `-O -G -P` | RELOC |
| `zevent/mevent.c` | 113 | 0x03b600 | 727 | whole segment | `-O -G -P` | RELOC |
| `zevent/mouse.c` | 114 | 0x03b8d7 | 902 | whole segment | `-O -G -P` | RELOC |
| `zevent/msclick.c` | 115 | 0x03bc5d | 964 | whole segment | `-O -G -P` | RELOC |
| `zevent/cursor.c` | 116 | 0x03c021 | 0 | whole segment | `-O -G` | unrecorded |
| `zevent/systimer.c` | 117 | 0x03c021 | 808 | whole segment | `-O -G -P -1 -Y` | RELOC |
| `zevent/u7point.c` | 118 | 0x03c349 | 2262 | whole segment | `-O -G -P -d` | EXACT |
| `zevent/u7event.c` | 119 | 0x03cc1f | 2349 | whole segment | `-O -G -P` | EXACT |
| `main/dos/normptr.asm` | 120 | 0x03d54c | 56 | whole segment | `/mx` | RELOC |
| `main/dos/ptraddr.asm` | 121 | 0x03d584 | 33 | whole segment | `/mx` | RELOC |
| `main/dos/addrptr.asm` | 122 | 0x03d5a6 | 36 | whole segment | `/mx` | RELOC |
| `main/dos/memfill.asm` | 123 | 0x03d5ca | 49 | whole segment | `/mx` | RELOC |
| `main/dos/memmove.asm` | 124 | 0x03d5fc | 175 | whole segment | `/mx` | RELOC |
| `main/mem/inthook.c` | 125 | 0x03d6ab | 144 | whole segment | `-O -G` | RELOC |
| `main/errors.c` | 126 | 0x03d73b | 414 | whole segment | `-O -G -P` | RELOC |
| `main/video/colormap.c` | 127 | 0x03d8d9 | 62 | whole segment | `-O -G` | RELOC |
| `main/dos/doswrite.asm` | 128 | 0x03d918 | 165 | whole segment | `/mx` | RELOC |
| `main/dos/dosread.asm` | 129 | 0x03d9be | 253 | whole segment | `/mx` | RELOC |
| `main/dos/dosfile.asm` | 130 | 0x03dabc | 76 | whole segment | `/mx` | RELOC |
| `main/dos/doscreat.asm` | 131 | 0x03db08 | 43 | whole segment | `/mx` | RELOC |
| `main/dos/fileread.asm` | 132 | 0x03db34 | 41 | whole segment | `/mx` | RELOC |
| `main/dos/filewrit.asm` | 133 | 0x03db5e | 41 | whole segment | `/mx` | RELOC |
| `main/dos/dosseek.asm` | 134 | 0x03db88 | 56 | whole segment | `/mx` | RELOC |
| `main/dos/dosnull.c` | 135 | 0x03dbc0 | 0 | whole segment | `-O -G` | unrecorded |
| `main/mem/memapi.c` | 136 | 0x03dbc0 | 172 | whole segment | `-O -G` | RELOC |
| `main/mem/memmgr.c` | 137 | 0x03dc6c | 3292 | whole segment | `-O -G` | RELOC |
| `main/mem/memstat.c` | 138 | 0x03e948 | 316 | whole segment | `-O -G` | RELOC |
| `main/dos/flatbits.asm` | 139 | 0x03ea84 | 115 | whole segment | `/mx` | RELOC |
| `main/dos/shppack.asm` | 140 | 0x03eaf8 | 315 | whole segment | `/mx` | RELOC |
| `main/mem/memhook.c` | 141 | 0x03ec33 | 65 | whole segment | `-O -G` | RELOC |
| `main/mem/a20gate.asm` | 142 | 0x03ec74 | 82 | whole segment | `/mx` | RELOC |
| `main/mem/screen.c` | 143 | 0x03ecc6 | 438 | whole segment | `-O -G -P -vi-` | RELOC |
| `main/mem/drawbuf.c` | 144 | 0x03ee7c | 486 | whole segment | `-O -G -P -vi-` | RELOC |
| `main/xmm/xmmcheck.c` | 148 | 0x03f062 | 21 | whole segment | `-O -G` | RELOC |
| `main/xmm/freexmm.c` | 153 | 0x03f078 | 219 | whole segment | `-O -G` | RELOC |
| `main/xmm/xmminit.c` | 155 | 0x03f154 | 338 | whole segment | `-O -G` | RELOC |
| `main/xmm/xmsnull.asm` | 156 | 0x03f2a6 | 12 | whole segment | `/mx` | RELOC |
| `main/xmm/shapehgt.c` | 157 | 0x03f2b2 | 290 | whole segment | `-O -G -P -vi-` | RELOC |
| `main/xmm/shapewid.c` | 158 | 0x03f3d4 | 152 | whole segment | `-O -G -P -vi-` | RELOC |
| `main/xmm/xmmstale.c` | 159 | 0x03f46c | 63 | whole segment | `-O -G` | RELOC |
| `main/xmm/shapehit.c` | 160 | 0x03f4ab | 84 | whole segment | `-O -G` | RELOC |
| `main/xmm/fillrect.asm` | 168 | 0x03f500 | 215 | whole segment | `/mx /m2` | RELOC |
| `main/xmm/drawline.asm` | 173 | 0x03f5d8 | 552 | whole segment | `/mx /m2` | RELOC |
| `main/xmm/vooalloc.c` | 176 | 0x03f800 | 189 | whole segment | `-O -G` | RELOC |
| `main/xmm/emscheck.asm` | 179 | 0x03f8be | 52 | whole segment | `/mx` | RELOC |
| `main/xmm/xmmhand.c` | 180 | 0x03f8f2 | 299 | whole segment | `-O -G` | RELOC |
| `main/xmm/xmmblock.asm` | 181 | 0x03fa1e | 893 | whole segment | `/mx` | RELOC |
| `main/video/colorreg.c` | 182 | 0x03fd9b | 124 | whole segment | `-O -G -P` | RELOC |
| `main/video/modecolr.c` | 183 | 0x03fe17 | 95 | whole segment | `-O -G -P` | RELOC |
| `main/video/vidmode.c` | 184 | 0x03fe76 | 275 | whole segment | `-O -G` | RELOC |
| `main/overlay/ovrdump.c` | 185 | 0x03ff89 | 389 | whole segment | `-O -G -P -vi-` | RELOC |
| `main/overlay/ovrprof.asm` | 186 | 0x04010e | 919 | whole segment | `/mx` | RELOC |
| `main/video/farptrs.asm` | 187 | 0x0404a6 | 0 | whole segment | `/mx` | unrecorded |
| `main/video/crtport.asm` | 188 | 0x0404a6 | 20 | whole segment | `/mx` | RELOC |
| `main/video/vretrace.asm` | 189 | 0x0404ba | 15 | whole segment | `/mx` | RELOC |
| `main/actitem.c` | 207 | 0x04e900 | 6705 | whole segment | `-O -G -P` | EXACT |
| `main/actmove.c` | 208 | 0x0504f0 | 4321 | whole segment | `-O -G -P` | RELOC |
| `main/actutil.c` | 209 | 0x0516f0 | 3246 | whole segment | `-O -G -P` | RELOC |
| `main/bark.c` | 210 | 0x052450 | 2252 | whole segment | `-O -G -P -Y` | RELOC |
| `main/bodies.c` | 211 | 0x052d70 | 206 | whole segment | `-O -G -P` | RELOC |
| `main/bogus.c` | 212 | 0x052e50 | 4669 | whole segment | `-O -G -P` | RELOC |
| `main/cast.c` | 213 | 0x054160 | 2464 | whole segment | `-O -G -P -Y` | RELOC |
| `main/cheat.c` | 214 | 0x054b60 | 21431 | whole segment | `-O -G -P -d` | EXACT |
| `main/cheat_ov.c` | 215 | 0x05a570 | 2688 | whole segment | `-O -G -P -d` | RELOC |
| `main/chngsche.c` | 216 | 0x05b0d0 | 839 | whole segment | `-O -G -P -d -Y` | RELOC |
| `main/dirpack.c` | 217 | 0x05b450 | 2380 | whole segment | `-O -G -P -d` | RELOC |
| `main/eggspawn.c` | 218 | 0x05be20 | 2514 | whole segment | `-O -G -P` | RELOC |
| `main/equip.c` | 219 | 0x05c860 | 3753 | whole segment | `-O -G -P` | RELOC |
| `main/combat/equipdat.c` | 220 | 0x05d790 | 311 | whole segment | `-O -G -P -Y` | RELOC |
| `main/explode.c` | 221 | 0x05d8e0 | 1924 | whole segment | `-O -G -P` | RELOC |
| `main/flexvoo.c` | 222 | 0x05e0a0 | 507 | whole segment | `-O -G -P` | RELOC |
| `main/getpick.c` | 223 | 0x05e2c0 | 461 | whole segment | `-O -G -P` | RELOC |
| `main/init.c` | 224 | 0x05e4b0 | 1471 | whole segment | `-O -G -P -Y` | RELOC |
| `main/initwp.c` | 225 | 0x05eb10 | 1822 | whole segment | `-O -G -P -d` | EXACT |
| `main/keyring.c` | 226 | 0x05f2d0 | 585 | whole segment | `-O -G -P -d -Y` | RELOC |
| `main/look.c` | 227 | 0x05f530 | 6797 | whole segment | `-O -G -P -d` | RELOC |
| `main/lookat.c` | 228 | 0x0610e0 | 201 | whole segment | `-O -G -P -d` | RELOC |
| `main/npcpath.c` | 229 | 0x0611c0 | 8931 | whole segment | `-O -G -P -d -Y` | RELOC |
| `main/operate.c` | 230 | 0x0635d0 | 1406 | whole segment | `-O -G -P` | RELOC |
| `main/party.c` | 231 | 0x063b70 | 6464 | whole segment | `-O -G -P -d -Y` | RELOC |
| `main/philbert.c` | 232 | 0x0655d0 | 405 | whole segment | `-O -G -P -Y` | EXACT |
| `main/powder.c` | 233 | 0x065790 | 1951 | whole segment | `-O -G -P` | RELOC |
| `main/polymorp.c` | 234 | 0x065fa0 | 1028 | whole segment | `-O -G -P -d -Y` | RELOC |
| `main/preload.c` | 235 | 0x0663e0 | 2850 | whole segment | `-O -G -P -d -Y -b-` | EXACT |
| `main/ovlnull.c` | 236 | 0x067020 | 0 | whole segment | `-O -G` | unrecorded |
| `main/savegame.c` | 237 | 0x067030 | 1189 | whole segment | `-O -G -P -d` | RELOC |
| `main/selweap.c` | 238 | 0x067550 | 4767 | whole segment | `-O -G -P` | RELOC |
| `main/slime.c` | 239 | 0x0688d0 | 1400 | whole segment | `-O -G -P` | RELOC |
| `main/special.c` | 240 | 0x068e70 | 715 | whole segment | `-O -G -P` | RELOC |
| `main/spell.c` | 241 | 0x069160 | 1021 | whole segment | `-O -G -P` | RELOC |
| `main/sprite.c` | 242 | 0x069570 | 1920 | whole segment | `-O -G -P -Y` | RELOC |
| `main/sprite2.c` | 243 | 0x069d20 | 372 | whole segment | `-O -G -P` | RELOC |
| `main/text.c` | 244 | 0x069ea0 | 401 | whole segment | `-O -G -P -d -Y` | RELOC |
| `main/tools.c` | 245 | 0x06a050 | 1546 | whole segment | `-O -G -P` | RELOC |
| `main/trigger2.c` | 246 | 0x06a6b0 | 2970 | whole segment | `-O -G -P` | RELOC |
| `main/trigger.c` | 247 | 0x06b2e0 | 1394 | whole segment | `-O -G -P` | RELOC |
| `main/u7npc.c` | 248 | 0x06b890 | 1163 | whole segment | `-O -G -P -d` | RELOC |
| `main/use.c` | 249 | 0x06bd80 | 3263 | whole segment | `-O -G -P` | RELOC |
| `main/vitem.c` | 250 | 0x06cad0 | 2912 | whole segment | `-O -G -P -Y` | RELOC |
| `main/voolook.c` | 251 | 0x06d6a0 | 1671 | whole segment | `-O -G -P` | RELOC |
| `main/vstring.c` | 252 | 0x06dd80 | 1103 | whole segment | `-O -G -P -d -Y` | RELOC |
| `main/warmth.c` | 253 | 0x06e200 | 847 | whole segment | `-O -G -P -Y` | RELOC |
| `main/weather.c` | 254 | 0x06e570 | 1990 | whole segment | `-O -G -P -Y` | RELOC |
| `common/weight.c` | 255 | 0x06ed80 | 728 | whole segment | `-O -G -P` | RELOC |
| `common/schedit.c` | 256 | 0x06f070 | 883 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheprac.c` | 257 | 0x06f410 | 565 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schetalk.c` | 258 | 0x06f660 | 600 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schedanc.c` | 259 | 0x06f8f0 | 1827 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schefood.c` | 260 | 0x0700a0 | 748 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schefarm.c` | 261 | 0x0703c0 | 1228 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schemine.c` | 262 | 0x0708f0 | 5 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheloit.c` | 263 | 0x070900 | 1557 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schewand.c` | 264 | 0x070fa0 | 726 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheforg.c` | 265 | 0x0712c0 | 2388 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheslep.c` | 266 | 0x071cb0 | 5009 | whole segment | `-O -G -P` | EXACT |
| `main/schedule/schekids.c` | 267 | 0x073190 | 464 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schealch.c` | 268 | 0x073390 | 976 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheflee.c` | 269 | 0x0737b0 | 503 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schepace.c` | 270 | 0x0739d0 | 1704 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheserv.c` | 271 | 0x074100 | 7541 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schewait.c` | 272 | 0x076020 | 5 | whole segment | `-O -G` | RELOC |
| `main/schedule/schesew.c` | 273 | 0x076030 | 2806 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schebake.c` | 274 | 0x076c00 | 2777 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheeat.c` | 275 | 0x0777a0 | 699 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schethef.c` | 276 | 0x077a90 | 1158 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheprch.c` | 277 | 0x077f70 | 1427 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schepatr.c` | 278 | 0x078580 | 5178 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schedesk.c` | 279 | 0x079b60 | 914 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schesit.c` | 280 | 0x079f30 | 743 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schepath.c` | 281 | 0x07a260 | 3112 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schegraz.c` | 282 | 0x07af50 | 877 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheshop.c` | 283 | 0x07b310 | 1147 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schestnd.c` | 284 | 0x07b7f0 | 114 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schehoun.c` | 285 | 0x07b870 | 1662 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schefill.c` | 286 | 0x07bf70 | 1185 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schesit2.c` | 287 | 0x07c460 | 2496 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheread.c` | 288 | 0x07cea0 | 964 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheresp.c` | 289 | 0x07d290 | 670 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schewatr.c` | 290 | 0x07d570 | 893 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schearch.c` | 291 | 0x07d930 | 2709 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schefenc.c` | 292 | 0x07e490 | 557 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schehand.c` | 293 | 0x07e6f0 | 843 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schetag.c` | 294 | 0x07ea80 | 1398 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schelght.c` | 295 | 0x07f050 | 2554 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/scheshut.c` | 296 | 0x07fae0 | 2392 | whole segment | `-O -G -P` | RELOC |
| `main/schedule/schearea.c` | 297 | 0x0804b0 | 873 | whole segment | `-O -G -P` | RELOC |
| `inter/flags.c` | 298 | 0x080870 | 1347 | whole segment | `-O -G -P -Y` | RELOC |
| `inter/usehook.c` | 299 | 0x080e10 | 1003 | whole segment | `-O -P -Y` | EXACT |
| `inter/ucstack.c` | 300 | 0x081250 | 895 | whole segment | `-O -G -P` | RELOC |
| `inter/uclist.c` | 301 | 0x081610 | 7147 | whole segment | `-O -P -Y` | RELOC |
| `inter/routine.c` | 302 | 0x083340 | 2579 | whole segment | `-O -P` | RELOC |
| `inter/inter.c` | 303 | 0x083dc0 | 3489 | whole segment | `-O -G -P` | EXACT |
| `inter/ucctrl.c` | 304 | 0x084cf0 | 86 | whole segment | `-O -G` | RELOC |
| `inter/farstr.c` | 305 | 0x084d50 | 2243 | whole segment | `-O -G -P` | RELOC |
| `main/console.c` | 306 | 0x085660 | 731 | whole segment | `-O -G -P -d` | RELOC |
| `inter/uccomm6.c` | 307 | 0x085980 | 3155 | whole segment | `-O -P` | RELOC |
| `inter/uccomm7.c` | 308 | 0x0866e0 | 2321 | whole segment | `-O -G -P` | EXACT |
| `inter/uccomm8.c` | 309 | 0x0870a0 | 3236 | whole segment | `-O -P` | RELOC |
| `inter/uccomm9.c` | 310 | 0x087e50 | 1108 | whole segment | `-O -P` | RELOC |
| `inter/uccomm10.c` | 311 | 0x088310 | 4849 | whole segment | `-O -P` | RELOC |
| `inter/uccomm11.c` | 312 | 0x089750 | 1403 | whole segment | `-O -P` | RELOC |
| `inter/uccomm12.c` | 313 | 0x089d60 | 1219 | whole segment | `-O -P` | RELOC |
| `inter/uccomm13.c` | 314 | 0x08a280 | 1023 | whole segment | `-O -P` | RELOC |
| `inter/uccomm14.c` | 315 | 0x08a6d0 | 1531 | whole segment | `-O -P` | RELOC |
| `inter/uccomm1.c` | 316 | 0x08ad50 | 3400 | whole segment | `-O -P` | RELOC |
| `inter/uccomm2.c` | 317 | 0x08bba0 | 2597 | whole segment | `-O -P` | RELOC |
| `inter/uccomm3.c` | 318 | 0x08c6b0 | 2657 | whole segment | `-O -P -d` | RELOC |
| `inter/uccomm4.c` | 319 | 0x08d230 | 2485 | whole segment | `-O -P -Y` | RELOC |
| `inter/uccomm5.c` | 320 | 0x08dcc0 | 2586 | whole segment | `-O -P` | RELOC |
| `main/combg_ov.c` | 321 | 0x08e7e0 | 4232 | whole segment | `-O -P -Y` | RELOC |
| `main/convgump.c` | 322 | 0x08f910 | 3543 | whole segment | `-O -P -d` | RELOC |
| `main/convmgr.c` | 323 | 0x090770 | 3785 | whole segment | `-O -P -d -Y` | EXACT |
| `main/gamegump.c` | 324 | 0x091710 | 2015 | whole segment | `-O -P` | RELOC |
| `main/controls.c` | 325 | 0x091f50 | 3467 | whole segment | `-O -P` | RELOC |
| `main/gumpmgr.c` | 326 | 0x092d80 | 4980 | whole segment | `-O -P -Y` | EXACT |
| `main/gumpmgr2.c` | 327 | 0x0941e0 | 2508 | whole segment | `-O -P -Y` | RELOC |
| `main/gumpmgr3.c` | 328 | 0x094c30 | 686 | whole segment | `-O -P -Y` | RELOC |
| `main/contgump.c` | 329 | 0x094f10 | 4185 | whole segment | `-O -P -Z -Y` | RELOC |
| `main/itemdrag.c` | 331 | 0x09a1d0 | 1312 | whole segment | `-O -P` | RELOC |
| `main/jawg_ov.c` | 332 | 0x09a730 | 2682 | whole segment | `-O -P` | RELOC |
| `main/lsgump.c` | 333 | 0x09b1f0 | 6122 | whole segment | `-O -P -Y` | EXACT |
| `main/shapegen.c` | 334 | 0x09cb30 | 622 | whole segment | `-O -P` | RELOC |
| `main/signgump.c` | 335 | 0x09cdc0 | 1038 | whole segment | `-O -P` | RELOC |
| `main/scrlg_ov.c` | 336 | 0x09d1f0 | 836 | whole segment | `-O -P` | RELOC |
| `main/slider.c` | 337 | 0x09d550 | 2145 | whole segment | `-O -P` | RELOC |
| `main/spellbk.c` | 338 | 0x09de20 | 3338 | whole segment | `-O -P -d` | RELOC |
| `sound/cflxcach.c` | 339 | 0x09ebb0 | 594 | whole segment | `-O -G -P` | RELOC |
| `sound/cspeech.c` | 340 | 0x09ee20 | 482 | whole segment | `-O -G -P` | RELOC |
| `sound/voice.c` | 341 | 0x09f020 | 2059 | whole segment | `-O -G -P -d -Y` | RELOC |
| `main/combat/ammo.c` | 342 | 0x09f8b0 | 316 | whole segment | `-O -G -P -Y` | RELOC |
| `main/combat/attack.c` | 343 | 0x09fa10 | 9091 | whole segment | `-O -G -P` | RELOC |
| `main/appts.c` | 344 | 0x0a1ff0 | 1031 | whole segment | `-O -G -P` | RELOC |
| `main/combat/armor.c` | 345 | 0x0a2440 | 311 | whole segment | `-O -G -P -Y` | RELOC |
| `main/combat/cbattack.c` | 346 | 0x0a2590 | 6220 | whole segment | `-O -G -P -Y` | RELOC |
| `main/combat/combat.c` | 347 | 0x0a3f20 | 781 | whole segment | `-O -G -P` | RELOC |
| `main/combat/combatai.c` | 348 | 0x0a4270 | 16686 | whole segment | `-O -G -P` | RELOC |
| `main/combat/combmode.c` | 349 | 0x0a8780 | 2700 | whole segment | `-O -G -P` | RELOC |
| `main/combat/combpick.c` | 350 | 0x0a92b0 | 6769 | whole segment | `-O -G -P` | RELOC |
| `main/combat/combaux.c` | 351 | 0x0aaec0 | 2489 | whole segment | `-O -G -P` | RELOC |
| `main/combat/damage.c` | 352 | 0x0ab930 | 5676 | whole segment | `-O -G -P` | RELOC |
| `main/combat/combstat.c` | 353 | 0x0ad080 | 582 | whole segment | `-O -G -P` | RELOC |
| `main/combat/crime.c` | 354 | 0x0ad2e0 | 5489 | whole segment | `-O -G -P -Y` | RELOC |
| `main/combat/crimeov1.c` | 355 | 0x0ae9b0 | 6178 | whole segment | `-O -G -P -Y` | RELOC |
| `main/death.c` | 356 | 0x0b0330 | 6150 | whole segment | `-O -G -P` | RELOC |
| `main/combat/missile.c` | 357 | 0x0b1c70 | 5103 | whole segment | `-O -G -P -Y` | RELOC |
| `main/combat/misstrac.c` | 358 | 0x0b30e0 | 4868 | whole segment | `-O -G -P` | RELOC |
| `main/combat/monsters.c` | 359 | 0x0b4480 | 367 | whole segment | `-O -G -P -Y` | RELOC |
| `main/combat/randstat.c` | 360 | 0x0b4610 | 3800 | whole segment | `-O -G -P` | RELOC |
| `main/combat/ready.c` | 361 | 0x0b55f0 | 311 | whole segment | `-O -G -P -Y` | RELOC |
| `main/combat/weapons.c` | 362 | 0x0b5740 | 341 | whole segment | `-O -G -P -Y` | RELOC |
| `main/combat/daze.c` | 363 | 0x0b58b0 | 3144 | whole segment | `-O -G -P` | RELOC |
| `main/combat/combwpn.c` | 364 | 0x0b65b0 | 7031 | whole segment | `-O -G -P -Y` | RELOC |
