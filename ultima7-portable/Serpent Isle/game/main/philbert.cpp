/* Serpent Isle SI.EXE, overlay segment 232 (file offsets 0x0655d0 to 0x065765, 405 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
#include "dosio.h"
#include "easyfile.h"
#include "datanode.h"
#include "keyring.h"
#include "uccomm7.h"

/* Where usecode last saved the Avatar's position. */
extern int16_t MarkedX, MarkedY, MarkedZ;

/* The PHILBERT.DAT file in a saved game: the keyring, infravision and the saved Avatar position. */
struct PhilbertSaver : DataNode {
	char *name();
	void save(char *dir);
	void load(char *dir);
};

PhilbertSaver PhilbertFile;
char *const PhilbertFileName = "PHILBERT.DAT";

char *PhilbertSaver::name()
{
	return PhilbertFileName;
}

void PhilbertSaver::save(char *dir)
{
	int16_t fd;

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
