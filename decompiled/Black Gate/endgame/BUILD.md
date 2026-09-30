# Building ENDGAME.EXE

## Requirements

- Borland C++ 2.0: BCC 2.0, TLINK 4.0, TLIB and MAKE 3.5, with its libraries and headers.
- Turbo Assembler 2.51, on the `PATH`.
- DOS, or DOSBox-X with EMS off (BCC hangs when it spills into EMS).
- U7's tree beside this one as `..\U7`, and the modules the helper programs share as `..\shared`.

The Borland tools are commercial. Supply your own copy; never commit it.

## Build

```
make                    compiler in C:\BORLANDC
make -DBC=C:\BC20       compiler elsewhere
```

Run it in this directory, the build directory. Objects, `FATALERR.LIB`, `ENDGAME.EXE` and
`ENDGAME.MAP` land here.

- `MAKEFILE`: every module's compiler options. It is the source of truth.
- `ENDGAME.LNK`: TLINK's response file: link order, startup module, `CM.LIB`, then
  `FATALERR.LIB`.
- `ENDGAME.CFG`, `U7.CFG`: include paths, which MAKE writes. Each module sees its own tree's
  headers first.
- `fatalerr.c` links from a library searched after `CM.LIB`: the shipped file keeps its data
  after the runtime library's.

## Output

`ENDGAME.EXE` is 107,630 bytes, as shipped, and differs from it in 73 bytes around `cachelst.c`:
the shipped file places DoubleList's table and destructor copy ahead of CacheList's. The object
is not the cause: its relocations come out in the shipped order. TLINK places virtual segments
in the order it first meets their names, and the shipped link met DoubleList's two names in an
earlier object that does not define them. Which object that was is unknown.
