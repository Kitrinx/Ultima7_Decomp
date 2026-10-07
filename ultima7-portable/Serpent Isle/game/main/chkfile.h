#ifndef CHKFILE_H
#define CHKFILE_H

/* How open() reaches the file; byte-sized where built with -b-. */
enum FileMode { FILE_CREATE, FILE_OPEN };

/* A named file: its own copy of the name, the open mode and the DOS handle. */
struct DataFile {
	char *name;
	int8_t mode;
	int16_t handle;

	DataFile() { handle = -1; name = 0; }
	DataFile(char *path, int8_t how);
	~DataFile();
	void fail(int16_t code);
	void setName(char *path, int8_t how);
	uint8_t open(char *path, FileMode how);
	uint8_t open(char *path, int32_t how) { return open(path, (FileMode)how); }
	int8_t reopen(int8_t how);
	void close();
	int32_t tell();
	int32_t seek(int32_t offset);
	int32_t skip(int32_t count);
	int16_t atEnd();
	int32_t getLength();
	int32_t seekEnd();
	int8_t readByte();
	int16_t readWord();
	int32_t readLong();
	uint32_t readUntil(int8_t stop, char *buf, uint32_t limit);
	int32_t read(void *buf, int32_t size);
	int8_t readRecords(uint32_t count, char *buf, uint32_t size);
	void writeByte(int8_t value);
	int16_t write(void *buf, int32_t size);
	void writeWord(int16_t n) { write(&n, INT32_C(2)); }
	void writeRecords(uint32_t count, char *buf, uint32_t size);
	void copyFrom(DataFile *source, uint32_t count);
};

#endif
