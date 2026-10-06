/* Serpent Isle MAINMENU.EXE, resident segment 5 (file offsets 0x00bdba to 0x00c030, 630 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include <io.h>
#include "memapi.h"
#include "easyfile.h"
#include "vooalloc.h"
#include "oops.h"
#include "flex.h"
#include "itable.h"

/* Reads a whole shape file into Voodoo memory as the font. */
void FlexTextPrinter::load(char *name)
{
	int handle = OpenFileOrFail(name);
	long data = 0;

	ReadHandleToVoodoo(handle, 0L, filelength(handle), &data);
	setShape(data, 0x111);
}

/* Reads one entry of a Flex file into Voodoo memory as the font. */
void FlexTextPrinter::load(char *flexName, int entry)
{
	Flex flex;

	flex.open(flexName);
	FlexEntry where;
	flex.getEntry(entry, &where);
	long data = AllocateVoodooMemory(&VoodooXmsBlock, where.size);
	if (data == 0)
		ReportOutOfVoodooMemory();
	flex.readEntryToVoodoo(&where, data, 0);
	flex.close();
	setShape(data, 0x111);
}

FlexTextPrinter::~FlexTextPrinter()
{
	reset();
}

void FlexTextPrinter::reset()
{
	if (hasShape() && ownsShape()) {
		FreeFarHeap((void far *)shape);
		setShape(0L, 0);
	}
}
