/* Black Gate ENDGAME.EXE, resident segment 11 (file offsets 0x00aeeb to 0x00afdd, 242 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include <string.h>
#include "iff.h"
#include "chunknam.h"

#define NAMESIZE    8

/* Reads a name: eight bytes, then the terminator. */
void ChunkName::read(IffFile *f)
{
	clear();
	f->read(NAMESIZE, text);
	text[NAMESIZE] = 0;
}

/* The same, into a far buffer. */
void ReadChunkName(char far *name, IffFile *f)
{
	ClearName(name);
	f->read(NAMESIZE, name);
	name[NAMESIZE] = 0;
}

/* Joins directory, name and extension into a new string. */
void PathName::build(char *dir, char *name, char *ext)
{
	int length;

	clear();
	length = strlen(dir);
	length += strlen(name);
	length += strlen(ext);
	length++;
	text = new char[length];
	strcpy(text, dir);
	strcat(text, name);
	strcat(text, ext);
	text[length - 1] = 0;
}
