/* Black Gate U7.EXE, overlay segment 231 (file offsets 0x06bc10 to 0x06bd47, 311 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "dosio.h"
#include "oops.h"
#include "makemojo.h"
#include "equip.h"

char EquipFileName[] = "equip.dat";
EquipTable EquipRecords;
uint8_t EquipCount = 0;

void EquipTable::load(char *name)
{
	char buf[60];
	int16_t fd;

	fd = DosOpen(name);
	if (fd == -1)
		ReportFileNotFound(name);
	if (DosRead(fd, INT32_C(0), INT32_C(1), &EquipCount) != 1)
		ReportFileNotFound(name);
	if (!MakeMojo(EquipCount + 1, INT32_C(60), &base, &count))
		ReportOutOfVoodooMemory();
	FillFarBytes(buf, 60, 0);
	CheckMojoBounds(count, INT32_C(0));
	CopyFarToLinear(base, buf, INT32_C(60));
	if (!ReadMojo(fd, -INT32_C(1), INT32_C(1), EquipCount, count, base, INT32_C(60)))
		ReportFileNotFound(name);
	DosClose(fd);
}
