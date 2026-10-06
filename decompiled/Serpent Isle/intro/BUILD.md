# Building INTRO.EXE

## Requirements

- Borland C++ 2.0: BCC 2.0, TLINK 4.0, TLIB and MAKE 3.5, with its libraries and headers.
- Turbo Assembler 2.51, on the `PATH`.
- DOS, or DOSBox-X with EMS off (BCC hangs when it spills into EMS).

The tree is self-contained: Serpent Isle keeps its own copy of every module. The Borland tools are
commercial. Supply your own copy; never commit it.

## Build

```
make                    compiler in C:\BORLANDC
make -DBC=C:\BC20       compiler elsewhere
```

Run it in this directory, the build directory. Objects, `MODULES.LIB`, `FATALERR.LIB`, `INTRO.EXE` and `INTRO.MAP`
land here.

- `MAKEFILE`: every module's compiler options. It is the source of truth.
- `INTRO.LNK`: TLINK's response file: link order, startup module, `MODULES.LIB`,
  `CM.LIB`, then `FATALERR.LIB`.
  `trial_link.py --target si_intro` links in this order too.
- `INTRO.CFG`: include paths, which MAKE writes.
- `fatalerr.c` links from a library searched after `CM.LIB`: the shipped file keeps its data
  after the runtime library's.
- TLINK runs with `/i`: the shipped file carries its uninitialized data and stack as zeros, which
  only `/i` writes.
- `dirdelta.c`, `ds1482.c`, `box.c` and `flatflag.asm` hold data and no code. `flatflag.asm`'s empty
  word-aligned code segment makes the pad byte before `freexmm.c`.

From the repository root, `uv run python3 agents/tools/build_exe.py --target si_intro` runs this
makefile in DOSBox-X from a clean copy of the tree, and
`uv run --with capstone python3 agents/tools/golden_check.py --target si_intro` adds the style check,
every module's status and a full comparison.

## Output

`INTRO.EXE` equals the shipped file: 108,528 bytes.

TLINK reads every library member, even one it does not link, and places a virtual table where it
first meets its name. The shipped order needs DoubleList's names met before `cachelst.c`'s, so the
modules from `cachelst.c` on link from `MODULES.LIB` (members in link order, from `MODULES.RSP`),
which also holds `unlinked\newlist.c`: an unused member that uses DoubleList, standing in for one
of Origin's that is not known.
