/* Black Gate U7.EXE, overlay segment 255 (file offsets 0x078d10 to 0x0791cb, 1211 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "coord.h"
#include "maps.h"
#include "sche_ov1.h"
#include "sche.h"
#include <new>

char *ScheduleFileName = "static\\schedule.dat";
char *ScheduleTempFileName = "art\\schedule.$$$";
Schedules ScheduleTable;

uint8_t Schedule_findEntry(Schedules *s, uint8_t npc, uint8_t time);

/* Whether NPC npc has an nth entry. */
uint8_t Schedule_hasEntry(Schedules *s, uint8_t npc, uint8_t n)
{
	/* The first save of a new game comes before the schedules are read. DOS then read index
	 * from the start of the data segment, but count was still 0, so the answer was 0. */
	if (s->index == 0)
		return 0;
	if (s->index[npc] + n >= *(s->index + npc + 1) || (uint16_t)npc >= s->count)
		return 0;
	return 1;
}

/* What NPC npc does at time, or 0xff when it has no entries. */
uint8_t Schedule_getWorkType(Schedules *s, uint8_t npc, uint8_t time)
{
	int16_t t;
	uint8_t n;

	for (t = time; t > -1; t--)
		if ((n = Schedule_findEntry(s, npc, t)) != 0xff)
			return s->entries[s->index[npc] + n].type;
	for (t = 7; time < t; t--)
		if ((n = Schedule_findEntry(s, npc, t)) != 0xff)
			return s->entries[s->index[npc] + n].type;
	return 0xff;
}

/* Where NPC npc is at time, as a place in a region. */
uint8_t Schedule_getPlace(Schedules *s, uint8_t npc, uint8_t time, uint8_t *x, uint8_t *y,
	uint8_t *region)
{
	int16_t t;
	uint8_t n;

	for (t = time; t > -1; t--)
		if ((n = Schedule_findEntry(s, npc, t)) != 0xff) {
			*x = s->entries[s->index[npc] + n].x;
			*y = s->entries[s->index[npc] + n].y;
			*region = s->entries[s->index[npc] + n].region;
			return 1;
		}
	for (t = 7; time < t; t--)
		if ((n = Schedule_findEntry(s, npc, t)) != 0xff) {
			*x = s->entries[s->index[npc] + n].x;
			*y = s->entries[s->index[npc] + n].y;
			*region = s->entries[s->index[npc] + n].region;
			return 1;
		}
	return 0;
}

/* Where NPC npc is at time, in world coordinates. */
uint8_t Schedule_getCoord(Schedules *s, uint8_t npc, uint8_t time, Coord *x, Coord *y)
{
	uint8_t cx, cy, region;

	if (Schedule_getPlace(s, npc, time, &cx, &cy, &region)) {
		*x = Coord(RegionX[region] + cx);
		*y = Coord(RegionY[region] + cy);
		return 1;
	}
	return 0;
}

/* NPC npc's entry for exactly time, or 0xff. */
uint8_t Schedule_findEntry(Schedules *s, uint8_t npc, uint8_t time)
{
	int16_t n;

	for (n = 0; Schedule_hasEntry(s, npc, n); n++)
		if (s->entries[s->index[npc] + (uint8_t) n].time == time)
			return n;
	return 0xff;
}

uint8_t Schedule_getEntryWorkType(Schedules *s, uint8_t npc, uint8_t time)
{
	uint8_t n;

	if ((n = Schedule_findEntry(s, npc, time)) == 0xff)
		return 0xff;
	return s->entries[s->index[npc] + n].type;
}

int16_t Schedule_getEntryX(Schedules *s, uint8_t npc, uint8_t time)
{
	int8_t n;

	if ((n = Schedule_findEntry(s, npc, time)) == 0xff)
		return -1;
	return s->entries[s->index[npc] + n].x;
}

int16_t Schedule_getEntryY(Schedules *s, uint8_t npc, uint8_t time)
{
	int8_t n;

	if ((n = Schedule_findEntry(s, npc, time)) == 0xff)
		return -1;
	return s->entries[s->index[npc] + n].y;
}

int16_t Schedule_getEntryRegion(Schedules *s, uint8_t npc, uint8_t time)
{
	int8_t n;

	if ((n = Schedule_findEntry(s, npc, time)) == 0xff)
		return -1;
	return s->entries[s->index[npc] + n].region;
}

extern "C" void ResetScheGlobals(void)
{
	ScheduleFileName = "static\\schedule.dat";
	ScheduleTempFileName = "art\\schedule.$$$";
	memset(&ScheduleTable, 0, sizeof ScheduleTable);
}

extern "C" void ConstructScheGlobals(void)
{
	new (&ScheduleTable) Schedules();
}
