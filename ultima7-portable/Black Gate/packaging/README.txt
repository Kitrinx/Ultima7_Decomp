ULTIMA VII: THE BLACK GATE - PORTABLE
=====================================

This program runs Ultima VII: The Black Gate on a modern computer, without
DOSBox. It is only the program: you need your own copy of the game. Music
also needs Roland MT-32 ROM files, which are not included.


WHAT YOU NEED
-------------

1. Your Ultima VII: The Black Gate game files, from an installed copy (for
   example the GOG release). The folder you want is the one that contains a
   folder named STATIC, usually next to U7.EXE.

2. For music and sound effects (optional): the Roland MT-32 ROM files, a
   "control" ROM and a "PCM" ROM. They must be from an MT-32; CM-32L ROMs are
   not used. The file names don't matter. Without them the game plays with
   speech but no music or sound effects.


SETTING UP
----------

Windows and Linux:

1. Copy the game folder (the one with STATIC in it) somewhere you can write
   to, such as Documents. The game saves into this folder. Don't use a folder
   inside Program Files.

2. Put this program into that folder, next to STATIC:
     Windows:        Ultima7.exe
     Linux:          Ultima7

   Or name the game folder Ultima7 and put it in your Documents folder; the
   program then finds it from anywhere.

3. If you have them, put the two MT-32 ROM files into the same folder.

When you are done, the folder looks like this (the game's other files can
stay where they are):

     Ultima7\
       STATIC\
       Ultima7.exe         (Ultima7 on Linux)
       MT32_CONTROL.ROM    (any name; optional)
       MT32_PCM.ROM        (any name; optional)
       U7.EXE, ULTIMA7.COM and other files from the game

macOS (Apple Silicon Macs only):

1. Open the disk image and drag Ultima7 onto the Applications folder.

2. Copy the game folder (the one with STATIC in it) into your Documents
   folder and name it Ultima7. The game saves into this folder.

3. If you have them, put the two MT-32 ROM files into that Ultima7 folder.

When you are done, Documents looks like this:

     Documents/
       Ultima7/
         STATIC/
         MT32_CONTROL.ROM  (any name; optional)
         MT32_PCM.ROM      (any name; optional)
         U7.EXE, ULTIMA7.COM and other files from the game

You don't need to change any settings: music turns on by itself when the
ROMs are there, and speech is always on.


PLAYING
-------

Windows:  Double-click Ultima7.exe. (Not ULTIMA7.COM or U7.EXE: those are the
          original DOS programs and won't run.) The first time, Windows may
          show "Windows protected your PC" because the program is not
          signed: click "More info", then "Run anyway".

macOS:    Open Ultima7 from Applications or Launchpad. The first time, macOS
          asks whether to open an app downloaded from the internet, then
          whether Ultima7 may use your Documents folder: allow both.

Linux:    Double-click Ultima7, or run ./Ultima7 from the folder. Needs a
          desktop with OpenGL and sound (ALSA), and a recent system (glibc
          2.39 or newer, such as Ubuntu 24.04).

The game starts with its introduction (press Esc to skip it), then the main
menu. Choose "Start New Game" to begin, or "Journey Onward" to continue your
saved game. To quit, close the window.

Your saved games are kept in the game folder (the GAMEDAT folder and the
GAME*.U7 files). Back them up by copying them somewhere safe.

FOR ADVANCED USERS
------------------

Started from a terminal, the program accepts options (Ultima7 --help lists
them):

  --data <folder>    use the game files in this folder
  --cheat            enable the original cheat keys

and these environment variables:

  U7_DATA            the game folder, instead of --data
  U7_MT32_ROMS       a folder holding the MT-32 ROMs, if not the game folder

Messages and errors are printed to the terminal.


LICENSES
--------

This program includes libmatoya (MIT license) and Munt's mt32emu (GNU Lesser
General Public License 2.1 or later). Their license texts are in the licenses
folder. Ultima VII is a trademark of Electronic Arts; its game files are not
included.
