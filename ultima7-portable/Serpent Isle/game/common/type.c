/* Serpent Isle SI.EXE, resident segment 72 (file offsets 0x0301ad to 0x030268, 187 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
#include "dosio.h"
#include "memapi.h"
#include "oops.h"
#include "type.h"

char *TypeAnimations = 0;
struct TypeInfo gItemTypeInfo[1024];    /* TFA.DAT */

/* read 3072 bytes into buf from the head of the named file, and the 512-byte table after them */
void LoadTfa(char *buf, char *name)
{
	int16_t fd;

	if (TypeAnimations == 0) {
		TypeAnimations = AllocateFarHeap(INT32_C(512), 0);
		if (TypeAnimations == 0)
			ReportOutOfFarMemory();
	}
	fd = DosOpen(name);
	if (fd < 0)
		ReportFileNotFound(name);
	DosRead(fd, INT32_C(0), INT32_C(3072), buf);
	DosRead(fd, -INT32_C(1), INT32_C(512), TypeAnimations);
	DosClose(fd);
}

/* two types a byte, odd types in the high nibble */
int16_t GetTypeAnimation(uint16_t type)
{
	if (type & 1)
		return (TypeAnimations[type >> 1] >> 4) & 0xf;
	else
		return TypeAnimations[type >> 1] & 0xf;
}

void ResetTypeGlobals(void)
{
	TypeAnimations = 0;
	memset(gItemTypeInfo, 0, sizeof gItemTypeInfo);
}
