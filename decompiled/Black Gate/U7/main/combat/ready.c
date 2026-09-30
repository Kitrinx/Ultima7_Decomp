/* Black Gate U7.EXE, overlay segment 253 (file offsets 0x0786a0 to 0x0787d7, 311 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "lowlevel.h"
#include "dosio.h"
#include "oops.h"
#include "makemojo.h"
#include "voolook.h"
#include "ready.h"

char ReadyFileName[] = "ready.dat";
ReadyTable ReadyRecords;
unsigned char ReadyCount = 0;
ShapeLookup ReadyLookup;

void ReadyTable::load(char *name)
{
	char buf[9];
	int fd;

	fd = DosOpen(name);
	if (fd == -1)
		ReportFileNotFound(name);
	if (DosRead(fd, 0L, 1L, &ReadyCount) != 1)
		ReportFileNotFound(name);
	if (!MakeMojo(ReadyCount + 1, 9L, &base, &count))
		ReportOutOfVoodooMemory();
	FillFarBytes(buf, 9, 0);
	CheckMojoBounds(count, 0L);
	CopyFarToLinear(base, buf, 9L);
	if (!ReadMojo(fd, -1L, 1L, ReadyCount, count, base, 9L))
		ReportFileNotFound(name);
	DosClose(fd);
}
