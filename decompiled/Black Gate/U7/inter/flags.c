/* Black Gate U7.EXE, overlay segment 314 (file offsets 0x091290 to 0x0917d3, 1347 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

/* path: ..\inter\flags.c */
#include <conio.h>
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

#define FLAGS_ERROR(where, line)    FatalError("%s:%s%d", where, __FILE__, line)

char *FlagInitFileName = "FLAGINIT.";
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
	char far *p = bits;
	int i;

	sum = 0;
	for (i = 0; i < size; i++) {
		sum += *p;
		p++;
	}
}

/* complain if the bits changed since the last checksum */
void GameFlagSet::verify(char *where)
{
	long old = sum;

	checksum();
	if (sum != old)
		FLAGS_ERROR(where, 103);
}

void GameFlagSet::init()
{
	count = GetArchivedFileSize(file);
	size = count / 8 + 1;
	bits = (char far *) AllocateFarHeap(size, 0);
	if (bits == 0)
		ReportOutOfFarMemory();
}

/* one byte per flag */
void GameFlagSet::load(char *dir)
{
	DataFile input(BuildPath(dir, file, 0), 1);
	int i;

	if (input.getLength() != count)
		ReportError(0x6d03);
	_fmemset(bits, 0, size);
	for (i = 0; i < count; i++)
		set(i, input.readByte());
}

void GameFlagSet::save(char *dir)
{
	DataFile output(BuildPath(dir, file, 0), 0);
	int i;

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

unsigned char GameFlagSet::get(int flag)
{
	verify(0);
	if (count < flag)
		ReportError(0x6d02);
	return (bits[flag / 8] & (0x80 >> (flag & 7))) != 0;
}

void GameFlagSet::set(int flag, char on)
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
int GameFlagSet::find(char far *list, char *s, int len)
{
	char far *p;
	char far *end;
	int i;

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
void far *GameFlagSet::read(char *name, int *len)
{
	void far *p = 0;
	DataFile input(name, 1);

	*len = input.getLength();
	p = AllocateFarHeap(*len, 0);
	if (p)
		input.read(p, *len);
	return p;
}

/* the flag the debug screens act on; none */
int GameFlagSet::pick()
{
	return -1;
}

void GameFlagSet::edit()
{
	char c;
	int flag;

	flag = pick();
	if (flag != -1) {
		gotoxy(1, 5);
		printf("Enter (t) or (f)  ");
		while (!kbhit())
			;
		c = tolower(getche());
		if (c == 't')
			set(flag, 1);
		else if (c == 'f')
			set(flag, 0);
	}
	printf("\nPress a key...");
	while (!kbhit())
		;
	getch();
}

void GameFlagSet::show()
{
	int flag;

	flag = pick();
	if (flag != -1) {
		gotoxy(1, 5);
		printf("Flag num = %d, value = %d  ", flag, get(flag));
	}
	while (!kbhit())
		;
	getch();
}
