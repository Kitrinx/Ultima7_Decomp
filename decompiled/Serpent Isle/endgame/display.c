/* Serpent Isle ENDGAME.EXE, resident segment 3 (file offsets 0x00836e to 0x008588, 538 bytes).
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

/* Optionally restores text mode when graphics closes. */
void Display::close(int restoreText)
{
	if (GraphicsOn) {
		GraphicsOn = 0;
		if (restoreText) {
			VideoMode text;
			text.set(TEXT_MODE);
			SetVideoMode(&text.mode);
		}
	}
}

/* Shows the memory report against the snapshot taken at open, then closes. */
void Display::shutdown()
{
	ShowMessage(ReportMemory(&Memory));
	close(0);
}
