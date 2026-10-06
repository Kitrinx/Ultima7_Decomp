/* Serpent Isle SI.EXE, overlay segment 256 (file offsets 0x06f070 to 0x06f3e3, 883 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Filename inferred from its schedule-editing role.
 */

#include "dosio.h"
#include "fileutil.h"
#include "oops.h"
#include "sche.h"
#include "memapi.h"
#include "schedit.h"

unsigned char Schedule_write(Schedules *s, char *name)
{
	int fd;

	fd = DosCreate(name);
	if (fd == -1)
		return 0;
	DosWrite(fd, 0L, sizeof s->count, &s->count);
	DosWrite(fd, -1L, (s->count + 1) * sizeof(unsigned), s->index);
	DosWrite(fd, -1L, s->index[s->count] * sizeof(ScheduleEntry), s->entries);
	DosClose(fd);
	return 1;
}

unsigned char Schedule_clearEntry(Schedules *s, unsigned char npc, unsigned char n)
{
	if (!Schedule_hasEntry(s, npc, n))
		return 0;
	Schedule_setEntryTime(s, npc, n, 0);
	Schedule_setEntryWorkType(s, npc, n, 0);
	Schedule_setEntryPlace(s, npc, n, 0, 0, 0);
	return 1;
}

unsigned char Schedule_insertEntry(Schedules *s, unsigned char npc)
{
	unsigned position;
	unsigned n;
	unsigned i;

	if (npc >= s->count || s->index[npc] + 8 == *(s->index + npc + 1))
		return 0;
	position = *(s->index + npc + 1);
	MoveFarMemory(s->entries + (position + 1), s->entries + position,
		(998 - position) * sizeof(ScheduleEntry));
	n = position - s->index[npc];
	for (i = npc + 1; i < s->count + 1; i++)
		s->index[i]++;
	Schedule_clearEntry(s, npc, n);
	return 1;
}

unsigned char Schedule_removeEntry(Schedules *s, unsigned char npc, unsigned char n)
{
	unsigned position;
	unsigned i;

	position = s->index[npc] + n;
	if (!Schedule_hasEntry(s, npc, n))
		return 0;
	MoveFarMemory(s->entries + position, s->entries + (position + 1),
		(998 - position) * sizeof(ScheduleEntry));
	for (i = npc + 1; i < s->count + 1; i++)
		s->index[i]--;
	return 1;
}

void Schedule_setEntryTime(Schedules *s, unsigned char npc, unsigned char n, unsigned char time)
{
	if (Schedule_hasEntry(s, npc, n))
		s->entries[s->index[npc] + n].time = time;
}

void Schedule_setEntryWorkType(Schedules *s, unsigned char npc, unsigned char n, unsigned char type)
{
	if (Schedule_hasEntry(s, npc, n))
		s->entries[s->index[npc] + n].type = type;
}

void Schedule_setEntryPlace(Schedules *s, unsigned char npc, unsigned char n, unsigned char x,
	unsigned char y, unsigned char region)
{
	if (Schedule_hasEntry(s, npc, n)) {
		s->entries[s->index[npc] + n].x = x;
		s->entries[s->index[npc] + n].y = y;
		s->entries[s->index[npc] + n].region = region;
	}
}
