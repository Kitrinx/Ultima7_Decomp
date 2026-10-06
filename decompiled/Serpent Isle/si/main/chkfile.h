#ifndef CHKFILE_H
#define CHKFILE_H

/* How open() reaches the file; byte-sized where built with -b-. */
enum FileMode { FILE_CREATE, FILE_OPEN };

/* A named file: its own copy of the name, the open mode and the DOS handle. */
struct DataFile {
	char *name;
	char mode;
	int handle;

	DataFile() { handle = -1; name = 0; }
	DataFile(char far *path, char how);
	~DataFile();
	void fail(int code);
	void setName(char far *path, char how);
	unsigned char open(char far *path, FileMode how);
	char reopen(char how);
	void close();
	long tell();
	long seek(long offset);
	long skip(long count);
	int atEnd();
	long getLength();
	long seekEnd();
	char readByte();
	int readWord();
	long readLong();
	unsigned long readUntil(char stop, char far *buf, unsigned long limit);
	long read(void far *buf, long size);
	char readRecords(unsigned long count, char far *buf, unsigned long size);
	void writeByte(char value);
	int write(void far *buf, long size);
	void writeWord(int n) { write(&n, 2L); }
	void writeRecords(unsigned long count, char far *buf, unsigned long size);
	void copyFrom(DataFile *source, unsigned long count);
};

#endif
