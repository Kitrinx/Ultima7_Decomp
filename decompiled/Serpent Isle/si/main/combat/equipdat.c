/* Serpent Isle SI.EXE, overlay segment 220 (file offsets 0x05d790 to 0x05d8c7, 311 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

#include "lowlevel.h"
#include "dosio.h"
#include "oops.h"
#include "makemojo.h"
#include "equip.h"

char EquipFileName[] = "equip.dat";
EquipTable EquipRecords;
unsigned char EquipCount = 0;

void EquipTable::load(char *name)
{
	char buf[60];
	int fd;

	fd = DosOpen(name);
	if (fd == -1)
		ReportFileNotFound(name);
	if (DosRead(fd, 0L, 1L, &EquipCount) != 1)
		ReportFileNotFound(name);
	if (!MakeMojo(EquipCount + 1, 60L, &base, &count))
		ReportOutOfVoodooMemory();
	FillFarBytes(buf, 60, 0);
	CheckMojoBounds(count, 0L);
	CopyFarToLinear(base, buf, 60L);
	if (!ReadMojo(fd, -1L, 1L, EquipCount, count, base, 60L))
		ReportFileNotFound(name);
	DosClose(fd);
}
