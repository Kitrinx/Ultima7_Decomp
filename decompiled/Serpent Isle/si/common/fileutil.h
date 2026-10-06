#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <stdio.h>

FILE *OpenStreamOrFail(char *name, char *mode);
long GetStreamLength(FILE *fp);
unsigned char FileExists(char *name);
char CreateEmptyFile(char *name);
long GetFileSize(char *name);
void TruncateAndClose(FILE *fp);
void LowerCaseString(char *s);
char ReadFileRecord(char *name, long size, long index, void *buf);
char AppendFileRecord(char *name, long size, void *buf);
unsigned char WriteFileRecord(char *name, long index, long size, void *buf);
unsigned char IsRecordInFile(char *name, long size, long index);
void MoveBytes(char *src, char *dst, unsigned long n);
long CountFileRecords(char *name, long size);
char DeleteFileRecord(char *name, long index, long size);
void CreateFileOrExit(char *name);

#endif
