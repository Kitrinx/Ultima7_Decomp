#ifndef FARBUF_H
#define FARBUF_H

/* A block of far heap memory, filled from a file or a Flex entry; owned is set once allocated here. */
struct FarBuffer {
	void far *data;
	char owned;
	FarBuffer() { data = 0; owned = 0; }
	~FarBuffer() { release(); }
	void far *allocate(long size);
	long load(char *name);
	long load(char *flexName, int i);
	void release();
};

#endif
