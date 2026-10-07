/* Serpent Isle SI.EXE, overlay segment 342 (file offsets 0x09f8b0 to 0x09f9ec, 316 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
#include "lowlevel.h"
#include "dosio.h"
#include "debug.h"
#include "oops.h"
#include "makemojo.h"
#include "voolook.h"
#include "ammo.h"

char AmmoFileName[] = "ammo.dat";
AmmoTable AmmoRecords;
uint8_t AmmoCount = 0;
ShapeLookup AmmoLookup;

void AmmoTable::load(char *name)
{
	AmmoRecord record;
	int16_t fd;

	fd = DosOpen(name);
	if (fd == -1)
		ReportFileNotFound(name);
	if (DosRead(fd, INT32_C(0), INT32_C(1), &AmmoCount) != 1)
		ReportError(0xe302);
	if (!MakeMojo(AmmoCount + 1, INT32_C(13), &base, &count))
		ReportOutOfVoodooMemory();
	FillFarBytes(&record, 13, 0);
	record.family = -1;
	record.projectile = -1;
	CheckMojoBounds(count, INT32_C(0));
	CopyFarToLinear(base, &record, INT32_C(13));
	if (!ReadMojo(fd, -INT32_C(1), INT32_C(1), AmmoCount, count, base, INT32_C(13)))
		ReportError(0xe302);
	DosClose(fd);
}

extern "C" void ResetAmmoGlobals(void)
{
	strcpy(AmmoFileName, "ammo.dat");
	memset((void *)&AmmoRecords, 0, sizeof(AmmoRecords));
	AmmoCount = 0;
	memset(&AmmoLookup, 0, sizeof(AmmoLookup));
}

extern "C" void ConstructAmmoGlobals(void)
{
	new (&AmmoRecords) AmmoTable();
}
