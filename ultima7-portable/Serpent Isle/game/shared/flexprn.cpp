/* Serpent Isle MAINMENU.EXE, resident segment 5 (file offsets 0x00bdba to 0x00c030, 630 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "plat.h"
#include "dosio.h"
#include "memapi.h"
#include "easyfile.h"
#include "vooalloc.h"
#include "oops.h"
#include "flex.h"
#include "itable.h"

namespace Shared {

/* Reads a whole shape file into Voodoo memory as the font. */
void FlexTextPrinter::load(char *name)
{
	int16_t handle = OpenFileOrFail(name);
	int32_t data = 0;

	ReadHandleToVoodoo(handle, INT32_C(0), plat_file_length(handle), &data);
	setShape(data, 0x111);
}

/* Reads one entry of a Flex file into Voodoo memory as the font. */
void FlexTextPrinter::load(char *flexName, int16_t entry)
{
	Flex flex;

	flex.open(flexName);
	FlexEntry where;
	flex.getEntry(entry, &where);
	int32_t data = AllocateVoodooMemory(&VoodooXmsBlock, where.size);
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
		FreeFarHeap(LinearToPointer(shape));
		setShape(INT32_C(0), 0);
	}
}

}
