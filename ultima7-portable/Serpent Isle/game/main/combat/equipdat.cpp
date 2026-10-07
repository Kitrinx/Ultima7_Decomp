/* Serpent Isle SI.EXE, overlay segment 220 (file offsets 0x05d790 to 0x05d8c7, 311 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
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

extern "C" void ResetEquipdatGlobals(void)
{
	strcpy(EquipFileName, "equip.dat");
	memset((void *)&EquipRecords, 0, sizeof(EquipRecords));
	EquipCount = 0;
}

extern "C" void ConstructEquipdatGlobals(void)
{
	new (&EquipRecords) EquipTable();
}
