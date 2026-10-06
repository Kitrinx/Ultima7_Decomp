#ifndef IFF_H
#define IFF_H

#include "memfile.h"

/* "FORM", its size and its type: where a form's chunks begin. */
#define FORM_HEADER     12

/* A chunk's header as read from the file, with where the chunk starts and ends. */
struct ChunkHeader {
	long start;
	char id[5];
	long size;
	long end;
};

/* A FORM or CAT entered, kept on a stack so it can be left again. */
struct IffChunk {
	IffChunk *prev;
	long start;
	char id[5];
	long size;
	long end;
	IffChunk(IffChunk *outer, ChunkHeader *header);
	~IffChunk();
};

/* An IFF file: FORM and CAT chunks entered and left like directories, chunks found by id. */
struct IffFile : MemoryFile {
	long offset;
	int depth;
	IffChunk *form;
	ChunkHeader chunk;
	long records;
	long recordSize;
	int loaded;
	long formEnd;
	long table;
	long entries;
	int columns;
	int rows;
	long mark;
	long unused1;
	long entrySize;
	long unused2;
	IffFile() { form = 0; }
	IffFile(char *name, char mode);
	~IffFile();
	void describe();
	unsigned char validate();
	void leaveFinished();
	void rewind();
	long readSize();
	int readForm();
	int readList();
	int findChunk(char *id);
	int findForm(char *id);
	int findList(char *id);
	void startTable();
	void startGrid(int width, int height);
	int findEntry(long index);
	int findCell(int column, int row);
	void skipTable(int count);
	void load();
	void setOffset(long at);
	void read(long size, void far *buffer);
	int readWord();
	long readLong();
	long peekLong();
	long readBytes(void far *buffer, long size);
	long readChunk(void far *buffer);
	long loadChunk(long block);
	int readByte();
	long readAt(void far *buffer, long at, long size);
	void readId();
	void readHeader();
	void skipChunk();
	void rewindForm();
	void enterForm();
	void enterList();
	void skipForm();
	void leaveForm();
	void leaveList();
	int isId(char *id);
	long setRecordSize(long size);
	void readRecords(void *buffer);
	int atChunkEnd();
	int atFormEnd();
	int atFileEnd();
	int atEnd();
};

char *GetFormType(char *name);
void DumpIff(char *name);

#endif
