/* Serpent Isle SI.EXE, overlay segment 345 (file offsets 0x0a2440 to 0x0a2577, 311 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
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
