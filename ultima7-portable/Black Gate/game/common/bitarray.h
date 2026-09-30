#ifndef BITARRAY_H
#define BITARRAY_H

/* a block of near memory and its size, used as an array of bits */
struct BitArray {
	int16_t size;
	uint8_t *data;
	BitArray() { size = 0; data = 0; }
	~BitArray();
	uint8_t test(int16_t n) { return data[n >> 3] & (1 << (n & 7)); }
	void setBit(int16_t n);
	void clearBit(int16_t n);
	uint8_t allocate(int16_t bits);
	void clear();
};

#endif
