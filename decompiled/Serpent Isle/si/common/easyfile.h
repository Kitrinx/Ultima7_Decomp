#ifndef EASYFILE_H
#define EASYFILE_H

/* Paths come back in one of a few rotating buffers. */
char *StorePath(char far *name);
char *BuildPath(char *dir, char *name, char *ext);
char *BuildNumberedPath(char *dir, char *fmt, int n, char *ext);
void ReplaceString(char **p, char *s);

/* Open or create, stopping the game on failure. */
int OpenFileOrFail(char *name);
int CreateFileOrFail(char *name);

/* Move size bytes between a file and a far Voodoo block. */
long ReadHandleToVoodoo(int fd, long offset, long size, long *block);
unsigned char WriteHandleFromVoodoo(int fd, long offset, long size, long block);

char *BuildNumberedTempPath(char *dir, char *fmt, int n, char temp);

extern void far *FileTransferBuffer;

#endif
