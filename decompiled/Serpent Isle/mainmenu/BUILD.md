# Building MAINMENU.EXE

## Requirements

- Borland C++ 2.0: BCC 2.0, TLINK 4.0 and MAKE 3.5, with its libraries and headers.
- Turbo Assembler 2.51, on the `PATH`.
- DOS, or DOSBox-X with EMS off (BCC hangs when it spills into EMS).

The tree is self-contained: Serpent Isle keeps its own copy of every module. The Borland tools are
commercial. Supply your own copy; never commit it.

## Build

```
make                    compiler in C:\BORLANDC
make -DBC=C:\BC20       compiler elsewhere
```

Run it in this directory, the build directory. Objects, `MAINMENU.EXE` and `MAINMENU.MAP` land here.

- `MAKEFILE`: every module's compiler options. It is the source of truth. `midiplay.c` compiles with
  `-y`: without line numbers BCC merges two branch ends in SetMidiSfxControl that the shipped code keeps.
- `MAINMENU.LNK`: TLINK's response file: link order, startup module and `CM.LIB`.
  `trial_link.py --target si_mainmenu` links in this order too.
- `MAINMENU.CFG`: include paths, which MAKE writes.
- TLINK runs with `/i`: the shipped file carries its uninitialized data and stack as zeros.
- `dirdelta.c` and `main/video/vidmode.asm` hold data and no code, as in Black Gate MAINMENU.
  `vidmode.asm`'s empty VIDMODE_TEXT places `main/vidmode.c`'s code before `crtport.asm` although
  `vidmode.c` links after `modecolr.c`. Its object keeps its folder, since the names clash.
- `freexmm.c` and `xmmhand.c` compile in their own folder: `__FILE__` gives their bare names.

From the repository root, `uv run python3 agents/tools/build_exe.py --target si_mainmenu` runs this
makefile in DOSBox-X from a clean copy of the tree, and
`uv run --with capstone python3 agents/tools/golden_check.py --target si_mainmenu` adds the style
check, every module's status and a full comparison.

## Output

`MAINMENU.EXE` equals the shipped file: 132,080 bytes, SHA-256
`d1873a679dca562e6b0897f96b445549f09c09e6738de3b70bde632969ea2adc`.
