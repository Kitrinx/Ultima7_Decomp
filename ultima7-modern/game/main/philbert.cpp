/* Serpent Isle SI.EXE, overlay segment 232 (file offsets 0x0655d0 to 0x065765, 405 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
#include "plat.h"
#include "dosio.h"
#include "easyfile.h"
#include "datanode.h"
#include "keyring.h"
#include "uccomm7.h"
#include "rules.h"

/* Where usecode last saved the Avatar's position. */
extern "C" int16_t MarkedX, MarkedY, MarkedZ;

/* The PHILBERT.DAT file in a saved game: the keyring, infravision and the saved Avatar position. */
struct PhilbertSaver : DataNode {
	char *name();
	void save(char *dir);
	void load(char *dir);
};

PhilbertSaver PhilbertFile;
char *const PhilbertFileName = "PHILBERT.DAT";

char *const KeyRingFileName = "KEYRING.DAT";

/* Black Gate keeps the key ring and infravision (set by Serpent Isle's items), in a file written
 * only while either is in use, so saves without them keep the same files. */
static void SaveKeyRingAlone(char *dir)
{
	int16_t fd, i;

	for (i = 0; i < 32 && !KeyRing.keys[i]; i++)
		;
	if (i == 32 && !InfravisionEnabled) {
		plat_file_remove(BuildPath(dir, KeyRingFileName, 0));
		return;
	}
	KeyRing.encode();
	fd = CreateFileOrFail(BuildPath(dir, KeyRingFileName, 0));
	DosWrite(fd, -INT32_C(1), INT32_C(33), &KeyRing);
	DosWrite(fd, -INT32_C(1), INT32_C(2), &InfravisionEnabled);
	DosClose(fd);
	KeyRing.decode();
}

static void LoadKeyRingAlone(char *dir)
{
	int16_t fd;

	memset(KeyRing.keys, 0, sizeof KeyRing.keys);
	InfravisionEnabled = 0;
	if (!plat_file_exists(BuildPath(dir, KeyRingFileName, 0)))
		return;
	fd = OpenFileOrFail(BuildPath(dir, KeyRingFileName, 0));
	DosRead(fd, -INT32_C(1), INT32_C(33), &KeyRing);
	DosRead(fd, -INT32_C(1), INT32_C(2), &InfravisionEnabled);
	DosClose(fd);
	KeyRing.decode();
}

char *PhilbertSaver::name()
{
	return PhilbertFileName;
}

void PhilbertSaver::save(char *dir)
{
	int16_t fd;

	if (!HasPhilbertFile()) {
		SaveKeyRingAlone(dir);
		return;
	}
	KeyRing.encode();
	fd = CreateFileOrFail(BuildPath(dir, PhilbertFileName, 0));
	DosWrite(fd, -INT32_C(1), INT32_C(33), &KeyRing);
	DosWrite(fd, -INT32_C(1), INT32_C(2), &InfravisionEnabled);
	DosWrite(fd, -INT32_C(1), INT32_C(2), &MarkedX);
	DosWrite(fd, -INT32_C(1), INT32_C(2), &MarkedY);
	DosWrite(fd, -INT32_C(1), INT32_C(2), &MarkedZ);
	DosClose(fd);
	KeyRing.decode();
}

void PhilbertSaver::load(char *dir)
{
	int16_t fd;

	if (!HasPhilbertFile()) {
		LoadKeyRingAlone(dir);
		return;
	}
	fd = OpenFileOrFail(BuildPath(dir, PhilbertFileName, 0));
	DosRead(fd, -INT32_C(1), INT32_C(33), &KeyRing);
	DosRead(fd, -INT32_C(1), INT32_C(2), &InfravisionEnabled);
	DosRead(fd, -INT32_C(1), INT32_C(2), &MarkedX);
	DosRead(fd, -INT32_C(1), INT32_C(2), &MarkedY);
	DosRead(fd, -INT32_C(1), INT32_C(2), &MarkedZ);
	DosClose(fd);
	KeyRing.decode();
}

extern "C" void ResetPhilbertGlobals(void)
{
	memset((void *)&PhilbertFile, 0, sizeof(PhilbertFile));
}

extern "C" void ConstructPhilbertGlobals(void)
{
	new (&PhilbertFile) PhilbertSaver();
}
