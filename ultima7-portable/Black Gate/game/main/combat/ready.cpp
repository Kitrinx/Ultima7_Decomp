/* Black Gate U7.EXE, overlay segment 253 (file offsets 0x0786a0 to 0x0787d7, 311 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include <new>
#include "lowlevel.h"
#include "dosio.h"
#include "oops.h"
#include "makemojo.h"
#include "voolook.h"
#include "ready.h"

char ReadyFileName[] = "ready.dat";
ReadyTable ReadyRecords;
uint8_t ReadyCount = 0;
ShapeLookup ReadyLookup;

void ReadyTable::load(char *name)
{
	char buf[9];
	int16_t fd;

	fd = DosOpen(name);
	if (fd == -1)
		ReportFileNotFound(name);
	if (DosRead(fd, INT32_C(0), INT32_C(1), &ReadyCount) != 1)
		ReportFileNotFound(name);
	if (!MakeMojo(ReadyCount + 1, INT32_C(9), &base, &count))
		ReportOutOfVoodooMemory();
	FillFarBytes(buf, 9, 0);
	CheckMojoBounds(count, INT32_C(0));
	CopyFarToLinear(base, buf, INT32_C(9));
	if (!ReadMojo(fd, -INT32_C(1), INT32_C(1), ReadyCount, count, base, INT32_C(9)))
		ReportFileNotFound(name);
	DosClose(fd);
}

extern "C" void ResetReadyGlobals(void)
{
	strcpy(ReadyFileName, "ready.dat");
	memset((void *)&ReadyRecords, 0, sizeof(ReadyRecords));
	ReadyCount = 0;
	memset(&ReadyLookup, 0, sizeof(ReadyLookup));
}

extern "C" void ConstructReadyGlobals(void)
{
	new (&ReadyRecords) ReadyTable();
}
