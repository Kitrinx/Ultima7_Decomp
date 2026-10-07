/* Serpent Isle SI.EXE, overlay segment 256 (file offsets 0x06f070 to 0x06f3e3, 883 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Filename inferred from its schedule-editing role.
 */

#include "u7port.h"
#include "dosio.h"
#include "fileutil.h"
#include "oops.h"
#include "sche.h"
#include "memapi.h"
#include "schedit.h"

uint8_t Schedule_write(Schedules *s, char *name)
{
	int16_t fd;

	fd = DosCreate(name);
	if (fd == -1)
		return 0;
	DosWrite(fd, INT32_C(0), sizeof s->count, &s->count);
	DosWrite(fd, -INT32_C(1), (s->count + 1) * sizeof(uint16_t), s->index);
	DosWrite(fd, -INT32_C(1), s->index[s->count] * sizeof(ScheduleEntry), s->entries);
	DosClose(fd);
	return 1;
}

uint8_t Schedule_clearEntry(Schedules *s, uint8_t npc, uint8_t n)
{
	if (!Schedule_hasEntry(s, npc, n))
		return 0;
	Schedule_setEntryTime(s, npc, n, 0);
	Schedule_setEntryWorkType(s, npc, n, 0);
	Schedule_setEntryPlace(s, npc, n, 0, 0, 0);
	return 1;
}

uint8_t Schedule_insertEntry(Schedules *s, uint8_t npc)
{
	uint16_t position;
	uint16_t n;
	uint16_t i;

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

uint8_t Schedule_removeEntry(Schedules *s, uint8_t npc, uint8_t n)
{
	uint16_t position;
	uint16_t i;

	position = s->index[npc] + n;
	if (!Schedule_hasEntry(s, npc, n))
		return 0;
	MoveFarMemory(s->entries + position, s->entries + (position + 1),
		(998 - position) * sizeof(ScheduleEntry));
	for (i = npc + 1; i < s->count + 1; i++)
		s->index[i]--;
	return 1;
}

void Schedule_setEntryTime(Schedules *s, uint8_t npc, uint8_t n, uint8_t time)
{
	if (Schedule_hasEntry(s, npc, n))
		s->entries[s->index[npc] + n].time = time;
}

void Schedule_setEntryWorkType(Schedules *s, uint8_t npc, uint8_t n, uint8_t type)
{
	if (Schedule_hasEntry(s, npc, n))
		s->entries[s->index[npc] + n].type = type;
}

void Schedule_setEntryPlace(Schedules *s, uint8_t npc, uint8_t n, uint8_t x,
	uint8_t y, uint8_t region)
{
	if (Schedule_hasEntry(s, npc, n)) {
		s->entries[s->index[npc] + n].x = x;
		s->entries[s->index[npc] + n].y = y;
		s->entries[s->index[npc] + n].region = region;
	}
}
