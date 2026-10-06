# Building ENDGAME.EXE

## Requirements

- Borland C++ 2.0: BCC 2.0, TLINK 4.0, TLIB and MAKE 3.5, with its libraries and headers.
- Turbo Assembler 2.51, on the `PATH`.
- DOS, or DOSBox-X with EMS off (BCC hangs when it spills into EMS).
- The tree is self-contained: the U7 modules this program links are copied under `u7\`, with
  their headers.

The Borland tools are commercial. Supply your own copy; never commit it.

## Build

```
make                    compiler in C:\BORLANDC
make -DBC=C:\BC20       compiler elsewhere
```

Run it in this directory, the build directory. Objects, `MODULES.LIB`, `FATALERR.LIB`, `ENDGAME.EXE` and
`ENDGAME.MAP` land here.

- `MAKEFILE`: every module's compiler options. It is the source of truth.
- `ENDGAME.LNK`: TLINK's response file: link order, startup module, `MODULES.LIB`,
  `CM.LIB`, then `FATALERR.LIB`.
- `ENDGAME.CFG`, `U7.CFG`: include paths, which MAKE writes. Each module sees its own tree's
  headers first.
- `fatalerr.c` links from a library searched after `CM.LIB`: the shipped file keeps its data
  after the runtime library's.

## Output

`ENDGAME.EXE` equals the shipped file: 107,630 bytes.

TLINK reads every library member, even one it does not link, and places a virtual table where it
first meets its name. The shipped order needs DoubleList's names met before `cachelst.c`'s, so the
modules from `cachelst.c` on link from `MODULES.LIB` (members in link order, from `MODULES.RSP`),
which also holds `unlinked\newlist.c`: an unused member that uses DoubleList, standing in for one
of Origin's that is not known.
