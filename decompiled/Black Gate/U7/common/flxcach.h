#ifndef FLXCACH_H
#define FLXCACH_H

#include "flex.h"

/* The entry table, kept in extended memory. */
struct FlexSpans {
	long size;
	long block;
	FlexSpans() { block = 0; }
	void alloc(int bytes);
	void get(unsigned i, void *dst);
	void put(unsigned i, void *src);
	unsigned char loaded() { return block != 0; }
};

/* A Flex file whose entry table is read once and kept in extended memory. */
struct CachedFlex : FlexWriter {
	FlexSpans table;
	unsigned char getEntry(int i, FlexEntry *entry);
	void writeEntry(int i, FlexEntry *entry);
	void store(int i, FlexEntry *entry);
	~CachedFlex();
};

#endif
