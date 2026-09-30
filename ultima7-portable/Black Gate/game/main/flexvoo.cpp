/* Black Gate U7.EXE, overlay segment 233 (file offsets 0x06c520 to 0x06c71b, 507 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "vooalloc.h"
#include "oops.h"
#include "flexvoo.h"

void VoodooFlex::alloc(int32_t n)
{
	size = n;
	addr = AllocateVoodooMemory(&VoodooXmsBlock, size);
	if (addr == 0)
		ReportOutOfVoodooMemory();
}

uint8_t VoodooFlex::load(char *name)
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

uint8_t VoodooFlex::getEntry(int16_t n, FlexEntry *e)
{
	if (n >= hdr.count) {
		e->size = 0;
		e->offset = 0;
	} else
		CopyLinearToFar(e, addr + n * sizeof(FlexEntry) + sizeof(FlexHeader), sizeof(FlexEntry));
	return !e->empty();
}

uint8_t VoodooFlex::readEntryToVoodoo(FlexEntry *e, int32_t block, uint8_t quiet)
{
	if (!e->empty())
		MoveLinearFlat(block, addr + e->offset, e->size);
	return !e->empty();
}

uint8_t VoodooFlex::readEntry(FlexEntry *e, void *buf, uint8_t quiet)
{
	if (!e->empty())
		CopyLinearToFar(buf, addr + e->offset, e->size);
	return !e->empty();
}
