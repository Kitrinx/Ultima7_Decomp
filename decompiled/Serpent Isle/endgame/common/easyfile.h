#ifndef EASYFILE_H
#define EASYFILE_H

/* Open a file, stopping the program on failure. */
int OpenFileOrFail(char *name);

/* Read a file into a far Voodoo block. */
long ReadHandleToVoodoo(int fd, long offset, long size, long *block);
long LoadFileToVoodoo(char *name, long block);

#endif
