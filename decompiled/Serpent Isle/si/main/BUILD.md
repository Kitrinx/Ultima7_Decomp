# Building SI.EXE

## Requirements

- Borland C++ 2.0: BCC 2.0, TLINK 4.0 and MAKE 3.5, with its libraries and headers.
- Turbo Assembler 2.51, on the `PATH`.
- DOS, or DOSBox-X with EMS off (BCC hangs when it spills into EMS).

The tree is self-contained: Serpent Isle keeps its own copy of every module. The Borland tools are
commercial. Supply your own copy; never commit it.

## Build

```
cd main
make                    compiler in C:\BORLANDC
make -DBC=C:\BC20       compiler elsewhere
```

`main` is the build directory. Objects, `SI.EXE` and `SI.MAP` land in it.

- `MAKEFILE`: every module's compiler options. `SI.LNK`: TLINK's response file, with the link order,
  the overlays (`/o`), the startup module and the libraries (stock `OVERLAY.LIB`, `EMU.LIB`,
  `MATHM.LIB`, `CM.LIB`). Both are written by `.agents/tools/si_makefile.py` from the sources and
  the last trial link; regenerate them, never edit them by hand. `--check` reports drift.
- `SI.CFG` and `SUB.CFG`: options every C module shares, which MAKE writes. Each folder gets its own
  `-I`: BCC 2.0 hangs on one `-I` that long.
- Sources whose `__FILE__` is a bare name (`/* path: chunk.c */`) compile inside their own folder,
  as the shipped error strings show. MAKE rebuilds those on every run: their dependency records name
  the source relative to that folder.
- `main/palette/paldata1.asm`, `main/video/farptrs.asm` and the other codeless modules exist for
  their segment-table entries and data; `main/ovlnull.c` is an overlay with no code.
- `main/inv_ov2.c` (overlay segment 330) builds without line numbers (`-y-`): they change how `-O`
  merges its branch ends.

From the repository root, `uv run python3 agents/tools/build_exe.py --target si` runs this makefile
in DOSBox-X from a clean copy of the tree. `uv run --with capstone python3 agents/tools/golden_check.py
--target si` adds the style check, every module's status and a full comparison.

## Output

`SI.EXE` equals the shipped file: 754,240 bytes, SHA-256
`9591cd8d6d8c9b05bcb99fe6849f637abfab5c5cd52608078b2bd0cddea3a8a3`, provided the link is run as
Origin ran it:

- The map is named `mapfile.map`. TLINK leaves the map name's first letters after the output name
  in the overlay header.
- The date is 07/14/1993, which TLINK stores as the link date.
- EMS is available. Without it TLINK swaps to disk and leaves stale memory in the padding after
  overlays.

`build_exe.py` compiles under MAKE without EMS (BCC can hang with it), then reruns the makefile's
TLINK command with that date and EMS on (target profile `build.link_run`).
