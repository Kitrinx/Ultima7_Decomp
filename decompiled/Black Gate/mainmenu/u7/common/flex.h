#ifndef FLEX_H
#define FLEX_H

/* Where an object sits in a Flex file, and how long it is. */
struct FlexEntry {
	long offset;
	long size;
	FlexEntry() {}
	FlexEntry(long start, long length) { offset = start; size = length; }
	unsigned char empty() { return size == 0; }
	void clear() { size = 0; offset = 0; }
	int operator==(FlexEntry other);
};

/* The 128 bytes at the start of a Flex file; the entry table follows. */
struct FlexHeader {
	char title[81];
	unsigned char eof;
	int magic;
	long count;
	long version;
	long saveVersion, unusedField1;
	long waste;
	long checksum;
	char wasteFlag;
	char packed;
	int unusedField2;
	char unusedField3[16];

	FlexHeader() { clear(); }
	unsigned char valid() { return magic == -1; }
	unsigned char unchanged() { return computeChecksum() == checksum; }
	void setFlexMarks();
	void clear();
	long computeChecksum();
	void read(int fd);
	void writeIfChanged(int fd);
	void create(int entries, char *text);
	void setTitle(char *text);
	char *getTitle(char *buf);
	void addWaste(char flag, long bytes);
};

struct FlexBase {
	int handle;
	FlexBase() { handle = -1; }
};

/* A Flex file and its header. */
struct Flex : FlexBase {
	char *name;
	int error;
	FlexHeader hdr;

	Flex() { name = 0; error = 0; }
	~Flex() { delete name; }
	unsigned char isopen() { return handle >= 0; }
	long getFileLength();
	void fatalError(FlexEntry *entry, int code);
	void setCount(int entries);
	void setName(char *path);
	void setError(int code);
	int getError(unsigned char clear);
	void printError(FlexEntry *entry);
	unsigned char open(char *path);
	void openOrFail(char *path);
	void close();
	virtual unsigned char getEntry(int i, FlexEntry *entry);
	virtual unsigned char readEntry(FlexEntry *entry, void far *buf, unsigned char quiet);
	virtual unsigned char readEntryToVoodoo(FlexEntry *entry, long block, unsigned char quiet);
	unsigned char readRecord(int i, void far *buf, unsigned char quiet);
	unsigned char readRecordToVoodoo(int i, long block, unsigned char quiet);
};

/* A Flex file opened for writing. */
struct FlexWriter : Flex {
	int fragmentation;
	char verify;

	void openForWrite(char *path, FlexHeader *initial);
	virtual void writeEntry(int i, FlexEntry *entry);
	void write(int i, void far *buffer, long size, unsigned char quiet);
	void writeBlock(int i, long block, long size, unsigned char quiet);
	void replace(char *path, int i, void far *buffer, long size);
	long countObjectBytes();
	int measureWaste();
	void compact(void far *buffer, long size);
	unsigned char compactIfNeeded(int percent);
};

#endif
