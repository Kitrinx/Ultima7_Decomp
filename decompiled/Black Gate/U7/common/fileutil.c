/* Black Gate U7.EXE, resident segment 99 (file offsets 0x03604c to 0x03658a, 1342 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include "oops.h"

unsigned char IsRecordInFile(char *name, long size, long index);

/* opens a file, reporting a failure */
FILE *OpenStreamOrFail(char *name, char *mode)
{
	FILE *fp;

	fp = fopen(name, mode);
	if (fp == 0)
		ReportFileNotFound(name);
	return fp;
}

/* size of an open file, keeping its position */
long GetStreamLength(FILE *fp)
{
	long pos, size;

	pos = ftell(fp);
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fseek(fp, pos, SEEK_SET);
	return size;
}

/* whether the file can be opened */
unsigned char FileExists(char *name)
{
	FILE *fp;

	fp = fopen(name, "r+b");
	if (fp != 0) {
		fclose(fp);
		return 1;
	}
	return 0;
}

/* creates the file, reporting a failure */
char CreateEmptyFile(char *name)
{
	FILE *fp;

	fp = fopen(name, "w+b");
	if (fp != 0) {
		fclose(fp);
		return 1;
	}
	ReportFileNotFound(name);
}

/* size of a named file */
long GetFileSize(char *name)
{
	FILE *fp;
	long size;

	fp = fopen(name, "r+b");
	if (fp == 0)
		ReportFileNotFound(name);
	size = GetStreamLength(fp);
	fclose(fp);
	return size;
}

/* closes a file, cutting it off at the current position */
void TruncateAndClose(FILE *fp)
{
	int fd;

	fd = fileno(fp);
	_write(fd, 0, 0);
	fclose(fp);
}

void LowerCaseString(char *s)
{
	while (*s) {
		if (*s >= 'A' && *s <= 'Z')
			*s++ += 'a' - 'A';
		else
			s++;
	}
}

/* reads record index of a file of size-byte records */
char ReadFileRecord(char *name, long size, long index, void *buf)
{
	FILE *fp;

	if (!IsRecordInFile(name, size, index))
		return 0;
	fp = fopen(name, "r+b");
	if (fp == 0)
		return 0;
	fseek(fp, index * size, SEEK_SET);
	fread(buf, (unsigned) size, 1, fp);
	fclose(fp);
	return 1;
}

/* appends a record */
char AppendFileRecord(char *name, long size, void *buf)
{
	FILE *fp;

	fp = fopen(name, "a+b");
	if (fp == 0)
		return 0;
	fwrite(buf, (unsigned) size, 1, fp);
	fclose(fp);
	return 1;
}

/* writes record index */
unsigned char WriteFileRecord(char *name, long index, long size, void *buf)
{
	FILE *fp;

	if (!IsRecordInFile(name, size, index))
		return 0;
	fp = fopen(name, "r+b");
	if (fp == 0)
		return 0;
	fseek(fp, index * size, SEEK_SET);
	fwrite(buf, (int) size, 1, fp);
	fclose(fp);
	return 1;
}

/* whether record index lies inside the file */
unsigned char IsRecordInFile(char *name, long size, long index)
{
	long length;

	length = GetFileSize(name);
	if (length == -1L)
		return 0;
	if (index * size >= length)
		return 0;
	return 1;
}

void MoveBytes(char *src, char *dst, unsigned long n)
{
	if (src > dst) {
		while (n-- > 0)
			*dst++ = *src++;
	} else if (src < dst) {
		src += n - 1;
		dst += n - 1;
		while (n-- > 0)
			*dst-- = *src--;
	}
}

/* number of size-byte records in the file */
long CountFileRecords(char *name, long size)
{
	long length;

	length = GetFileSize(name);
	if (length == -1L)
		return -1L;
	return length / size;
}

/* removes record index from a file of size-byte records */
char DeleteFileRecord(char *name, long index, long size)
{
	char *buf;
	long i;
	long count;
	FILE *fp;

	buf = new char[size];
	count = GetFileSize(name) / size;
	if (index >= count) {
		delete buf;
		return 0;
	}
	fp = fopen(name, "r+b");
	if (fp == 0) {
		delete buf;
		return 0;
	}
	if (index == count - 1)
		fseek(fp, index * size, SEEK_SET);
	else
		for (i = index; i < count - 1; i++) {
			fseek(fp, (i + 1) * size, SEEK_SET);
			fread(buf, size, 1, fp);
			fseek(fp, i * size, SEEK_SET);
			fwrite(buf, size, 1, fp);
		}
	TruncateAndClose(fp);
	delete buf;
	return 1;
}

/* creates an empty file or quits */
void CreateFileOrExit(char *name)
{
	FILE *fp;

	fp = fopen(name, "w+b");
	if (fp == 0)
		exit(1);
	fclose(fp);
}
