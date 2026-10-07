#ifndef FLEX_H
#define FLEX_H

/* Where an object sits in a Flex file, and how long it is. */
struct FlexEntry {
	int32_t offset;
	int32_t size;
	FlexEntry() {}
	FlexEntry(int32_t start, int32_t length) { offset = start; size = length; }
	uint8_t empty() { return size == 0; }
	void clear() { size = 0; offset = 0; }
	int16_t operator==(FlexEntry other);
};

/* The 128 bytes at the start of a Flex file; the entry table follows. */
struct FlexHeader {
	char title[81];
	uint8_t eof;
	int16_t magic;
	int32_t count;
	int32_t version;
	int32_t saveVersion, unusedField1;
	int32_t waste;
	int32_t checksum;
	int8_t wasteFlag;
	int8_t packed;
	int16_t unusedField2;
	char unusedField3[16];

	FlexHeader() { clear(); }
	uint8_t valid() { return magic == -1; }
	uint8_t unchanged() { return computeChecksum() == checksum; }
	void setFlexMarks();
	void clear();
	int32_t computeChecksum();
	void read(int16_t fd);
	void writeIfChanged(int16_t fd);
	void create(int16_t entries, char *text);
	void setTitle(char *text);
	char *getTitle(char *buf);
	void addWaste(int8_t flag, int32_t bytes);
};

struct FlexBase {
	int16_t handle;
	FlexBase() { handle = -1; }
};

/* A Flex file and its header. */
struct Flex : FlexBase {
	char *name;
	int16_t error;
	FlexHeader hdr;

	Flex() { name = 0; error = 0; }
	~Flex() { delete name; }
	uint8_t isopen() { return handle >= 0; }
	int32_t getFileLength();
	void fatalError(FlexEntry *entry, int16_t code);
	void setCount(int16_t entries);
	void setName(char *path);
	void setError(int16_t code);
	int16_t getError(uint8_t clear);
	void printError(FlexEntry *entry);
	uint8_t open(char *path);
	void openOrFail(char *path);
	void close();
	virtual uint8_t getEntry(int16_t i, FlexEntry *entry);
	virtual uint8_t readEntry(FlexEntry *entry, void *buf, uint8_t quiet);
	virtual uint8_t readEntryToVoodoo(FlexEntry *entry, int32_t block, uint8_t quiet);
	uint8_t readRecord(int16_t i, void *buf, uint8_t quiet);
	uint8_t readRecordToVoodoo(int16_t i, int32_t block, uint8_t quiet);
};

/* A Flex file opened for writing. */
struct FlexWriter : Flex {
	int16_t fragmentation;
	int8_t verify;

	void openForWrite(char *path, FlexHeader *initial);
	virtual void writeEntry(int16_t i, FlexEntry *entry);
	void write(int16_t i, void *buffer, int32_t size, uint8_t quiet);
	void writeBlock(int16_t i, int32_t block, int32_t size, uint8_t quiet);
	void replace(char *path, int16_t i, void *buffer, int32_t size);
	int32_t countObjectBytes();
	int16_t measureWaste();
	void compact(void *buffer, int32_t size);
	uint8_t compactIfNeeded(int16_t percent);
};

#endif
