#ifndef EASYFILE_H
#define EASYFILE_H

/* Paths come back in one of a few rotating buffers. */
char *StorePath(char *name);
char *BuildPath(char *dir, char *name, char *ext);
char *BuildNumberedPath(char *dir, char *fmt, int16_t n, char *ext);
void ReplaceString(char **p, char *s);

/* Open or create, stopping the game on failure. */
int16_t OpenFileOrFail(char *name);
int16_t CreateFileOrFail(char *name);

/* Move size bytes between a file and a far Voodoo block. */
int32_t ReadHandleToVoodoo(int16_t fd, int32_t offset, int32_t size, int32_t *block);
uint8_t WriteHandleFromVoodoo(int16_t fd, int32_t offset, int32_t size, int32_t block);

char *BuildNumberedTempPath(char *dir, char *fmt, int16_t n, int8_t temp);

extern void *FileTransferBuffer;

#endif
