/* Black Gate U7.EXE, overlay segment 270 (file offsets 0x0800b0 to 0x080217, 359 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "lowlevel.h"
#include "dosio.h"
#include "debug.h"
#include "oops.h"
#include "makemojo.h"
#include "voolook.h"
#include "weapons.h"

char WeaponsFileName[] = "weapons.dat";
WeaponTable WeaponRecords;
unsigned char WeaponCount = 0;
ShapeLookup WeaponLookup;

void WeaponTable::load(char *name)
{
	int fd;
	WeaponRecord record;

	fd = DosOpen(name);
	if (fd == -1)
		ReportFileNotFound(name);
	if (DosRead(fd, 0L, 1L, &WeaponCount) != 1)
		ReportFileNotFound(name);
	DebugPrintf("%d weapons + default record\n", WeaponCount);
	if (!MakeMojo(WeaponCount + 1, 21L, &base, &count))
		ReportOutOfVoodooMemory();
	FillFarBytes(&record, 21, 0);
	record.damage = 1;
	record.ammo = -1;
	record.projectile = -3;
	record.range = 3;
	record.meleeReadyFrame = 1;
	record.meleeStrikeFrame = 1;
	CheckMojoBounds(count, 0L);
	CopyFarToLinear(base, &record, 21L);
	if (!ReadMojo(fd, -1L, 1L, WeaponCount, count, base, 21L))
		ReportFileNotFound(name);
	DosClose(fd);
}
