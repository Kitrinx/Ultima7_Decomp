/* Serpent Isle SI.EXE, overlay segment 216 (file offsets 0x05b0d0 to 0x05b417, 839 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -Y rebuilds it byte for byte as C++.
 */

/* path: chngsche.c */
#include "dosio.h"
#include "init.h"
#include "easyfile.h"
#include "chkfile.h"
#include "datanode.h"
#include "sche.h"
#include "sche_ov1.h"
#include "schedit.h"

/* The changed schedules, kept in SCHEDULE.DAT of a saved game. */
struct ScheduleSaver : DataNode {
	char *name();
	void load(char *dir);
	void save(char *dir);
};

char *ScheduleSaveName = "SCHEDULE.DAT";
ScheduleSaver ScheduleFile;

char *ScheduleSaver::name()
{
	return ScheduleSaveName;
}

void ScheduleSaver::load(char *dir)
{
	DataFile f(BuildPath(dir, ScheduleSaveName, 0), 1);
	int n;
	unsigned total;
	int i;

	n = f.readLong();
	if (!Schedule_resize(&ScheduleTable, n))
		FatalError("resize failed", __FILE__, 81);
	f.read(ScheduleTable.index, (n + 1) * sizeof(unsigned));
	f.read(ScheduleTable.entries, 4000L);
	total = 0;
	for (i = 0; i < ScheduleTable.count; i++)
		total += (unsigned char)(*(ScheduleTable.index + (unsigned char)i + 1) -
			ScheduleTable.index[(unsigned char)i]);
	ScheduleTable.index[ScheduleTable.count] = total;
}

void ScheduleSaver::save(char *dir)
{
	DataFile f(BuildPath(dir, ScheduleSaveName, 0), 0);
	long n;

	n = ScheduleTable.count;
	f.write(&n, 4L);
	f.write(ScheduleTable.index, (ScheduleTable.count + 1) * sizeof(unsigned));
	f.write(ScheduleTable.entries, ScheduleTable.index[ScheduleTable.count] * sizeof(ScheduleEntry));
}

/* Puts an NPC's schedules back as the game first shipped them. */
void RevertSchedule(int npc)
{
	unsigned first = 0;
	unsigned last = 0;
	ScheduleEntry entry;
	char n;
	int fd;

	fd = DosOpen("static\\schedule.dat");
	if (fd == -1)
		FatalError("Cannot open schedule.dat", __FILE__, 110);
	DosSeek(fd, (npc - 1) * sizeof(unsigned) + 4, 0);
	DosRead(fd, -1L, 2L, &first);
	DosRead(fd, -1L, 2L, &last);
	DosSeek(fd, ScheduleTable.count * sizeof(unsigned) + first * sizeof(ScheduleEntry) + 4, 0);
	for (n = (unsigned char)(*(ScheduleTable.index + (unsigned char)npc + 1) -
			ScheduleTable.index[(unsigned char)npc]) - 1; n >= 0; n--)
		Schedule_removeEntry(&ScheduleTable, npc, n);
	for (n = 0; first != last; first++, n++) {
		DosRead(fd, -1L, 4L, &entry);
		Schedule_insertEntry(&ScheduleTable, npc);
		Schedule_setEntryTime(&ScheduleTable, npc, n, entry.time);
		Schedule_setEntryWorkType(&ScheduleTable, npc, n, entry.type);
		Schedule_setEntryPlace(&ScheduleTable, npc, n, entry.x, entry.y, entry.region);
	}
	DosClose(fd);
}
