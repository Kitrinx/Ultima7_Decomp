/* Serpent Isle SI.EXE, overlay segment 306 (file offsets 0x085660 to 0x08593b, 731 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
#include <new>
#include "plat.h"
#include "console.h"

int16_t ConsoleColumns;
int16_t ConsoleRows;
ConsoleWindow *ActiveConsole;
static ConsoleWindow MainConsoleWindow(1, 1, 80, 50, 15, 0, 0, 0, 0);

void InitDebugConsole(void)
{
	ConsoleColumns = 80;
	ConsoleRows = 50;
	ActiveConsole = &MainConsoleWindow;
}

ConsoleWindow::ConsoleWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
	int16_t fg, int16_t bg, int8_t frame, int16_t frameFg, int16_t frameBg)
{
	left = x0;
	top = y0;
	right = x1;
	bottom = y1;
	foreground = fg;
	background = bg;
	border = frame;
	borderForeground = frameFg;
	borderBackground = frameBg;
	cursorX = 1;
	cursorY = 1;
}

void ConsoleWindow::draw()
{
	int16_t i;

	plat_console_window(1, 1, ConsoleColumns, ConsoleRows);
	plat_console_text_color(borderForeground);
	plat_console_text_background(borderBackground);
	if (border) {
		plat_console_goto(left - 1, top - 1);
		plat_console_cputs("\xc9");
		plat_console_goto(right + 1, top - 1);
		plat_console_cputs("\xbb");
		plat_console_goto(left - 1, bottom + 1);
		plat_console_cputs("\xc8");
		plat_console_goto(right + 1, bottom + 1);
		plat_console_cputs("\xbc");
		plat_console_goto(left, top - 1);
		for (i = left; i <= right; i++)
			plat_console_cputs("\xcd");
		plat_console_goto(left, bottom + 1);
		for (i = left; i <= right; i++)
			plat_console_cputs("\xcd");
		for (i = top; i <= bottom; i++) {
			plat_console_goto(left - 1, i);
			plat_console_cputs("\xba");
		}
		for (i = top; i <= bottom; i++) {
			plat_console_goto(right + 1, i);
			plat_console_cputs("\xba");
		}
	}
	select();
	plat_console_clrscr();
}

void ConsoleWindow::select()
{
	plat_console_window(left, top, right, bottom);
	plat_console_text_color(foreground);
	plat_console_text_background(background);
	plat_console_goto(cursorX, cursorY);
}

void ResetConsoleCursor(void)
{
	plat_console_goto(1, 1);
}

ConsoleSwitch::ConsoleSwitch(ConsoleWindow *next)
{
	previous = ActiveConsole;
	previous->cursorX = plat_console_where_x();
	previous->cursorY = plat_console_where_y();
	ActiveConsole = next;
	next->select();
}

ConsoleSwitch::~ConsoleSwitch()
{
	ActiveConsole = previous;
	previous->select();
}

extern "C" void ResetConsoleGlobals(void)
{
	ConsoleColumns = 0;
	ConsoleRows = 0;
	ActiveConsole = 0;
	memset((void *)&MainConsoleWindow, 0, sizeof(MainConsoleWindow));
}

extern "C" void ConstructConsoleGlobals(void)
{
	new (&MainConsoleWindow) ConsoleWindow(1, 1, 80, 50, 15, 0, 0, 0, 0);
}
