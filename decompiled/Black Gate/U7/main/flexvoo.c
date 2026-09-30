/* Black Gate U7.EXE, overlay segment 233 (file offsets 0x06c520 to 0x06c71b, 507 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "lowlevel.h"
#include "vooalloc.h"
#include "oops.h"
#include "flexvoo.h"

void VoodooFlex::alloc(long n)
{
	size = n;
	addr = AllocateVoodooMemory(&VoodooXmsBlock, size);
	if (addr == 0)
		ReportOutOfVoodooMemory();
}

unsigned char VoodooFlex::load(char *name)
{
	FlexEntry whole;

	if (open(name)) {
		whole.offset = 0;
		whole.size = getFileLength();
		Flex::readEntryToVoodoo(&whole, addr, 0);
		close();
		setName(0);
		return 1;
	}
	hdr.count = -1;
	setName(0);
	return 0;
}

void VoodooFlex::release()
{
}

unsigned char VoodooFlex::getEntry(int n, FlexEntry *e)
{
	if (n >= hdr.count) {
		e->size = 0;
		e->offset = 0;
	} else
		CopyLinearToFar(e, addr + n * sizeof(FlexEntry) + sizeof(FlexHeader), sizeof(FlexEntry));
	return !e->empty();
}

unsigned char VoodooFlex::readEntryToVoodoo(FlexEntry *e, long block, unsigned char quiet)
{
	if (!e->empty())
		MoveLinearFlat(block, addr + e->offset, e->size);
	return !e->empty();
}

unsigned char VoodooFlex::readEntry(FlexEntry *e, void far *buf, unsigned char quiet)
{
	if (!e->empty())
		CopyLinearToFar(buf, addr + e->offset, e->size);
	return !e->empty();
}
