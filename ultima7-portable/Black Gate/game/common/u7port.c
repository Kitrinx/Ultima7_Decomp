/* Borland C library helpers the game used that standard C lacks. */

#include "u7port.h"

int16_t u7_stricmp(const char *a, const char *b)
{
	int16_t ca, cb;

	do {
		ca = (int16_t)toupper((uint8_t)*a++);
		cb = (int16_t)toupper((uint8_t)*b++);
	} while (ca == cb && ca != 0);
	return ca - cb;
}

int16_t u7_strnicmp(const char *a, const char *b, size_t n)
{
	int16_t ca = 0, cb = 0;

	while (n-- > 0) {
		ca = (int16_t)toupper((uint8_t)*a++);
		cb = (int16_t)toupper((uint8_t)*b++);
		if (ca != cb || ca == 0)
			break;
	}
	return ca - cb;
}

/* Negative numbers get a sign only in base 10; other bases show the raw bits. */
static char *FormatNumber(int32_t value, uint32_t bits, char *out, int16_t radix)
{
	char digits[34];
	uint32_t n;
	int16_t len = 0, i = 0;

	if (radix == 10 && value < 0) {
		out[i++] = '-';
		n = (uint32_t)(-(int64_t)value);
	} else {
		n = bits;
	}
	do {
		digits[len++] = "0123456789abcdefghijklmnopqrstuvwxyz"[n % (uint32_t)radix];
		n /= (uint32_t)radix;
	} while (n != 0);
	while (len > 0)
		out[i++] = digits[--len];
	out[i] = 0;
	return out;
}

char *u7_itoa(int16_t value, char *out, int16_t radix)
{
	return FormatNumber(value, (uint16_t)value, out, radix);
}

char *u7_ltoa(int32_t value, char *out, int16_t radix)
{
	return FormatNumber(value, (uint32_t)value, out, radix);
}

char *u7_strupr(char *s)
{
	char *p;

	for (p = s; *p; p++)
		*p = (char)toupper((uint8_t)*p);
	return s;
}

/* Borland's rand(), never reseeded by the game, so rolls repeat from run to run. */
static uint32_t BorlandSeed = 1;

static int16_t BorlandRand(void)
{
	BorlandSeed = BorlandSeed * 22695477u + 1u;
	return (int16_t)((BorlandSeed >> 16) & 0x7fff);
}

/* Borland's random(n): rand() scaled to 0..n-1. */
int16_t u7_random(int16_t range)
{
	return (int16_t)(((int32_t)BorlandRand() * range) / 0x8000);
}
