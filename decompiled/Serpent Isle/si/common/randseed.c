/* Serpent Isle SI.EXE, resident segment 65 (file offsets 0x02c44e to 0x02c53f, 241 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include <stdlib.h>
#include "dosio.h"
#include "easyfile.h"
#include "datanode.h"
#include "random.h"

static char RandSeedFileName[] = "RANDSEED.DAT";

/* the random number seed, carried from one session to the next */
struct RandSeed : DataNode {
	char *name() { return RandSeedFileName; }
	void load(char *dir);
	void save(char *dir);
};

RandSeed RandSeedFile;

void RandSeed::save(char *dir)
{
	long seed;
	int fd;

	fd = CreateFileOrFail(BuildPath(dir, RandSeedFileName, 0));
	seed = GetRandomSeed();
	DosWrite(fd, -1L, 4L, &seed);
	DosClose(fd);
}

void RandSeed::load(char *dir)
{
	long seed;
	int fd;

	fd = DosOpen(BuildPath(dir, RandSeedFileName, 0));
	if (fd != -1) {
		DosRead(fd, -1L, 4L, &seed);
		SetRandomSeed(seed);
	}
	DosClose(fd);
}

int RollRandom(int n)
{
	return random(n);
}
