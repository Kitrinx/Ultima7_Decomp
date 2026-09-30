/* Black Gate shared module FARBUF, linked into MAINMENU.EXE and INTRO.EXE: far heap buffers
 * loaded from files.
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

/* Frees the data whether or not it was allocated here, as the original's test assigned instead
 * of comparing; the heap ignores a block it does not hold. */
void FarBuffer::release()
{
	if (data)
		FreeFarHeap(data);
	owned = 0;
	data = 0;
}

}
