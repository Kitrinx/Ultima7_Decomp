/* The game's lowest file calls, over the platform's files.
 * A failure stores a DOS error code in DosError and calls DosErrorHandler; a handler that
 * clears DosError asks for the call to be tried again.
 */
#include "u7port.h"
#include "plat.h"
#include "dosio.h"

#define DOS_FILE_NOT_FOUND 2
#define DOS_ACCESS_DENIED 5
#define DOS_DISK_FULL 0xFFFF

static void AcceptDosError(void)
{
}

uint16_t DosError = 0;
void (*DosErrorHandler)(void) = AcceptDosError;
int32_t DosBytesRead = 0;

static uint8_t GiveUp(uint16_t code)
{
	DosError = code;
	DosErrorHandler();
	return DosError != 0;
}

/* A position whose high word is 0xFFFF means "where the file is now". */
static uint8_t SeekUnlessCurrent(int16_t handle, int32_t pos)
{
	if (((uint32_t)pos >> 16) == 0xFFFF)
		return 1;
	return plat_file_seek(handle, pos, PLAT_SEEK_SET) >= 0;
}

int16_t DosOpen(const char *name)
{
	int16_t handle;

	DosError = 0;
	for (;;) {
		handle = plat_file_open(name, PLAT_FILE_READ | PLAT_FILE_WRITE);
		if (handle < 0)
			handle = plat_file_open(name, PLAT_FILE_READ);
		if (handle >= 0)
			return handle;
		if (GiveUp(DOS_FILE_NOT_FOUND))
			return -1;
	}
}

void DosClose(int16_t handle)
{
	plat_file_close(handle);
}

int16_t DosCreate(const char *name)
{
	int16_t handle;

	for (;;) {
		handle = plat_file_create(name);
		if (handle >= 0)
			return handle;
		if (GiveUp(DOS_ACCESS_DENIED))
			return -1;
	}
}

int32_t DosRead(int16_t handle, int32_t pos, int32_t count, void *buf)
{
	int32_t got;

	DosError = 0;
	if (count == 0)
		return 0;
	for (;;) {
		DosBytesRead = 0;
		if (buf && SeekUnlessCurrent(handle, pos)) {
			got = plat_file_read(handle, buf, count);
			if (got >= 0) {
				DosBytesRead = got;
				return got;
			}
		}
		if (GiveUp(buf ? DOS_ACCESS_DENIED : 1))
			return 0;
	}
}

uint8_t DosWrite(int16_t handle, int32_t pos, int32_t count, void *buf)
{
	int32_t put;
	uint16_t code;

	for (;;) {
		code = 1;
		if (buf && SeekUnlessCurrent(handle, pos)) {
			put = plat_file_write(handle, buf, count);
			if (put == count)
				return 1;
			code = put < 0 ? DOS_ACCESS_DENIED : DOS_DISK_FULL;
		}
		if (GiveUp(code))
			return 0;
	}
}

int32_t DosSeek(int16_t handle, int32_t pos, int8_t method)
{
	int32_t at;

	DosError = 0;
	for (;;) {
		at = plat_file_seek(handle, pos, method);
		if (at >= 0)
			return at;
		if (GiveUp(DOS_ACCESS_DENIED))
			return -1;
	}
}

int32_t ReadFileBlock(int16_t handle, int32_t pos, int32_t count, void *buf)
{
	return DosRead(handle, pos, count, buf);
}

void WriteFileBlock(int16_t handle, int32_t pos, int32_t count, char *buf)
{
	DosWrite(handle, pos, count, buf);
}

extern "C" void ResetDosioGlobals(void)
{
	DosError = 0;
	DosErrorHandler = AcceptDosError;
	DosBytesRead = 0;
}
