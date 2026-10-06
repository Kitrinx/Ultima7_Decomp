/* Serpent Isle SI.EXE, resident segment 81 (file offsets 0x032a57 to 0x032d50, 761 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: sche_ov1.c */
#include "dosio.h"
#include "init.h"
#include "fileutil.h"
#include "oops.h"
#include "sche.h"
#include "memapi.h"
#include "sche_ov1.h"

#define MAX_SCHEDULE_ENTRIES 1000


/* Resizes the index for n NPCs, new NPCs getting no entries. */
unsigned char Schedule_resize(Schedules *s, unsigned n)
{
	unsigned far *index;
	unsigned i;

	if (s->count == n)
		return 1;
	index = (unsigned far *) AllocateFarHeap((n + 1) * sizeof(unsigned), 0);
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
unsigned char Schedule_read(Schedules *s, char *name)
{
	unsigned n;
	int fd;
	long size;
	int i, total;

	fd = DosOpen(name);
	if (fd == -1)
		return 0;
	DosRead(fd, -1L, sizeof n, &n);
	if (!Schedule_resize(s, n))
		return 0;
	DosRead(fd, -1L, (n + 1) * sizeof(unsigned), s->index);
	DosRead(fd, -1L, MAX_SCHEDULE_ENTRIES * sizeof(ScheduleEntry), s->entries);
	DosClose(fd);
	size = GetFileSize(name);
	size -= 2;
	size -= (s->count + 1) * sizeof(unsigned);
	size /= sizeof(ScheduleEntry);
	s->index[s->count] = size;
	total = 0;
	for (i = 0; i < s->count; i++)
		total += (unsigned char)(*(s->index + (unsigned char)i + 1) - s->index[(unsigned char)i]);
	if (size != total)
		FatalError("changeable schedules problem!", size, total, __FILE__, 108);
	return 1;
}

/* Sets up empty schedules for n NPCs, then reads the schedule file, or writes it if missing. */
void Schedule_init(Schedules *s, unsigned n)
{
	unsigned i;

	s->count = n;
	if (s->index != 0)
		FreeFarHeap(s->index);
	s->index = (unsigned far *) AllocateFarHeap((s->count + 1) * sizeof(unsigned), 0);
	if (s->index == 0)
		ReportOutOfFarMemory();
	if (s->entries != 0)
		FreeFarHeap(s->entries);
	s->entries = (ScheduleEntry far *) AllocateFarHeap(MAX_SCHEDULE_ENTRIES * sizeof(ScheduleEntry), 0);
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
