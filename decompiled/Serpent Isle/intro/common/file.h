#ifndef FILE_H
#define FILE_H

#include "reporter.h"

/* Open modes; their names print in File::describe. */
#define FILE_CREATE     0
#define FILE_READ       1
#define FILE_APPEND     2

/* Seek origins, as DOS numbers them. */
#define FROM_START      0
#define FROM_HERE       1
#define FROM_END        2

/* The DOS file routines, in assembly. */
extern "C" {
long pascal DOSREAD(int handle, long at, long size, void far *buffer);
unsigned char pascal DOSWRITE(int handle, long at, long size, void far *buffer);
long pascal DOSSEEK(int handle, long offset, char whence);
int pascal DOSOPEN(char far *name);
int pascal DOSCREATE(char far *name);
void pascal DOSCLOSE(int handle);
unsigned far DosGetAttributes(char far *name);
unsigned char far DosFileExists(char far *name);
}

/* A DOS file handle. */
struct DosFile {
	int handle;
	DosFile() { handle = 0; }
	~DosFile() { if (handle) close(); }
	virtual long read(void far *buffer, long at, long size);
	virtual long write(void far *buffer, long at, long size);
	virtual long seek(long offset, char whence);
	virtual unsigned char open(char *name, char mode);
	virtual unsigned char close();
	virtual long length();
	virtual long position();
	unsigned char isOpen() { return handle != 0; }
};

/* A named file, or a window on part of another one. Positions count from the window's start. */
struct File : ErrorReporter {
	Message name;
	unsigned char mode;
	long position;
	long start;
	long length;
	DosFile *dos;
	File *parent;

	File();
	File(DosFile *dos);
	File(char *name, char mode);
	~File();
	void describe();
	void setName(char *name);
	void setMode(char mode);
	void setHandle(DosFile *dos);
	void set(char *name, char mode);
	void clear();
	void openPart(File *whole, long from, long to);
	void makeHandle();
	unsigned char open();
	unsigned char open(char *name, char mode);
	unsigned char setWindow(DosFile *dos, long from, long to);
	unsigned char close();
	unsigned char isOpen();
	unsigned char atEnd();
	void seek(long offset);
	void skip(long delta);
	void seekEnd();
	void rewind();
	long read(void far *buffer, long size, long at = -1);
	virtual unsigned char validate();
	virtual void load();
	virtual DosFile *createHandle();
	void notOpen();
	void checkOpen() { if (!isOpen()) notOpen(); }
	long getLength();
	long getPosition();
	char *getName();
	unsigned char reopen();

	static ErrorHandler defaultHandler;
	static void setDefaultHandler(ErrorHandler handler);
};

/* A File read a line at a time through a 160-byte buffer. */
struct LineFile : File {
	char *buffer;
	char *next;
	char *end;

	LineFile();
	~LineFile();
	unsigned char open(char *name) { return File::open(name, FILE_READ); }
	unsigned char hasLine() { return next < end || !atEnd(); }
	unsigned char fill();
	int readLine(char *line, unsigned size);
};

#endif
