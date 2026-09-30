/* Black Gate MAINMENU.EXE modules FONTPRN and FLEXPRN, the font printers of the shared ITABLE
 * header: text drawn as frames of a font shape.
 */

#include "u7port.h"
#include "plat.h"
#include "lowlevel.h"
#include "dosio.h"
#include "../main/mem/errors.h"
#include "view.h"
#include "memapi.h"
#include "chkfile.h"
#include "easyfile.h"
#include "vooalloc.h"
#include "oops.h"
#include "flex.h"
#include "itable.h"

namespace Shared {

void FontTextPrinter::setShape(int32_t data, int16_t flags)
{
	shape = data;
	drawFlags = flags;
}

void FontTextPrinter::drawChar(View *view, int16_t px, int16_t py, int8_t c)
{
	DrawFrame(view, px, py, shape, c, drawFlags);
}

int16_t FontTextPrinter::charHeight(int8_t c)
{
	Rect bounds;

	GetFrameBounds(&bounds, 0, 0, shape, c, drawFlags);
	return bounds.y1 - bounds.y0 + leading + 1;
}

int16_t FontTextPrinter::charWidth(int8_t c)
{
	Rect bounds;

	GetFrameBounds(&bounds, 0, 0, shape, c, drawFlags);
	return bounds.x1 - bounds.x0 + spacing + 1;
}

int16_t FontTextPrinter::textWidth(char *text)
{
	return TextPrinter::textWidth(text);
}

/* Width of the first count characters of text. */
int16_t FontTextPrinter::measure(char *text, int16_t count)
{
	int16_t width = 0;
	char c;

	while (count-- && (c = *text++) != 0) {
		width += charWidth(c);
		width += spacing;
	}
	return width;
}

/* Height of the tallest character in text, plus the leading. */
int16_t FontTextPrinter::textHeight(char *text)
{
	int16_t height = 0;
	char c;

	while ((c = *text++) != 0) {
		int16_t h = charHeight(c);
		if (h > height)
			height = h;
	}
	return height + leading;
}

int16_t FontTextPrinter::frameCount()
{
	return GetShapeFrameCount(shape, drawFlags);
}

void FontTextPrinter::printFormatted(int16_t px, int16_t py, char *format, ...)
{
	va_list args;

	if (format != WorkString) {
		va_start(args, format);
		vsnprintf(WorkString, WorkstringSize, format, args);
		va_end(args);
	}
	print(px, py, WorkString);
}

/* Reads a whole shape file into Voodoo memory as the font. */
void FlexTextPrinter::load(char *name)
{
	int16_t handle = OpenFileOrFail(name);
	int32_t data = 0;

	ReadHandleToVoodoo(handle, 0, plat_file_length(handle), &data);
	DosClose(handle);
	setShape(data, 0x111);
}

/* Reads one entry of a Flex file into Voodoo memory as the font. */
void FlexTextPrinter::load(char *flexName, int16_t entry)
{
	Flex flex;
	FlexEntry where;
	int32_t data;

	flex.open(flexName);
	flex.getEntry(entry, &where);
	data = AllocateVoodooMemory(&VoodooXmsBlock, where.size);
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
		setShape(0, 0);
	}
}

}
