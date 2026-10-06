/* Serpent Isle MAINMENU.EXE, resident segment 43 (file offsets 0x012d37 to 0x012d9b, 100 bytes).
 * Borland C++ 2.0 -mm -O -1 rebuilds it byte for byte.
 */

#include "init.h"
#include "oops.h"

void ReportOutOfFarMemory(void)
{
	FatalError("Out of far memory.");
}

void ReportOutOfNearMemory(void)
{
	FatalError("Out of near memory.");
}

void ReportOutOfVoodooMemory(void)
{
	FatalError("Out of voodoo memory.");
}

void ReportFileNotFound(char far *name)
{
	FatalError("File \"%Fs\" not found!", name);
}

void ReportFileReadError(char far *name)
{
	FatalError("Error readings file \"%Fs\"!", name);
}

void ReportInvalidSaveGame(void)
{
	FatalError("Invalid Save Game.");
}
