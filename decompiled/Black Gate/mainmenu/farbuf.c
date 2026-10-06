/* Black Gate MAINMENU.EXE, resident segment 22 (file offsets 0x00f323 to 0x00f4b9, 406 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "chkfile.h"
#include "flex.h"
#include "memapi.h"
#include "oops.h"
#include "farbuf.h"

void far *FarBuffer::allocate(long size)
{
	data = AllocateFarHeap(size, 0);
	if (data == 0)
		ReportOutOfFarMemory();
	owned = 1;
	return data;
}

/* Reads a whole file into the buffer; returns its size. */
long FarBuffer::load(char *name)
{
	if (data != 0)
		release();
	DataFile f(name, 1);
	long size = f.getLength();
	allocate(size);
	f.read(data, size);
	return size;
}

/* Reads entry i of a Flex file into the buffer; returns its size. */
long FarBuffer::load(char *flexName, int i)
{
	if (data != 0)
		release();
	Flex flex;
	flex.open(flexName);
	FlexEntry entry;
	flex.getEntry(i, &entry);
	long size = entry.size;
	allocate(size);
	flex.readEntry(&entry, data, 0);
	flex.close();
	return size;
}

/* The test assigns, so the buffer is freed whether or not it was allocated here. */
void FarBuffer::release()
{
	if (owned = 1)
		FreeFarHeap(data);
	owned = 0;
	data = 0;
}
