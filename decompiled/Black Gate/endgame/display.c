/* Black Gate ENDGAME.EXE, resident segment 3 (file offsets 0x008b92 to 0x008da0, 526 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include "display.h"
#include "memsys.h"

Display Screen;
unsigned char GraphicsOn = 0;

/* Switches to mode 13h and makes the screen view. */
void Display::open()
{
	if (!GraphicsOn) {
		Memory.snapshot();
		VideoMode graphics;
		graphics.set(GRAPHICS_MODE);
		SetVideoMode(&graphics.mode);
		FindCrtStatusPort();
		screen.open();
		GraphicsOn = 1;
	}
}

/* Back to the text screen. */
void Display::close()
{
	if (GraphicsOn) {
		GraphicsOn = 0;
		VideoMode text;
		text.set(TEXT_MODE);
		SetVideoMode(&text.mode);
	}
}

/* Shows the memory report against the snapshot taken at open, then closes. */
void Display::shutdown()
{
	ShowMessage(ReportMemory(&Memory));
	close();
}
