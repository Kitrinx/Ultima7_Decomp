# SERPENT.COM

The launcher uses local `serpent.asm`, copied unchanged from the dual-game
launcher source, and the local MAKEFILE selects TASM's `SERPENT` define.

From the repository root:

```
python3 agents/tools/build_com.py --target serpent
```

This runs TASM 2.51 and TLINK 4.0 through the local MAKEFILE, checks every output
byte against retail `SERPENT.COM`, and retains the output and log in
`.agents/tmp/build_com_serpent/`. It builds and checks only the SI launcher.
