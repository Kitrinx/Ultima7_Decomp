ULTIMA VII - PORTABLE
=====================

Two programs that run the Ultima VII games on a modern computer, without
DOSBox:

  Ultima7    Ultima VII: The Black Gate (with Forge of Virtue)
  Serpent    Ultima VII Part Two: Serpent Isle (with The Silver Seed)

They are only the programs: you need your own copy of each game you want to
play. Music also needs Roland MT-32 ROM files, which are not included.

The Serpent Isle port is new and has so far been tested on macOS only.


WHAT YOU NEED
-------------

1. The game files, from an installed copy (for example the GOG releases). The
   folder you want is the one that contains a folder named STATIC:
     Black Gate:     usually next to U7.EXE and ULTIMA7.COM
     Serpent Isle:   usually next to SI.EXE and SERPENT.COM

2. For music and sound effects (optional): the Roland MT-32 ROM files, a
   "control" ROM and a "PCM" ROM. They must be from an MT-32; CM-32L ROMs are
   not used. The file names don't matter. Without them the games play with
   speech but no music or sound effects.


SETTING UP
----------

Each program goes with its own game: Ultima7 with The Black Gate, Serpent
with Serpent Isle. Set up only the ones you have.

Windows and Linux:

1. Copy the game folder (the one with STATIC in it) somewhere you can write
   to, such as Documents. The game saves into this folder. Don't use a folder
   inside Program Files.

2. Put the program into that folder, next to STATIC:
     Black Gate:     Ultima7.exe (Windows), Ultima7 (Linux)
     Serpent Isle:   Serpent.exe (Windows), Serpent (Linux)

   Or name the game folder Ultima7 (Black Gate) or Serpent (Serpent Isle) and
   put it in your Documents folder; the program then finds it from anywhere.

3. If you have them, put the two MT-32 ROM files into the same folder.

When you are done, the folders look like this (the games' other files can
stay where they are):

     Ultima7\
       STATIC\
       Ultima7.exe         (Ultima7 on Linux)
       MT32_CONTROL.ROM    (any name; optional)
       MT32_PCM.ROM        (any name; optional)
       U7.EXE, ULTIMA7.COM and other files from the game

     Serpent\
       STATIC\
       Serpent.exe         (Serpent on Linux)
       MT32_CONTROL.ROM    (any name; optional)
       MT32_PCM.ROM        (any name; optional)
       SI.EXE, SERPENT.COM and other files from the game

macOS (Apple Silicon Macs only):

1. Open the disk image and drag Ultima7 and Serpent onto the Applications
   folder.

2. Copy each game folder (the one with STATIC in it) into your Documents
   folder: name The Black Gate's Ultima7 and Serpent Isle's Serpent. The
   games save into these folders.

3. If you have them, put the two MT-32 ROM files into each of those folders.

When you are done, Documents looks like this:

     Documents/
       Ultima7/
         STATIC/
         MT32_CONTROL.ROM  (any name; optional)
         MT32_PCM.ROM      (any name; optional)
         U7.EXE, ULTIMA7.COM and other files from the game
       Serpent/
         STATIC/
         MT32_CONTROL.ROM  (any name; optional)
         MT32_PCM.ROM      (any name; optional)
         SI.EXE, SERPENT.COM and other files from the game

You don't need to change any settings: music turns on by itself when the
ROMs are there, and speech is always on.


PLAYING
-------

Windows:  Double-click Ultima7.exe or Serpent.exe. (Not ULTIMA7.COM, U7.EXE,
          SERPENT.COM or SI.EXE: those are the original DOS programs and
          won't run.) The first time, Windows may show "Windows protected
          your PC" because the programs are not signed: click "More info",
          then "Run anyway".

macOS:    Open Ultima7 or Serpent from Applications or Launchpad. The first
          time, macOS asks whether to open an app downloaded from the
          internet, then whether it may use your Documents folder: allow
          both.

Linux:    Double-click Ultima7 or Serpent, or run ./Ultima7 or ./Serpent
          from the folder. Needs a desktop with OpenGL and sound (ALSA), and
          a recent system (glibc 2.39 or newer, such as Ubuntu 24.04).

Each game starts with its introduction (press Esc to skip it), then the main
menu, as the original did. Start a new game there, or journey onward with
your saved game. To quit, close the window. Alt-Enter (Option-Return on a Mac
keyboard) switches between a window and full screen.

Your saved games are kept in each game's folder (the GAMEDAT folder and the
GAME*.U7 files). Back them up by copying them somewhere safe.


FOR ADVANCED USERS
------------------

Started from a terminal, each program accepts options (--help lists them):

  --data <folder>    use the game files in this folder
  --cheat            enable the original cheat keys
  --mt32-short-waits skip the original's pauses after MT-32 memory writes

and these environment variables:

  U7_DATA            the game folder, instead of --data
  U7_MT32_ROMS       a folder holding the MT-32 ROMs, if not the game folder

Messages and errors are printed to the terminal.


LICENSES
--------

These programs include libmatoya (MIT license) and Munt's mt32emu (GNU Lesser
General Public License 2.1 or later). Their license texts are in the licenses
folder. Ultima VII and Serpent Isle are trademarks of Electronic Arts; the
game files are not included.
