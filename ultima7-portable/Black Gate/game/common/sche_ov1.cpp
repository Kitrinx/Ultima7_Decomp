/* Black Gate U7.EXE, overlay segment 256 (file offsets 0x0791e0 to 0x0794f0, 784 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "dosio.h"
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
	int32_t size;
	int16_t fd;

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
	return 1;
}

/* Writes the schedules to the file name. */
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
