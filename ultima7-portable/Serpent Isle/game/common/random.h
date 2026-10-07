#ifndef RANDOM_H
#define RANDOM_H

#ifdef __cplusplus
extern "C" {
#endif

/* a random number below n, or 0 when n is 0 */
int16_t GenerateRandomIntegerInRange(int16_t n);

int32_t GetRandomSeed(void);
void SetRandomSeed(int32_t seed);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* True one time in n. */
inline uint8_t RollChance(int16_t n) { return GenerateRandomIntegerInRange(n) == 0; }

/* A number from low to high, or high when the range is empty. */
inline int16_t RandomBetween(int16_t low, int16_t high)
{
	return high >= low ? GenerateRandomIntegerInRange(high - low + 1) + low : high;
}
#endif

#ifdef __cplusplus
struct RandSeed;

extern RandSeed RandSeedFile;
int16_t RollRandom(int16_t n);
#endif

#endif
