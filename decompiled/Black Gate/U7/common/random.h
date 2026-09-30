#ifndef RANDOM_H
#define RANDOM_H

#ifdef __cplusplus
extern "C" {
#endif

/* a random number below n, or 0 when n is 0 */
int far GenerateRandomIntegerInRange(int n);

long GetRandomSeed(void);
void SetRandomSeed(long seed);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* True one time in n. */
inline unsigned char RollChance(int n) { return GenerateRandomIntegerInRange(n) == 0; }

/* A number from low to high, or high when the range is empty. */
inline int RandomBetween(int low, int high)
{
	return high >= low ? GenerateRandomIntegerInRange(high - low + 1) + low : high;
}
#endif

#ifdef __cplusplus
struct RandSeed;

extern RandSeed RandSeedFile;
int RollRandom(int n);
#endif

#endif
