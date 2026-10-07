/* Serpent Isle SI.EXE, overlay segment 362 (file offsets 0x0b5740 to 0x0b5895, 341 bytes).
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
#include "weapons.h"

char WeaponsFileName[] = "weapons.dat";
WeaponTable WeaponRecords;
uint8_t WeaponCount = 0;
ShapeLookup WeaponLookup;

void WeaponTable::load(char *name)
{
	int16_t fd;
	WeaponRecord record;

	fd = DosOpen(name);
	if (fd == -1)
		ReportFileNotFound(name);
	if (DosRead(fd, INT32_C(0), INT32_C(1), &WeaponCount) != 1)
		ReportFileNotFound(name);
	if (!MakeMojo(WeaponCount + 1, INT32_C(21), &base, &count))
		ReportOutOfVoodooMemory();
	FillFarBytes(&record, 21, 0);
	record.damage = 1;
	record.ammo = -1;
	record.projectile = -3;
	record.range = 3;
	record.meleeReadyFrame = 1;
	record.meleeStrikeFrame = 1;
	CheckMojoBounds(count, INT32_C(0));
	CopyFarToLinear(base, &record, INT32_C(21));
	if (!ReadMojo(fd, -INT32_C(1), INT32_C(1), WeaponCount, count, base, INT32_C(21)))
		ReportFileNotFound(name);
	DosClose(fd);
}

extern "C" void ResetWeaponsGlobals(void)
{
	strcpy(WeaponsFileName, "weapons.dat");
	memset((void *)&WeaponRecords, 0, sizeof(WeaponRecords));
	WeaponCount = 0;
	memset(&WeaponLookup, 0, sizeof(WeaponLookup));
}

extern "C" void ConstructWeaponsGlobals(void)
{
	new (&WeaponRecords) WeaponTable();
}
