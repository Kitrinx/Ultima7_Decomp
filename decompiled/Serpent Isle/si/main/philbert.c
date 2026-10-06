/* Serpent Isle SI.EXE, overlay segment 232 (file offsets 0x0655d0 to 0x065765, 405 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

#include "dosio.h"
#include "easyfile.h"
#include "datanode.h"
#include "keyring.h"
#include "uccomm7.h"

/* Where usecode last saved the Avatar's position. */
extern int MarkedX, MarkedY, MarkedZ;

/* The PHILBERT.DAT file in a saved game: the keyring, infravision and the saved Avatar position. */
struct PhilbertSaver : DataNode {
	char *name();
	void save(char *dir);
	void load(char *dir);
};

PhilbertSaver PhilbertFile;
char *PhilbertFileName = "PHILBERT.DAT";

char *PhilbertSaver::name()
{
	return PhilbertFileName;
}

void PhilbertSaver::save(char *dir)
{
	int fd;

	KeyRing.encode();
	fd = CreateFileOrFail(BuildPath(dir, PhilbertFileName, 0));
	DosWrite(fd, -1L, 33L, &KeyRing);
	DosWrite(fd, -1L, 2L, &InfravisionEnabled);
	DosWrite(fd, -1L, 2L, &MarkedX);
	DosWrite(fd, -1L, 2L, &MarkedY);
	DosWrite(fd, -1L, 2L, &MarkedZ);
	DosClose(fd);
	KeyRing.decode();
}

void PhilbertSaver::load(char *dir)
{
	int fd;

	fd = OpenFileOrFail(BuildPath(dir, PhilbertFileName, 0));
	DosRead(fd, -1L, 33L, &KeyRing);
	DosRead(fd, -1L, 2L, &InfravisionEnabled);
	DosRead(fd, -1L, 2L, &MarkedX);
	DosRead(fd, -1L, 2L, &MarkedY);
	DosRead(fd, -1L, 2L, &MarkedZ);
	DosClose(fd);
	KeyRing.decode();
}
