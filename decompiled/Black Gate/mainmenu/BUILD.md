# Building MAINMENU.EXE

## Requirements

- Borland C++ 2.0: BCC 2.0, TLINK 4.0 and MAKE 3.5, with its libraries and headers.
- Turbo Assembler 2.51, on the `PATH`.
- DOS, or DOSBox-X with EMS off (BCC hangs when it spills into EMS).
- U7's tree beside this one as `..\U7`: MAINMENU links 58 of its modules unchanged.

The Borland tools are commercial. Supply your own copy; never commit it.

## Build

```
make                    compiler in C:\BORLANDC
make -DBC=C:\BC20       compiler elsewhere
```

Run it in this directory, the build directory. Objects, `MAINMENU.EXE` and `MAINMENU.MAP` land
here.

- `MAKEFILE`: every module's compiler options. It is the source of truth.
- `MAINMENU.LNK`: TLINK's response file: link order, startup module, `CM.LIB`.
- `MAINMENU.CFG`, `SHARED.CFG`: include paths, which MAKE writes. The program's modules see its
  own, older headers first; U7's modules see U7's.
- `IDESTUB.BAT`: Origin built MAINMENU with the BC 2.0 IDE, which compiles each file under its
  full upper-case path. `mouse.c` and `specache.c` keep theirs in `__FILE__`
  (`\U7\ZEVENT\MOUSE.C`), so the batch file copies each to that path on the build drive and
  writes a stub that includes it.

## Output

`MAINMENU.EXE` equals the shipped file: 127,116 bytes, SHA-256
`73d88ccdb103ee3c6ead70e64ed41eefa9312875ded98d93829979e2a511f43c`. A plain MZ file keeps no
output name and no linker scratch, so every byte is compared.
