#ifndef MEMFILE_H
#define MEMFILE_H

#include "file.h"

#define DEFAULT_BUFFER  256

/* A File that can grow in the middle, moving its tail through a far-memory buffer. */
struct MemoryFile : File {
	void far *buffer;
	long bufferSize;
	MemoryFile() { clear(); }
	MemoryFile(DosFile *d, long size);
	~MemoryFile();
	void clear() { bufferSize = DEFAULT_BUFFER; buffer = 0; }
	void describe();
	void load();
	DosFile *createHandle();
	long write(void far *buf, long size, long at = -1);
	long insert(void far *buf, long size, long at = -1);
};

/* The everyday file. */
struct DiskFile : MemoryFile {
	DiskFile() {}
	~DiskFile() {}
	void describe() { MemoryFile::describe(); }
};

#endif
