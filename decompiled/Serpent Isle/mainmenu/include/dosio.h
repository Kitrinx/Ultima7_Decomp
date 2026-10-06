#ifndef DOSIO_H
#define DOSIO_H

/* A shared 256-byte buffer for formatting text. */
extern char *WorkString;

/* Report an error code with the caller's address. */
#ifdef __cplusplus
extern "C"
#endif
void ReportError(int code);
void ReportErrorSubtype(int code, int subtype);

int LargerOf(int a, int b);
int ClampInt(int lo, int v, int hi);

#ifdef __cplusplus
extern "C" {
#endif
unsigned long far pascal PointerToLinear(void far *p);
void far *far pascal LinearToPointer(long linear);
void far pascal FillFarBytes(void far *dest, unsigned count, int value);
void far pascal MoveFarMemory(void far *dest, const void far *src, unsigned count);

/* DOS file calls. A failure goes through the error handler, then returns 0 or -1. */
unsigned char far pascal DosWrite(int handle, long pos, long count, void far *buf);
long far pascal DosRead(int handle, long pos, long count, void far *buf);
int far pascal DosOpen(const char far *name);
void far pascal DosClose(int handle);
int far pascal DosCreate(const char far *name);
long far pascal ReadFileBlock(int handle, long pos, long count, void far *buf);
long far pascal DosSeek(int handle, long pos, char method);
void far pascal WriteFileBlock(int handle, long pos, long count, char far *buf);

void UnhookInterrupt(int vector);
#ifdef __cplusplus
}
#endif

#endif
