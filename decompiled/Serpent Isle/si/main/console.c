/* Serpent Isle SI.EXE, overlay segment 306 (file offsets 0x085660 to 0x08593b, 731 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include <conio.h>
#include "console.h"

int ConsoleColumns;
int ConsoleRows;
ConsoleWindow *ActiveConsole;

void far InitDebugConsole(void)
{
	struct text_info info;

	gettextinfo(&info);
	ConsoleColumns = 80;
	ConsoleRows = 50;
	static ConsoleWindow mainWindow(1, 1, ConsoleColumns, ConsoleRows, 15, 0, 0, 0, 0);
	ActiveConsole = &mainWindow;
}

ConsoleWindow::ConsoleWindow(int x0, int y0, int x1, int y1,
	int fg, int bg, char frame, int frameFg, int frameBg)
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
	int i;

	window(1, 1, ConsoleColumns, ConsoleRows);
	textcolor(borderForeground);
	textbackground(borderBackground);
	if (border) {
		gotoxy(left - 1, top - 1);
		cprintf("\xc9");
		gotoxy(right + 1, top - 1);
		cprintf("\xbb");
		gotoxy(left - 1, bottom + 1);
		cprintf("\xc8");
		gotoxy(right + 1, bottom + 1);
		cprintf("\xbc");
		gotoxy(left, top - 1);
		for (i = left; i <= right; i++)
			cprintf("\xcd");
		gotoxy(left, bottom + 1);
		for (i = left; i <= right; i++)
			cprintf("\xcd");
		for (i = top; i <= bottom; i++) {
			gotoxy(left - 1, i);
			cprintf("\xba");
		}
		for (i = top; i <= bottom; i++) {
			gotoxy(right + 1, i);
			cprintf("\xba");
		}
	}
	select();
	clrscr();
}

void ConsoleWindow::select()
{
	window(left, top, right, bottom);
	textcolor(foreground);
	textbackground(background);
	gotoxy(cursorX, cursorY);
}

void ResetConsoleCursor(void)
{
	gotoxy(1, 1);
}

ConsoleSwitch::ConsoleSwitch(ConsoleWindow *next)
{
	previous = ActiveConsole;
	previous->cursorX = wherex();
	previous->cursorY = wherey();
	ActiveConsole = next;
	next->select();
}

ConsoleSwitch::~ConsoleSwitch()
{
	ActiveConsole = previous;
	previous->select();
}
