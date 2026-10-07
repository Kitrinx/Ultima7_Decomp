/* Serpent Isle SI.EXE, overlay segment 345 (file offsets 0x0a2440 to 0x0a2577, 311 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
#include "lowlevel.h"
#include "dosio.h"
#include "oops.h"
#include "makemojo.h"
#include "voolook.h"
#include "armor.h"

char ArmorFileName[] = "armor.dat";
ArmorTable ArmorRecords;
uint8_t ArmorCount = 0;
ShapeLookup ArmorLookup;

void ArmorTable::load(char *name)
{
	char buf[10];
	int16_t fd;

	fd = DosOpen(name);
	if (fd == -1)
		ReportFileNotFound(name);
	if (DosRead(fd, INT32_C(0), INT32_C(1), &ArmorCount) != 1)
		ReportFileNotFound(name);
	if (!MakeMojo(ArmorCount + 1, INT32_C(10), &base, &count))
		ReportOutOfVoodooMemory();
	FillFarBytes(buf, 10, 0);
	CheckMojoBounds(count, INT32_C(0));
	CopyFarToLinear(base, buf, INT32_C(10));
	if (!ReadMojo(fd, -INT32_C(1), INT32_C(1), ArmorCount, count, base, INT32_C(10)))
		ReportFileNotFound(name);
	DosClose(fd);
}

extern "C" void ResetArmorGlobals(void)
{
	strcpy(ArmorFileName, "armor.dat");
	memset((void *)&ArmorRecords, 0, sizeof(ArmorRecords));
	ArmorCount = 0;
	memset(&ArmorLookup, 0, sizeof(ArmorLookup));
}

extern "C" void ConstructArmorGlobals(void)
{
	new (&ArmorRecords) ArmorTable();
}
