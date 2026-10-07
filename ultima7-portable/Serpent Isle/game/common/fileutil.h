#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <stdio.h>

int16_t OpenStreamOrFail(char *name, char *mode);
int32_t GetStreamLength(int16_t fp);
uint8_t FileExists(char *name);
int8_t CreateEmptyFile(char *name);
int32_t GetFileSize(char *name);
void TruncateAndClose(int16_t fp);
void LowerCaseString(char *s);
int8_t ReadFileRecord(char *name, int32_t size, int32_t index, void *buf);
int8_t AppendFileRecord(char *name, int32_t size, void *buf);
uint8_t WriteFileRecord(char *name, int32_t index, int32_t size, void *buf);
uint8_t IsRecordInFile(char *name, int32_t size, int32_t index);
void MoveBytes(char *src, char *dst, uint32_t n);
int32_t CountFileRecords(char *name, int32_t size);
int8_t DeleteFileRecord(char *name, int32_t index, int32_t size);
void CreateFileOrExit(char *name);

#endif
