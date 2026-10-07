/* Serpent Isle SI.EXE, overlay segment 216 (file offsets 0x05b0d0 to 0x05b417, 839 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -Y rebuilds it byte for byte as C++.
 */

/* path: chngsche.c */
#include "u7port.h"
#include <new>
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

char *const ScheduleSaveName = "SCHEDULE.DAT";
ScheduleSaver ScheduleFile;

char *ScheduleSaver::name()
{
	return ScheduleSaveName;
}

void ScheduleSaver::load(char *dir)
{
	DataFile f(BuildPath(dir, ScheduleSaveName, 0), 1);
	int16_t n;
	uint16_t total;
	int16_t i;

	n = f.readLong();
	if (!Schedule_resize(&ScheduleTable, n))
		FatalError("resize failed", __FILE__, 81);
	f.read(ScheduleTable.index, (n + 1) * sizeof(uint16_t));
	f.read(ScheduleTable.entries, INT32_C(4000));
	total = 0;
	for (i = 0; i < ScheduleTable.count; i++)
		total += (uint8_t)(*(ScheduleTable.index + (uint8_t)i + 1) -
			ScheduleTable.index[(uint8_t)i]);
	ScheduleTable.index[ScheduleTable.count] = total;
}

void ScheduleSaver::save(char *dir)
{
	DataFile f(BuildPath(dir, ScheduleSaveName, 0), 0);
	int32_t n;

	n = ScheduleTable.count;
	f.write(&n, INT32_C(4));
	f.write(ScheduleTable.index, (ScheduleTable.count + 1) * sizeof(uint16_t));
	f.write(ScheduleTable.entries, ScheduleTable.index[ScheduleTable.count] * sizeof(ScheduleEntry));
}

/* Puts an NPC's schedules back as the game first shipped them. */
void RevertSchedule(int16_t npc)
{
	uint16_t first = 0;
	uint16_t last = 0;
	ScheduleEntry entry;
	int8_t n;
	int16_t fd;

	fd = DosOpen("static\\schedule.dat");
	if (fd == -1)
		FatalError("Cannot open schedule.dat", __FILE__, 110);
	DosSeek(fd, (npc - 1) * sizeof(uint16_t) + 4, 0);
	DosRead(fd, -INT32_C(1), INT32_C(2), &first);
	DosRead(fd, -INT32_C(1), INT32_C(2), &last);
	DosSeek(fd, ScheduleTable.count * sizeof(uint16_t) + first * sizeof(ScheduleEntry) + 4, 0);
	for (n = (uint8_t)(*(ScheduleTable.index + (uint8_t)npc + 1) -
			ScheduleTable.index[(uint8_t)npc]) - 1; n >= 0; n--)
		Schedule_removeEntry(&ScheduleTable, npc, n);
	for (n = 0; first != last; first++, n++) {
		DosRead(fd, -INT32_C(1), INT32_C(4), &entry);
		Schedule_insertEntry(&ScheduleTable, npc);
		Schedule_setEntryTime(&ScheduleTable, npc, n, entry.time);
		Schedule_setEntryWorkType(&ScheduleTable, npc, n, entry.type);
		Schedule_setEntryPlace(&ScheduleTable, npc, n, entry.x, entry.y, entry.region);
	}
	DosClose(fd);
}

extern "C" void ResetChngscheGlobals(void)
{
	memset((void *)&ScheduleFile, 0, sizeof(ScheduleFile));
}

extern "C" void ConstructChngscheGlobals(void)
{
	new (&ScheduleFile) ScheduleSaver();
}
