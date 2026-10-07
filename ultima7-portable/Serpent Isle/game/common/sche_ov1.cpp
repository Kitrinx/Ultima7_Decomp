/* Serpent Isle SI.EXE, resident segment 81 (file offsets 0x032a57 to 0x032d50, 761 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: sche_ov1.c */
#include "u7port.h"
#include "dosio.h"
#include "init.h"
#include "fileutil.h"
#include "oops.h"
#include "sche.h"
#include "memapi.h"
#include "sche_ov1.h"

#define MAX_SCHEDULE_ENTRIES 1000


/* Resizes the index for n NPCs, new NPCs getting no entries. */
uint8_t Schedule_resize(Schedules *s, uint16_t n)
{
	uint16_t *index;
	uint16_t i;

	if (s->count == n)
		return 1;
	index = (uint16_t *) AllocateFarHeap((n + 1) * sizeof(uint16_t), 0);
	if (s->count < n) {
		for (i = 0; i < s->count + 1; i++)
			index[i] = s->index[i];
		for (i = s->count + 1; i < n + 1; i++)
			index[i] = s->index[s->count];
	} else {
		for (i = 0; i < n + 1; i++)
			index[i] = s->index[i];
	}
	FreeFarHeap(s->index);
	s->index = index;
	s->count = n;
	return 1;
}

/* Reads the schedules from the file name. */
uint8_t Schedule_read(Schedules *s, char *name)
{
	uint16_t n;
	int16_t fd;
	int32_t size;
	int16_t i, total;

	fd = DosOpen(name);
	if (fd == -1)
		return 0;
	DosRead(fd, -INT32_C(1), sizeof n, &n);
	if (!Schedule_resize(s, n))
		return 0;
	DosRead(fd, -INT32_C(1), (n + 1) * sizeof(uint16_t), s->index);
	DosRead(fd, -INT32_C(1), MAX_SCHEDULE_ENTRIES * sizeof(ScheduleEntry), s->entries);
	DosClose(fd);
	size = GetFileSize(name);
	size -= 2;
	size -= (s->count + 1) * sizeof(uint16_t);
	size /= sizeof(ScheduleEntry);
	s->index[s->count] = size;
	total = 0;
	for (i = 0; i < s->count; i++)
		total += (uint8_t)(*(s->index + (uint8_t)i + 1) - s->index[(uint8_t)i]);
	if (size != total)
		FatalError("changeable schedules problem!", size, total, __FILE__, 108);
	return 1;
}

/* Sets up empty schedules for n NPCs, then reads the schedule file, or writes it if missing. */
void Schedule_init(Schedules *s, uint16_t n)
{
	uint16_t i;

	s->count = n;
	if (s->index != 0)
		FreeFarHeap(s->index);
	s->index = (uint16_t *) AllocateFarHeap((s->count + 1) * sizeof(uint16_t), 0);
	if (s->index == 0)
		ReportOutOfFarMemory();
	if (s->entries != 0)
		FreeFarHeap(s->entries);
	s->entries = (ScheduleEntry *) AllocateFarHeap(MAX_SCHEDULE_ENTRIES * sizeof(ScheduleEntry), 0);
	if (s->entries == 0)
		ReportOutOfFarMemory();
	for (i = 0; i < s->count + 1; i++)
		s->index[i] = 0;
	if (!FileExists(ScheduleFileName))
		Schedule_write(s, ScheduleFileName);
	else
		Schedule_read(s, ScheduleFileName);
}

void InitSchedules(void)
{
	Schedule_init(&ScheduleTable, 256);
}
