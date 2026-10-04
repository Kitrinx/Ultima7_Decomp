# Ultima7 Decompile

This is an AI assisted decompilation of the Ultima 7 game code. Original names were used wherever they were available, but the majority of the function and variable names are inferred, however the logic itself was tested to produce byte exact results using Borland C++ 2.0, the compiler originally used to build Ultima 7.

The `decompiled` folder contains the source code for the various binaries in Ultima 7. A lot of the original source was assembly code, which was faithfully preserved. It seems that they also used .c extensions for their C++ code, so I kept that convention.

The `ultima7-portable` folder is an attempt to clean up the decompiled code to make it more cross-platform friendly with minimal changes to the logic. This merges the multiple programs for the intro, endgame, menu, and main game, clears the globals between each load, attempts to catch or fix null pointer exceptions that were valid in dos, 16 to 32 bit issues, and etc.

The `ultima7-usecode` folder is tools for decompiling and building and working with U7 usecode. They haven't really been tested on anything but a mac.

I also plan to do Serpent Isle as well, and that's next on my plate.