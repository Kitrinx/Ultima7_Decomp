/* Black Gate U7.EXE, resident segment 101 (file offsets 0x036688 to 0x03678a, 258 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "dosio.h"
#include "easyfile.h"
#include "cullmask.h"

static char *OccludeFileName = "OCCLUDE.DAT";
Occlusion OcclusionTable;

void InitOcclusionTable(void)
{
	if (!OcclusionTable.allocate(1024))
		ReportError(0xade0);
}

void Occlusion::save(char *dir)
{
	int fd;

	fd = DosOpen(BuildPath(dir, OccludeFileName, 0));
	if (fd == -1)
		fd = CreateFileOrFail(BuildPath(dir, OccludeFileName, 0));
	WriteFileBlock(fd, 0L, (long) size, (char *)data);
	DosClose(fd);
}

void Occlusion::load(char *dir)
{
	int fd;

	fd = DosOpen(BuildPath(dir, OccludeFileName, 0));
	if (fd == -1)
		save(dir);
	else {
		ReadFileBlock(fd, 0L, (long) size, (char *)data);
		DosClose(fd);
	}
}
