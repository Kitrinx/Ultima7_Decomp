#ifndef MAKEMOJO_H
#define MAKEMOJO_H

void far CheckMojoBounds(unsigned long bound, unsigned long index);
unsigned char far MakeMojo(long count, long size, long *mem, long *bound);
unsigned char far ReadMojo(int fd, long pos, long first, long last, long bound, long mem, long size);
void far WriteMojo(int fd, long pos, long first, long last, long bound, long mem, long size);

#endif
