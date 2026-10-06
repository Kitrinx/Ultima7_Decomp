/* Serpent Isle SI.EXE, resident segment 54 (file offsets 0x0218ea to 0x021ac5, 475 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "lowlevel.h"
#include "vooalloc.h"
#include "oops.h"
#include "flxcach.h"

void FlexSpans::alloc(int bytes)
{
	size = bytes;
	block = AllocateVoodooMemory(&VoodooXmsBlock, bytes);
	if (block == 0)
		ReportOutOfVoodooMemory();
}

void FlexSpans::get(unsigned i, void *dst)
{
	CopyLinearToFar(dst, block + i * sizeof(FlexEntry), sizeof(FlexEntry));
}

void FlexSpans::put(unsigned i, void *src)
{
	CopyFarToLinear(block + i * sizeof(FlexEntry), src, sizeof(FlexEntry));
}

unsigned char CachedFlex::getEntry(int i, FlexEntry *entry)
{
	if (!table.loaded()) {
		FlexEntry all;    /* the whole table, read as one entry */

		table.alloc((int) hdr.count * sizeof(FlexEntry));
		all.offset = hdr.valid() ? 128L : 0L;
		all.size = (int) hdr.count * sizeof(FlexEntry);
		readEntryToVoodoo(&all, table.block, 0);
	}
	table.get(i, entry);
	return !entry->empty();
}

void CachedFlex::store(int i, FlexEntry *entry)
{
	table.put(i, entry);
}

void CachedFlex::writeEntry(int i, FlexEntry *entry)
{
	store(i, entry);
	FlexWriter::writeEntry(i, entry);
}

CachedFlex::~CachedFlex()
{
	close();
}
