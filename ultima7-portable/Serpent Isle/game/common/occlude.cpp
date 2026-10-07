/* Serpent Isle SI.EXE, resident segment 75 (file offsets 0x0308a4 to 0x0309a6, 258 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "dosio.h"
#include "easyfile.h"
#include "cullmask.h"
#include <new>

static char *OccludeFileName = "OCCLUDE.DAT";
Occlusion OcclusionTable;

void InitOcclusionTable(void)
{
	if (!OcclusionTable.allocate(1024))
		ReportError(44512);
}

void Occlusion::save(char *dir)
{
	int16_t fd;

	fd = DosOpen(BuildPath(dir, OccludeFileName, 0));
	if (fd == -1)
		fd = CreateFileOrFail(BuildPath(dir, OccludeFileName, 0));
	WriteFileBlock(fd, INT32_C(0), (int32_t) size, (char *)data);
	DosClose(fd);
}

void Occlusion::load(char *dir)
{
	int16_t fd;

	fd = DosOpen(BuildPath(dir, OccludeFileName, 0));
	if (fd == -1)
		save(dir);
	else {
		ReadFileBlock(fd, INT32_C(0), (int32_t) size, (char *)data);
		DosClose(fd);
	}
}

extern "C" void ResetOccludeGlobals(void)
{
	OccludeFileName = "OCCLUDE.DAT";
}

extern "C" void ConstructOccludeGlobals(void)
{
	new (&OcclusionTable) Occlusion();
}
