/* Black Gate U7.EXE, overlay segment 211 (file offsets 0x052f00 to 0x053037, 311 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "lowlevel.h"
#include "dosio.h"
#include "oops.h"
#include "makemojo.h"
#include "voolook.h"
#include "armor.h"

char ArmorFileName[] = "armor.dat";
ArmorTable ArmorRecords;
unsigned char ArmorCount = 0;
ShapeLookup ArmorLookup;

void ArmorTable::load(char *name)
{
	char buf[10];
	int fd;

	fd = DosOpen(name);
	if (fd == -1)
		ReportFileNotFound(name);
	if (DosRead(fd, 0L, 1L, &ArmorCount) != 1)
		ReportFileNotFound(name);
	if (!MakeMojo(ArmorCount + 1, 10L, &base, &count))
		ReportOutOfVoodooMemory();
	FillFarBytes(buf, 10, 0);
	CheckMojoBounds(count, 0L);
	CopyFarToLinear(base, buf, 10L);
	if (!ReadMojo(fd, -1L, 1L, ArmorCount, count, base, 10L))
		ReportFileNotFound(name);
	DosClose(fd);
}
