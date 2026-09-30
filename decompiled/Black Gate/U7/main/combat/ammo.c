/* Black Gate U7.EXE, overlay segment 209 (file offsets 0x052830 to 0x05297e, 334 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "lowlevel.h"
#include "dosio.h"
#include "debug.h"
#include "oops.h"
#include "makemojo.h"
#include "voolook.h"
#include "ammo.h"

char AmmoFileName[] = "ammo.dat";
AmmoTable AmmoRecords;
unsigned char AmmoCount = 0;
ShapeLookup AmmoLookup;

void AmmoTable::load(char *name)
{
	AmmoRecord record;
	int fd;

	fd = DosOpen(name);
	if (fd == -1)
		ReportFileNotFound(name);
	if (DosRead(fd, 0L, 1L, &AmmoCount) != 1)
		ReportError(0xe302);
	DebugPrintf("%d ammos + default record\n", AmmoCount);
	if (!MakeMojo(AmmoCount + 1, 13L, &base, &count))
		ReportOutOfVoodooMemory();
	FillFarBytes(&record, 13, 0);
	record.family = -1;
	record.projectile = -1;
	CheckMojoBounds(count, 0L);
	CopyFarToLinear(base, &record, 13L);
	if (!ReadMojo(fd, -1L, 1L, AmmoCount, count, base, 13L))
		ReportError(0xe302);
	DosClose(fd);
}
