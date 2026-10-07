/* Serpent Isle SI.EXE, resident segment 73 (file offsets 0x030268 to 0x0307a6, 1342 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
#include <stdio.h>
#include <stdlib.h>
#include "plat.h"
#include "oops.h"

#include "fileutil.h"

/* Opens a file the way fopen's mode would. "r+" falls back to read-only, as data files may be. */
static int16_t OpenStream(char *name, const char *mode)
{
	int16_t fd;

	if (mode[0] == 'w')
		return plat_file_create(name);
	fd = -1;
	if (mode[0] == 'a' || strchr(mode, '+'))
		fd = plat_file_open(name, PLAT_FILE_READ | PLAT_FILE_WRITE);
	if (fd < 0 && mode[0] == 'r')
		fd = plat_file_open(name, PLAT_FILE_READ);
	if (mode[0] == 'a') {
		if (fd < 0)
			fd = plat_file_create(name);
		if (fd >= 0)
			plat_file_seek(fd, 0, PLAT_SEEK_END);
	}
	return fd;
}

/* opens a file, reporting a failure */
int16_t OpenStreamOrFail(char *name, char *mode)
{
	int16_t fp;

	fp = OpenStream(name, mode);
	if (fp < 0)
		ReportFileNotFound(name);
	return fp;
}

/* size of an open file, keeping its position */
int32_t GetStreamLength(int16_t fp)
{
	return plat_file_length(fp);
}

/* whether the file can be opened */
uint8_t FileExists(char *name)
{
	return plat_file_exists(name) != 0;
}

/* creates the file, reporting a failure */
int8_t CreateEmptyFile(char *name)
{
	int16_t fp;

	fp = OpenStream(name, "w+b");
	if (fp >= 0) {
		plat_file_close(fp);
		return 1;
	}
	ReportFileNotFound(name);
	return 0;
}

/* size of a named file */
int32_t GetFileSize(char *name)
{
	int16_t fp;
	int32_t size;

	fp = OpenStream(name, "r+b");
	if (fp < 0)
		ReportFileNotFound(name);
	size = GetStreamLength(fp);
	plat_file_close(fp);
	return size;
}

/* closes a file, cutting it off at the current position */
void TruncateAndClose(int16_t fp)
{
	plat_file_truncate(fp);
	plat_file_close(fp);
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
int8_t ReadFileRecord(char *name, int32_t size, int32_t index, void *buf)
{
	int16_t fp;

	if (!IsRecordInFile(name, size, index))
		return 0;
	fp = OpenStream(name, "r+b");
	if (fp < 0)
		return 0;
	plat_file_seek(fp, index * size, PLAT_SEEK_SET);
	plat_file_read(fp, buf, (uint16_t) size);
	plat_file_close(fp);
	return 1;
}

/* appends a record */
int8_t AppendFileRecord(char *name, int32_t size, void *buf)
{
	int16_t fp;

	fp = OpenStream(name, "a+b");
	if (fp < 0)
		return 0;
	plat_file_write(fp, buf, (uint16_t) size);
	plat_file_close(fp);
	return 1;
}

/* writes record index */
uint8_t WriteFileRecord(char *name, int32_t index, int32_t size, void *buf)
{
	int16_t fp;

	if (!IsRecordInFile(name, size, index))
		return 0;
	fp = OpenStream(name, "r+b");
	if (fp < 0)
		return 0;
	plat_file_seek(fp, index * size, PLAT_SEEK_SET);
	plat_file_write(fp, buf, (int16_t) size);
	plat_file_close(fp);
	return 1;
}

/* whether record index lies inside the file */
uint8_t IsRecordInFile(char *name, int32_t size, int32_t index)
{
	int32_t length;

	length = GetFileSize(name);
	if (length == -INT32_C(1))
		return 0;
	if (index * size >= length)
		return 0;
	return 1;
}

void MoveBytes(char *src, char *dst, uint32_t n)
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
int32_t CountFileRecords(char *name, int32_t size)
{
	int32_t length;

	length = GetFileSize(name);
	if (length == -INT32_C(1))
		return -INT32_C(1);
	return length / size;
}

/* removes record index from a file of size-byte records */
int8_t DeleteFileRecord(char *name, int32_t index, int32_t size)
{
	char *buf;
	int32_t i;
	int32_t count;
	int16_t fp;

	buf = new char[size];
	count = GetFileSize(name) / size;
	if (index >= count) {
		delete buf;
		return 0;
	}
	fp = OpenStream(name, "r+b");
	if (fp < 0) {
		delete buf;
		return 0;
	}
	if (index == count - 1)
		plat_file_seek(fp, index * size, PLAT_SEEK_SET);
	else
		for (i = index; i < count - 1; i++) {
			plat_file_seek(fp, (i + 1) * size, PLAT_SEEK_SET);
			plat_file_read(fp, buf, size);
			plat_file_seek(fp, i * size, PLAT_SEEK_SET);
			plat_file_write(fp, buf, size);
		}
	TruncateAndClose(fp);
	delete buf;
	return 1;
}

/* creates an empty file or quits */
void CreateFileOrExit(char *name)
{
	int16_t fp;

	fp = OpenStream(name, "w+b");
	if (fp < 0)
		plat_exit(1);
	plat_file_close(fp);
}
