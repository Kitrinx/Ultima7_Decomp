/* Black Gate U7.EXE, resident segment 91 (file offsets 0x032208 to 0x0322f9, 241 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
#include <stdlib.h>
#include "dosio.h"
#include "easyfile.h"
#include "datanode.h"
#include "random.h"
#include <new>

static const char RandSeedFileNameStart[] = "RANDSEED.DAT";
static char RandSeedFileName[sizeof RandSeedFileNameStart];

/* the random number seed, carried from one session to the next */
struct RandSeed : DataNode {
	char *name() { return RandSeedFileName; }
	void load(char *dir);
	void save(char *dir);
};

RandSeed RandSeedFile;

void RandSeed::save(char *dir)
{
	int32_t seed;
	int16_t fd;

	fd = CreateFileOrFail(BuildPath(dir, RandSeedFileName, 0));
	seed = GetRandomSeed();
	DosWrite(fd, -INT32_C(1), INT32_C(4), &seed);
	DosClose(fd);
}

void RandSeed::load(char *dir)
{
	int32_t seed;
	int16_t fd;

	fd = DosOpen(BuildPath(dir, RandSeedFileName, 0));
	if (fd != -1) {
		DosRead(fd, -INT32_C(1), INT32_C(4), &seed);
		SetRandomSeed(seed);
	}
	DosClose(fd);
}

int16_t RollRandom(int16_t n)
{
	return random(n);
}

extern "C" void ResetRandseedGlobals(void)
{
	memcpy(RandSeedFileName, RandSeedFileNameStart, sizeof RandSeedFileName);
}

extern "C" void ConstructRandseedGlobals(void)
{
	new (&RandSeedFile) RandSeed();
}
