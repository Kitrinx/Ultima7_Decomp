/* Black Gate ENDGAME.EXE, resident segment 15 (file offsets 0x00bef8 to 0x00c053, 347 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- -d rebuilds it byte for byte as C++.
 */

#include "memfile.h"
#include "endstats.h"

GameDate::GameDate()
{
	year = month = day = 0;
}

/* 1 if all of it was read */
int GameDate::load(char *name)
{
	long n;

	if (!DosFileExists(name))
		return 0;
	DiskFile f;
	if (!f.open(name, FILE_READ)) {
		return 0;
	}
	n = f.read(this, sizeof *this);
	return n == sizeof *this;
}

/* 1 if all of it was written */
int GameDate::save(char *name)
{
	long n;
	DiskFile f;

	if (!f.open(name, FILE_CREATE)) {
		return 0;
	}
	n = f.write(this, sizeof *this);
	return n == sizeof *this;
}
