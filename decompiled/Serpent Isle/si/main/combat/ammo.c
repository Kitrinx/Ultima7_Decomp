/* Serpent Isle SI.EXE, overlay segment 342 (file offsets 0x09f8b0 to 0x09f9ec, 316 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
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
