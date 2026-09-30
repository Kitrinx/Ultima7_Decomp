/* Black Gate MAINMENU.EXE, resident segment 11 (file offsets 0x00da59 to 0x00dbfd, 420 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "textfld.h"
#include "itable.h"

TextBox::TextBox(FontTextPrinter *font, char cursor) : TextField(), Control()
{
	printer = font;
	cursorChar = cursor;
	x = 0;
	y = 0;
}

TextBox::TextBox(FontTextPrinter *font, int bufferSize, KeyQueue *queue, char cursor) :
	TextField(bufferSize, queue), Control()
{
	printer = font;
	cursorChar = cursor;
	x = 0;
	y = 0;
}

void TextBox::moveTo(int nx, int ny)
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
	int width;

	saved = printer->target;
	printer->target = this;
	printer->printFormatted(x, y, text);
	if (cursorVisible && active) {
		width = printer->measure(text, cursor);
		FontTextPrinter *p = printer;
		p->move(x + width, y + 1);
		p->printChar(cursorChar);
	}
	printer->target = saved;
}

void TextBox::reject()
{
}

unsigned char TextBox::contains(int px, int py)
{
	return 0;
}
