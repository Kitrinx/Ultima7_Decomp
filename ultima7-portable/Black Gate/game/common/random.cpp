/* The game's random numbers: each draw is the new seed's high word. */

#include "u7port.h"
#include "random.h"

extern "C" {
uint32_t RandomSeed = 0x0BAD0BAD;

/* Nearly seed * 65539 + 1, but the low word's carry is lost and the high word's own carry
 * wraps back into it. */
uint16_t NextRandom(void)
{
	uint32_t triple = RandomSeed * 3u;
	uint32_t high = (triple >> 16) + (RandomSeed & 0xffff);

	RandomSeed = (uint32_t) (uint16_t) (high + (high >> 16)) << 16 | (uint16_t) (triple + 1);
	return (uint16_t) (RandomSeed >> 16);
}

/* From lo to hi, either way round. A range of all 65536 values gives lo. */
uint16_t RandomFromRange(uint16_t lo, uint16_t hi)
{
	uint16_t r = NextRandom(), t, span;

	if (hi < lo) {
		t = hi;
		hi = lo;
		lo = t;
	}
	span = (uint16_t) (hi - lo + 1);
	return (uint16_t) ((span ? r % span : 0) + lo);
}

/* The low word is forced odd. */
void SetRandomSeed(int32_t seed)
{
	RandomSeed = (uint32_t) seed | 1;
}

int32_t GetRandomSeed(void)
{
	return (int32_t) RandomSeed;
}

int16_t GenerateRandomIntegerInRange(int16_t n)
{
	uint16_t r = NextRandom();

	return (int16_t) (n ? r % (uint16_t) n : 0);
}
}

extern "C" void ResetRandomGlobals(void)
{
	RandomSeed = 0x0BAD0BAD;
}
