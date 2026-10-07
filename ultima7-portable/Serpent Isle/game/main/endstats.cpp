/* Serpent Isle SI.EXE, resident segment 13 (file offsets 0x010ce5 to 0x010dc3, 222 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
#include "chkfile.h"
#include "endstats.h"

GameDate::GameDate()
{
	year = month = day = 0;
}

/* 1 if all of it was read */
int16_t GameDate::load(char *name)
{
	DataFile f(name, FILE_OPEN);
	int32_t n;

	n = f.read(this, sizeof *this);
	return n == sizeof *this;
}

int16_t GameDate::save(char *name)
{
	DataFile f(name, FILE_CREATE);

	f.write(this, sizeof *this);
	return 1;
}
