/* Serpent Isle SI.EXE, resident segment 80 (file offsets 0x03259c to 0x032a57, 1211 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

#include "coord.h"
#include "maps.h"
#include "sche_ov1.h"
#include "sche.h"

char *ScheduleFileName = "static\\schedule.dat";
char *ScheduleTempFileName = "art\\schedule.$$$";
Schedules ScheduleTable;

unsigned char Schedule_findEntry(Schedules *s, unsigned char npc, unsigned char time);

/* Whether NPC npc has an nth entry. */
unsigned char Schedule_hasEntry(Schedules *s, unsigned char npc, unsigned char n)
{
	if (s->index[npc] + n >= *(s->index + npc + 1) || npc >= s->count)
		return 0;
	return 1;
}

/* What NPC npc does at time, or 0xff when it has no entries. */
unsigned char Schedule_getWorkType(Schedules *s, unsigned char npc, unsigned char time)
{
	int t;
	unsigned char n;

	for (t = time; t > -1; t--)
		if ((n = Schedule_findEntry(s, npc, t)) != 0xff)
			return s->entries[s->index[npc] + n].type;
	for (t = 7; time < t; t--)
		if ((n = Schedule_findEntry(s, npc, t)) != 0xff)
			return s->entries[s->index[npc] + n].type;
	return 0xff;
}

/* Where NPC npc is at time, as a place in a region. */
unsigned char Schedule_getPlace(Schedules *s, unsigned char npc, unsigned char time, unsigned char *x, unsigned char *y,
	unsigned char *region)
{
	int t;
	unsigned char n;

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
unsigned char Schedule_getCoord(Schedules *s, unsigned char npc, unsigned char time, Coord *x, Coord *y)
{
	unsigned char cx, cy, region;

	if (Schedule_getPlace(s, npc, time, &cx, &cy, &region)) {
		*x = Coord(RegionX[region] + cx);
		*y = Coord(RegionY[region] + cy);
		return 1;
	}
	return 0;
}

/* NPC npc's entry for exactly time, or 0xff. */
unsigned char Schedule_findEntry(Schedules *s, unsigned char npc, unsigned char time)
{
	int n;

	for (n = 0; Schedule_hasEntry(s, npc, n); n++)
		if (s->entries[s->index[npc] + (unsigned char) n].time == time)
			return n;
	return 0xff;
}

unsigned char Schedule_getEntryWorkType(Schedules *s, unsigned char npc, unsigned char time)
{
	unsigned char n;

	if ((n = Schedule_findEntry(s, npc, time)) == 0xff)
		return 0xff;
	return s->entries[s->index[npc] + n].type;
}

int Schedule_getEntryX(Schedules *s, unsigned char npc, unsigned char time)
{
	char n;

	if ((n = Schedule_findEntry(s, npc, time)) == 0xff)
		return -1;
	return s->entries[s->index[npc] + n].x;
}

int Schedule_getEntryY(Schedules *s, unsigned char npc, unsigned char time)
{
	char n;

	if ((n = Schedule_findEntry(s, npc, time)) == 0xff)
		return -1;
	return s->entries[s->index[npc] + n].y;
}

int Schedule_getEntryRegion(Schedules *s, unsigned char npc, unsigned char time)
{
	char n;

	if ((n = Schedule_findEntry(s, npc, time)) == 0xff)
		return -1;
	return s->entries[s->index[npc] + n].region;
}
