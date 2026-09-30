#ifndef DOSIO_H
#define DOSIO_H

/* A shared 256-byte buffer for formatting text. */
#ifdef __cplusplus
extern "C" {
#endif
extern char *WorkString;
#ifdef __cplusplus
}
#endif

/* Report an error code with the caller's address. */
#ifdef __cplusplus
extern "C"
#endif
void ReportError(int16_t code);
void ReportErrorSubtype(int16_t code, int16_t subtype);

int16_t LargerOf(int16_t a, int16_t b);
int16_t ClampInt(int16_t lo, int16_t v, int16_t hi);

#ifdef __cplusplus
extern "C" {
#endif
uint32_t PointerToLinear(void *p);
void * LinearToPointer(int32_t linear);
void FillFarBytes(void *dest, uint16_t count, int16_t value);
void MoveFarMemory(void *dest, const void *src, uint16_t count);

/* DOS file calls. A failure goes through the error handler, then returns 0 or -1. */
extern uint16_t DosError;
extern void (*DosErrorHandler)(void);
extern int32_t DosBytesRead;
uint8_t DosWrite(int16_t handle, int32_t pos, int32_t count, void *buf);
int32_t DosRead(int16_t handle, int32_t pos, int32_t count, void *buf);
int16_t DosOpen(const char *name);
void DosClose(int16_t handle);
int16_t DosCreate(const char *name);
int32_t ReadFileBlock(int16_t handle, int32_t pos, int32_t count, void *buf);
int32_t DosSeek(int16_t handle, int32_t pos, int8_t method);
void WriteFileBlock(int16_t handle, int32_t pos, int32_t count, char *buf);

#ifdef __cplusplus
}
#endif

#endif
