/* Serpent Isle MAINMENU.EXE, resident segment 11 (file offsets 0x00e240 to 0x00e41d, 477 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "textfld.h"
#include "../shared/itable.h"

namespace MainMenu {

TextBox::TextBox(FontTextPrinter *font, int8_t cursor) : TextField(), Control()
{
	printer = font;
	cursorChar = cursor;
	x = 0;
	y = 0;
}

TextBox::TextBox(FontTextPrinter *font, int16_t bufferSize, KeyQueue *queue, int8_t cursor) :
	TextField(bufferSize, queue), Control()
{
	printer = font;
	cursorChar = cursor;
	x = 0;
	y = 0;
}

void TextBox::moveTo(int16_t nx, int16_t ny)
{
	x = nx;
	y = ny;
}

void TextBox::saveUnder()
{
}

void TextBox::draw()
{
	display();
}

void TextBox::restoreUnder()
{
}

/* Prints the text, and the cursor after it while it blinks on, into this box's view. */
void TextBox::display()
{
	View *saved;
	int16_t width;

	saved = printer->getTarget();
	printer->setTarget(this);
	/* DOS read a typed '%' as a format, printing stack words */
	printer->printFormatted(x, y, "%s", text);
	if (cursorVisible && active) {
		width = printer->measure(text, cursor);
		printer->print(x + width, y + 1, cursorChar);
	}
	printer->setTarget(saved);
}

void TextBox::reject()
{
}

uint8_t TextBox::contains(int16_t px, int16_t py)
{
	return 0;
}

}
