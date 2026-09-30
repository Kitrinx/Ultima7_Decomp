#ifndef FILEIO_H
#define FILEIO_H

#include "memfile.h"

/* Numbers written to and read from a file at an offset, or where it is when the offset is -1. */
void WriteWord(MemoryFile *file, int value, long at);
void WriteByte(MemoryFile *file, char value, long at);
void WriteLong(MemoryFile *file, long value, long at);
int ReadWord(File *file, long at);
char ReadByte(File *file, long at);
long ReadLong(File *file, long at);

/* Reads the whole file into buffer. */
void ReadAll(File *file, void far *buffer);

#endif
