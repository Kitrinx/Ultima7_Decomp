/* Serpent Isle MAINMENU.EXE, resident segment 22 (file offsets 0x00fca7 to 0x00fe34, 397 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "dosio.h"
#include "chkfile.h"
#include "flex.h"
#include "memapi.h"
#include "oops.h"
#include "farbuf.h"

namespace Shared {

void *FarBuffer::allocate(int32_t size)
{
	data = AllocateFarHeap(size, 0);
	if (data == 0)
		ReportOutOfFarMemory();
	owned = 1;
	return data;
}

/* Reads a whole file into the buffer; returns its size. */
int32_t FarBuffer::load(char *name)
{
	if (data != 0)
		release();
	DataFile f(name, 1);
	int32_t size = f.getLength();
	allocate(size);
	f.read(data, size);
	return size;
}

/* Reads entry i of a Flex file into the buffer; returns its size. */
int32_t FarBuffer::load(char *flexName, int16_t i)
{
	if (data != 0)
		release();
	Flex flex;
	flex.open(flexName);
	FlexEntry entry;
	flex.getEntry(i, &entry);
	int32_t size = entry.size;
	allocate(size);
	flex.readEntry(&entry, data, 0);
	flex.close();
	return size;
}

/* The test assigns, so the buffer is freed whether or not it was allocated here. */
void FarBuffer::release()
{
	if ((owned = 1))
		FreeFarHeap(data);
	owned = 0;
	data = 0;
}

}
