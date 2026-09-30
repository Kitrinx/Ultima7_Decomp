/* Black Gate U7.EXE, resident segment 100 (file offsets 0x03658a to 0x036688, 254 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include <mem.h>
#include "bitarray.h"

void BitArray::setBit(int n)
{
	int i = n >> 3;

	if (n >= 0 && size >= i)
		data[i] |= 1 << (n & 7);
}

void BitArray::clearBit(int n)
{
	int i = n >> 3;

	if (n >= 0 && size >= i)
		data[i] &= ~(1 << (n & 7));
}

BitArray::~BitArray()
{
	delete data;
}

/* room for the given number of bits, all clear */
unsigned char BitArray::allocate(int bits)
{
	size = (bits + 7) >> 3;
	data = new unsigned char[size];
	if (data != 0)
		clear();
	return data != 0;
}

void BitArray::clear()
{
	memset(data, 0, size);
}
