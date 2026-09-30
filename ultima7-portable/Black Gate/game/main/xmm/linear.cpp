/* Linear memory access and whole-screen copies.
 * Every address here is an arena offset; the flags that once marked an address as
 * segment:offset are ignored.
 */

#include "u7port.h"
#include "arena.h"
#include "lowlevel.h"

#define SCREEN_BYTES (320 * 200)

static int16_t Get16(const uint8_t *p) { return (int16_t) (p[0] | p[1] << 8); }

/* A frame: right, left, top and bottom extents from the hot spot, then spans of
 * length * 2 (+1 when run-encoded), x and y. pos and point are x, y pairs. */
int8_t IsPointInFrame(void *shape, void *pos, void *point, int16_t flags)
{
	const uint8_t *frame = (const uint8_t *) shape;
	const int16_t *at = (const int16_t *) pos;
	const int16_t *pt = (const int16_t *) point;
	int16_t length, x, y, dx;
	uint8_t run, count;

	if (pt[1] < (int16_t) (at[1] - Get16(frame + 4)) || pt[1] > (int16_t) (Get16(frame + 6) + at[1]))
		return 0;
	if (pt[0] < (int16_t) (at[0] - Get16(frame + 2)) || pt[0] > (int16_t) (Get16(frame) + at[0]))
		return 0;
	frame += 8;
	for (;;) {
		length = (int16_t) ((uint16_t) Get16(frame) >> 1);
		x = Get16(frame + 2) + at[0];
		y = Get16(frame + 4) + at[1];
		if (y == pt[1]) {
			dx = pt[0] - x;
			if (dx < 0)
				return 0;
			if (dx < length)
				return 1;
		}
		if (length == 0)
			return 0;
		if ((Get16(frame) & 1) == 0) {
			frame += 6 + (uint16_t) length;
			continue;
		}
		frame += 6;
		do {
			run = frame[0];
			frame += 2;
			count = run >> 1;
			length -= count;
			if ((run & 1) == 0)
				frame += count - 1;
		} while (length != 0);
	}
}

int32_t SumLinearBytes(int32_t src, int32_t numBytes)
{
	uint32_t sum = 0, n;

	for (n = (uint32_t) numBytes; n != 0; n--)
		sum += LinearGet8(src++);
	return (int32_t) sum;
}

/* Stops after a zero or maxLen bytes; a copy cut short gets no terminator. */
void CopyLinearStringN(uint32_t dest, uint32_t src, int16_t maxLen, int16_t flags)
{
	uint32_t n = (uint16_t) maxLen;
	uint8_t c;

	do {
		c = LinearGet8(src++);
		LinearPut8(dest++, c);
	} while (--n != 0 && c != 0);
}

void CopyLinearStringOut(char *dest, uint32_t src, int16_t maxLen)
{
	uint32_t n = (uint16_t) maxLen;
	char c;

	do {
		c = (char) LinearGet8(src++);
		*dest++ = c;
	} while (--n != 0 && c != 0);
}

void CopyLinearString(uint32_t dest, uint32_t src, int16_t flags)
{
	uint8_t c;

	do {
		c = LinearGet8(src++);
		LinearPut8(dest++, c);
	} while (c != 0);
}

int32_t PeekLong(int32_t address) { return (int32_t) LinearGet32(address); }
void PokeLong(int32_t address, int32_t value) { LinearPut32(address, (uint32_t) value); }
int16_t PeekWord(uint32_t address) { return (int16_t) LinearGet16(address); }
void PokeWord(int32_t address, int16_t value) { LinearPut16(address, (uint16_t) value); }
uint8_t PeekByte(int32_t address) { return LinearGet8(address); }
void PokeByte(int32_t address, int8_t value) { LinearPut8(address, (uint8_t) value); }

/* Bit n of a bit array, counting from 1. */
int16_t TestLinearBit(int32_t bits, uint16_t n)
{
	uint32_t i = (uint32_t) n - 1;

	return (LinearGet8(bits + (i >> 3)) >> (i & 7)) & 1;
}

void SetLinearBit(int32_t bits, uint16_t n)
{
	uint32_t i = (uint32_t) n - 1;

	LinearPut8(bits + (i >> 3), LinearGet8(bits + (i >> 3)) | 1 << (i & 7));
}

/* Counts from 0 but steps sixteen bits a byte; bit numbers that are multiples of 8 change nothing. */
void ClearLinearBit(int32_t bits, uint16_t n)
{
	int32_t at = bits + (n >> 4);
	uint16_t bit = (n & 7) - 1;

	LinearPut8(at, (uint8_t) (LinearGet8(at) & ~(1 << (bit & 15))));
}

void FillLinear(int32_t dst, int8_t fillByte, int32_t bytes, int16_t flags)
{
	memset(LINEAR(dst), (uint8_t) fillByte, (uint32_t) bytes);
}

void FillLinearRect(int32_t dst, uint8_t fillByte, uint16_t cols, uint16_t pitch, uint16_t rows)
{
	uint32_t n = rows;

	do {
		memset(LINEAR(dst), fillByte, cols);
		dst += pitch;
	} while (--n != 0);
}

int16_t MoveLinear(int32_t dst, int32_t src, uint32_t bytes, int16_t flags)
{
	memmove(LINEAR(dst), LINEAR(src), bytes);
	return 0;
}

int16_t CopyFarToLinear(int32_t dst, void *src, int32_t bytes)
{
	memcpy(LINEAR(dst), src, (uint32_t) bytes);
	return 0;
}

int16_t CopyLinearToFar(void *dst, int32_t src, int32_t bytes)
{
	memcpy(dst, LINEAR(src), (uint32_t) bytes);
	return 0;
}

void FillScreen(uint8_t color)
{
	memset(ScreenPixels(), color, SCREEN_BYTES);
}

void CopyScreen(int32_t src, int32_t dst)
{
	memmove(LINEAR(dst), LINEAR(src), SCREEN_BYTES);
}

void CopyScreenWords(int32_t src, int32_t dst)
{
	memmove(LINEAR(dst), LINEAR(src), SCREEN_BYTES);
}

void ClearScreen(int16_t color)
{
	memset(ScreenPixels(), (uint8_t) color, SCREEN_BYTES);
}

/* Passes every screen pixel through a 256-byte table. */
void RemapScreen(int32_t table, int16_t flags)
{
	uint8_t *p = ScreenPixels();
	const uint8_t *map = LINEAR(table);
	int32_t i;

	for (i = 0; i < SCREEN_BYTES; i++)
		p[i] = map[p[i]];
}

void FillScreenBuffer(int32_t buffer, uint8_t color)
{
	memset(LINEAR(buffer), color, SCREEN_BYTES);
}
