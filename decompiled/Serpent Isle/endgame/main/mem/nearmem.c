/* Serpent Isle ENDGAME.EXE, resident segment 40 (file offsets 0x00dafb to 0x00dc60, 357 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include <alloc.h>
#include <mem.h>
#include "memsys.h"

unsigned char NearMemory::initialized = 0;

void far *NearMemory::allocate(long size, unsigned char)
{
	return new char[size];
}

void NearMemory::release(void far **block)
{
	if (*block) {
		delete (char *) *block;
		*block = 0;
	}
}

void NearMemory::copy(void far *to, void far *from, unsigned size)
{
	memcpy((void *) to, (void *) from, size);
}

long NearMemory::available()
{
	return coreleft();
}

int NearMemory::checkHeap()
{
	return heapcheck();
}

unsigned char NearMemory::isInitialized()
{
	return initialized;
}

NearMemory::NearMemory()
{
	if (initialized)
		error("Illegal re-initialization of Near memory\n");
	initialized = 1;
}
