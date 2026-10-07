/* Serpent Isle SI.EXE, overlay segment 298 (file offsets 0x080870 to 0x080db3, 1347 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

/* path: flags.c */
#include "u7port.h"
#include <new>
#include "plat.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "dosio.h"
#include "easyfile.h"
#include "chkfile.h"
#include "init.h"
#include "fileutil.h"
#include "oops.h"
#include "memapi.h"
#include "flags.h"
#include "debug.h"
#include "u7event.h"
#include "savegame.h"

#define FLAGS_ERROR(where, line)    FatalError("%s:%s%d", where, __FILE__, line)

char *const FlagInitFileName = "FLAGINIT.";
GameFlagSet GameFlags(FlagInitFileName);

void VerifyFlags(char *where)
{
	GameFlags.verify(where);
}

GameFlagSet::GameFlagSet(char *fileName)
{
	count = 0;
	size = 0;
	bits = 0;
	file = fileName;
}

GameFlagSet::~GameFlagSet()
{
	if (bits) {
		FreeFarHeap(bits);
		bits = 0;
	}
}

/* the sum of the bytes as signed chars */
void GameFlagSet::checksum()
{
	char *p = bits;
	int16_t i;

	sum = 0;
	for (i = 0; i < size; i++) {
		sum += *p;
		p++;
	}
}

/* complain if the bits changed since the last checksum */
void GameFlagSet::verify(char *where)
{
	int32_t old = sum;

	checksum();
	if (sum != old)
		FLAGS_ERROR(where, 103);
}

void GameFlagSet::init()
{
	count = GetArchivedFileSize(file);
	size = count / 8 + 1;
	bits = (char *) AllocateFarHeap(size, 0);
	if (bits == 0)
		ReportOutOfFarMemory();
}

/* one byte per flag */
void GameFlagSet::load(char *dir)
{
	DataFile input(BuildPath(dir, file, 0), 1);
	int16_t i;

	if (input.getLength() != count)
		ReportError(0x6d03);
	_fmemset(bits, 0, size);
	for (i = 0; i < count; i++)
		set(i, input.readByte());
}

void GameFlagSet::save(char *dir)
{
	DataFile output(BuildPath(dir, file, 0), 0);
	int16_t i;

	for (i = 0; i < count; i++)
		output.writeByte(get(i));
}

void GameFlagSet::refresh(char *)
{
}

char *GameFlagSet::name()
{
	return FlagInitFileName;
}

uint8_t GameFlagSet::get(int16_t flag)
{
	verify(0);
	if (count < flag)
		ReportError(0x6d02);
	return (bits[flag / 8] & (0x80 >> (flag & 7))) != 0;
}

void GameFlagSet::set(int16_t flag, int8_t on)
{
	if (count >= flag) {
		if (on)
			bits[flag >> 3] |= 0x80 >> (flag & 7);
		else
			bits[flag >> 3] &= ~(0x80 >> (flag & 7));
		checksum();
	}
}

/* the position of name s in a block of len bytes of strings, or -1 */
int16_t GameFlagSet::find(char *list, char *s, int16_t len)
{
	char *p;
	char *end;
	int16_t i;

	if (list == 0)
		return -1;
	p = list;
	end = p + len;
	for (i = 0; p < end; i++) {
		if (_fstricmp(p, s) == 0)
			return i;
		p += _fstrlen(p) + 1;
	}
	return -1;
}

/* the whole of a file in far memory, its length in *len */
void *GameFlagSet::read(char *name, int16_t *len)
{
	void *p = 0;
	DataFile input(name, 1);

	*len = input.getLength();
	p = AllocateFarHeap(*len, 0);
	if (p)
		input.read(p, *len);
	return p;
}

/* the flag the debug screens act on; none */
int16_t GameFlagSet::pick()
{
	return -1;
}

void GameFlagSet::edit()
{
	int8_t c;
	int16_t flag;

	flag = pick();
	if (flag != -1) {
		plat_console_goto(1, 5);
		ConsoleWrite("Enter (t) or (f)  ");
		while (!KeyPressed())
			plat_yield();
		c = ReadKey();
		plat_console_putch((uint8_t) c);
		c = tolower(c);
		if (c == 't')
			set(flag, 1);
		else if (c == 'f')
			set(flag, 0);
	}
	ConsoleWrite("\nPress a key...");
	while (!KeyPressed())
		plat_yield();
	ReadKey();
}

void GameFlagSet::show()
{
	int16_t flag;
	char text[40];

	flag = pick();
	if (flag != -1) {
		sprintf(text, "Flag num = %d, value = %d  ", flag, get(flag));
		plat_console_goto(1, 5);
		ConsoleWrite(text);
	}
	while (!KeyPressed())
		plat_yield();
	ReadKey();
}

extern "C" void ResetFlagsGlobals(void)
{
	memset((void *)&GameFlags, 0, sizeof(GameFlags));
}

extern "C" void ConstructFlagsGlobals(void)
{
	new (&GameFlags) GameFlagSet(FlagInitFileName);
}
