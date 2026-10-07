/* Serpent Isle SI.EXE, resident segment 25 (file offsets 0x0166ac to 0x01671a, 110 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
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

void ReportFileNotFound(char *name)
{
	FatalError("File \"%s\" not found!", name);
}

void ReportFileReadError(char *name)
{
	FatalError("Error readings file \"%s\"!", name);
}

void ReportInvalidSaveGame(void)
{
	FatalError("Invalid Save Game.");
}
