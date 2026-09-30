#ifndef FLXCACH_H
#define FLXCACH_H

#include "flex.h"

/* The entry table, kept in extended memory. */
struct FlexSpans {
	int32_t size;
	int32_t block;
	FlexSpans() { block = 0; }
	void alloc(int16_t bytes);
	void get(uint16_t i, void *dst);
	void put(uint16_t i, void *src);
	uint8_t loaded() { return block != 0; }
};

/* A Flex file whose entry table is read once and kept in extended memory. */
struct CachedFlex : FlexWriter {
	FlexSpans table;
	uint8_t getEntry(int16_t i, FlexEntry *entry);
	void writeEntry(int16_t i, FlexEntry *entry);
	void store(int16_t i, FlexEntry *entry);
	~CachedFlex();
};

#endif
