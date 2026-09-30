# Building INTRO.EXE

## Requirements

- Borland C++ 2.0: BCC 2.0, TLINK 4.0 and MAKE 3.5, with its libraries and headers.
- Turbo Assembler 2.51, on the `PATH`.
- DOS, or DOSBox-X with EMS off (BCC hangs when it spills into EMS).
- U7's tree beside this one as `..\U7`, and the modules the helper programs share as `..\shared`.

The Borland tools are commercial. Supply your own copy; never commit it.

## Build

```
make                    compiler in C:\BORLANDC
make -DBC=C:\BC20       compiler elsewhere
```

Run it in this directory, the build directory. Objects, `INTRO.EXE` and `INTRO.MAP` land here.

- `MAKEFILE`: every module's compiler options. It is the source of truth.
- `INTRO.LNK`: TLINK's response file: link order, startup module, `CM.LIB`.
- `INTRO.CFG`, `SHARED.CFG`, `U7.CFG`: include paths, which MAKE writes. Each module sees its
  own tree's headers first.
- `IDESTUB.BAT`: Origin built INTRO with the BC 2.0 IDE, which compiles each file under its full
  upper-case path. `specache.c` keeps its path in `__FILE__` (`\U7\SOUND\SPECACHE.C`), so the
  batch file copies it to that path on the build drive and writes a stub that includes it.

## Output

`INTRO.EXE` equals the shipped file: 116,068 bytes, SHA-256
`6c7ec86d720f49ba5e47b15e5c26292c950ab002365a6bd43ca967c2309a2587`.
