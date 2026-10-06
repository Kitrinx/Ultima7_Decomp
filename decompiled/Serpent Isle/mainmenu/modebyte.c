/* Serpent Isle MAINMENU.EXE, one module of resident segment 83 (file offsets 0x01a500 to 0x01a50a, 10 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "init.h"

/* the BIOS video mode to go back to; 3 is color text */
struct VideoMode {
	unsigned char mode;
	VideoMode() : mode(3) {}
};

int UnusedStartupWord1 = 0x114;
int UnusedStartupWord2 = 0x100;
int UnusedStartupWord3 = 0;
int UnusedStartupWord4 = -1;
char UnusedStartupBytes[6] = { 0 };
HookRecord *InterruptHookList = 0;
char UnusedStartupBlock[29] = { 0 };
VideoMode UnusedVideoMode;
char *UnusedStartupBlockPointer = UnusedStartupBlock;
char UnusedStartupTable[14] = { 1, 0, 2, 0, 1, 0x14, 0, 1, 0, 1, 0, 1, 0, 0 };
char UnusedStartupSpace[402];
