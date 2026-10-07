#ifndef FARBUF_H
#define FARBUF_H

#include "dosio.h"

namespace Shared {

/* A block of far heap memory, filled from a file or a Flex entry; owned is set once allocated here. */
struct FarBuffer {
	void *data;
	int8_t owned;
	FarBuffer() { data = 0; owned = 0; }
	FarBuffer(char *name) { data = 0; owned = 0; load(name); }
	FarBuffer(char *name, int16_t entry) { data = 0; owned = 0; load(name, entry); }
	void *get() { return data; }
	~FarBuffer() { release(); }
	void *allocate(int32_t size);
	int32_t load(char *name);
	int32_t load(char *flexName, int16_t i);
	void release();
	/* The data's linear address, as shapes are drawn from. */
	int32_t linear() { return PointerToLinear(data); }
};

}

#endif
