/* Black Gate U7.EXE, overlay segment 231 (file offsets 0x06bc10 to 0x06bd47, 311 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
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
