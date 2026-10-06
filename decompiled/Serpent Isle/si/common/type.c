/* Serpent Isle SI.EXE, resident segment 72 (file offsets 0x0301ad to 0x030268, 187 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder chosen by subsystem.
 */

#include "dosio.h"
#include "memapi.h"
#include "oops.h"
#include "type.h"

char far *TypeAnimations = 0;
struct TypeInfo gItemTypeInfo[1024];    /* TFA.DAT */

/* read 3072 bytes into buf from the head of the named file, and the 512-byte table after them */
void LoadTfa(char *buf, char *name)
{
	int fd;

	if (TypeAnimations == 0) {
		TypeAnimations = AllocateFarHeap(512L, 0);
		if (TypeAnimations == 0)
			ReportOutOfFarMemory();
	}
	fd = DosOpen(name);
	if (fd < 0)
		ReportFileNotFound(name);
	DosRead(fd, 0L, 3072L, buf);
	DosRead(fd, -1L, 512L, TypeAnimations);
	DosClose(fd);
}

/* two types a byte, odd types in the high nibble */
int GetTypeAnimation(unsigned type)
{
	if (type & 1)
		return (TypeAnimations[type >> 1] >> 4) & 0xf;
	else
		return TypeAnimations[type >> 1] & 0xf;
}
