#ifndef FLEXVOO_H
#define FLEXVOO_H

#include "flex.h"

/* A Flex file read whole into voodoo memory and served from there. */
struct VoodooFlex : Flex {
	long size;
	long addr;
	VoodooFlex() { addr = 0; }
	void alloc(long n);
	unsigned char load(char *name);
	void release();
	unsigned char getEntry(int n, FlexEntry *e);
	unsigned char readEntryToVoodoo(FlexEntry *e, long block, unsigned char quiet);
	unsigned char readEntry(FlexEntry *e, void far *buf, unsigned char quiet);
};

#endif
