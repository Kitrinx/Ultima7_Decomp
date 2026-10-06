#ifndef BITARRAY_H
#define BITARRAY_H

/* a block of near memory and its size, used as an array of bits */
struct BitArray {
	int size;
	unsigned char *data;
	BitArray() { size = 0; data = 0; }
	~BitArray();
	unsigned char test(int n) { return data[n >> 3] & (1 << (n & 7)); }
	void setBit(int n);
	void clearBit(int n);
	unsigned char allocate(int bits);
	void clear();
};

#endif
