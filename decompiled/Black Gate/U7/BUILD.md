# Building U7.EXE

## Requirements

- Borland C++ 2.0: BCC 2.0, TLINK 4.0 and MAKE 3.5, with its libraries and headers.
- Turbo Assembler 2.51, on the `PATH`.
- DOS, or DOSBox-X with EMS off (BCC hangs when it spills into EMS).

The Borland tools are commercial. Supply your own copy; never commit it.

## Build

```
cd main
make                    compiler in C:\BORLANDC
make -DBC=C:\BC20       compiler elsewhere
```

`main` is the build directory. The other source directories sit beside it and compile as
`..\common\chunk.c`, as `__FILE__` in the shipped game shows. Objects, `U7.EXE` and `U7.MAP` land
in `main`.

- `main/MAKEFILE`: every module's compiler options. It is the source of truth.
- `main/U7.LNK`: TLINK's response file: link order, overlays (`/o`), startup module, libraries.
- `U7.CFG`: options every C module shares. MAKE writes it.

Each source also names its options in its header.

## Output

`U7.EXE` equals the shipped file: 689,248 bytes, SHA-256
`4d588b12c775927c77c221531be4910eaf413864a8e2ec3f6f0d7a9c302b6e54`, provided the link is run as
Origin ran it:

- The output is named `U7.EXE` and the map `objfile.map`. TLINK leaves the output name and the
  map name's first letters in the overlay header.
- The date is 06/02/1992, which TLINK stores as the link date.
- EMS is available. Without it TLINK swaps to disk and leaves stale memory in the padding after
  overlays.

So compile without EMS, then set that date, turn EMS on and rerun the makefile's TLINK command.
