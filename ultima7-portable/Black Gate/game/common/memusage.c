/* Black Gate U7.EXE, resident segment 89 (file offsets 0x030e2d to 0x030eb8, 139 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
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
		"'NEAR       %5u    %5u    %5" PRIu32 "\n"
		"'FAR     %8" PRIu32 " %8" PRIu32 " %8" PRIu32 "\n"
		"'HIGH    %8" PRIu32 " %8" PRIu32 " %8" PRIu32 "\n",
		start->nearFree, now.nearFree, (uint32_t) start->nearFree - now.nearFree,
		start->farFree, now.farFree, start->farFree - now.farFree,
		start->highFree, now.highFree, start->highFree - now.highFree);
	return WorkString;
}
