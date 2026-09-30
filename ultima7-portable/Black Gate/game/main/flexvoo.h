#ifndef FLEXVOO_H
#define FLEXVOO_H

#include "flex.h"

/* A Flex file read whole into voodoo memory and served from there. */
struct VoodooFlex : Flex {
	int32_t size;
	int32_t addr;
	VoodooFlex() { addr = 0; }
	void alloc(int32_t n);
	uint8_t load(char *name);
	void release();
	uint8_t getEntry(int16_t n, FlexEntry *e);
	uint8_t readEntryToVoodoo(FlexEntry *e, int32_t block, uint8_t quiet);
	uint8_t readEntry(FlexEntry *e, void *buf, uint8_t quiet);
};

#endif
