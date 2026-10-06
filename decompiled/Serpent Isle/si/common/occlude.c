/* Serpent Isle SI.EXE, resident segment 75 (file offsets 0x0308a4 to 0x0309a6, 258 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "dosio.h"
#include "easyfile.h"
#include "cullmask.h"

static char *OccludeFileName = "OCCLUDE.DAT";
Occlusion OcclusionTable;

void InitOcclusionTable(void)
{
	if (!OcclusionTable.allocate(1024))
		ReportError(44512);
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
