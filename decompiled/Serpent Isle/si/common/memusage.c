/* Serpent Isle SI.EXE, resident segment 63 (file offsets 0x02ab10 to 0x02ab9b, 139 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder chosen by subsystem.
 */

#include <stdio.h>
#include "dosio.h"
#include "memfree.h"

/* free memory now against the figures in start */
char *SprintfMemoryUsage(struct MemInfo *start)
{
	struct MemInfo now;

	now.nearFree = 0;
	now.farFree = 0;
	now.highFree = 0;
	GetMemoryInfo(&now);
	sprintf(WorkString,
		"'Memory: Original  Current     Used\n"
		"'NEAR       %5u    %5u    %5lu\n"
		"'FAR     %8lu %8lu %8lu\n"
		"'HIGH    %8lu %8lu %8lu\n",
		start->nearFree, now.nearFree, (unsigned long) start->nearFree - now.nearFree,
		start->farFree, now.farFree, start->farFree - now.farFree,
		start->highFree, now.highFree, start->highFree - now.highFree);
	return WorkString;
}
