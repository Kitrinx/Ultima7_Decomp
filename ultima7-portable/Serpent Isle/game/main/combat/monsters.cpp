/* Serpent Isle SI.EXE, overlay segment 359 (file offsets 0x0b4480 to 0x0b45ef, 367 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
#include "lowlevel.h"
#include "dosio.h"
#include "oops.h"
#include "makemojo.h"
#include "voolook.h"
#include "monsters.h"

char MonstersFileName[] = "monsters.dat";
MonsterTable MonsterRecords;
uint8_t MonsterCount = 0;
ShapeLookup MonsterLookup;

void MonsterTable::load(char *name)
{
	int16_t fd;
	MonsterRecord record;

	fd = DosOpen(name);
	if (fd == -1)
		ReportFileNotFound(name);
	if (DosRead(fd, INT32_C(0), INT32_C(1), &MonsterCount) != 1)
		ReportFileNotFound(name);
	if (!MakeMojo(MonsterCount + 1, INT32_C(25), &base, &count))
		ReportOutOfVoodooMemory();
	FillFarBytes(&record, 25, 0);
	record.range = 3;
	record.walk = 1;
	record.strength = 10;
	record.dexterity = 10;
	record.intelligence = 10;
	record.combat = 10;
	record.alignment = 0;
	record.category = 3;
	CheckMojoBounds(count, INT32_C(0));
	CopyFarToLinear(base, &record, INT32_C(25));
	if (!ReadMojo(fd, -INT32_C(1), INT32_C(1), MonsterCount, count, base, INT32_C(25)))
		ReportFileNotFound(name);
	DosClose(fd);
}

extern "C" void ResetMonstersGlobals(void)
{
	strcpy(MonstersFileName, "monsters.dat");
	memset((void *)&MonsterRecords, 0, sizeof(MonsterRecords));
	MonsterCount = 0;
	memset(&MonsterLookup, 0, sizeof(MonsterLookup));
}

extern "C" void ConstructMonstersGlobals(void)
{
	new (&MonsterRecords) MonsterTable();
}
