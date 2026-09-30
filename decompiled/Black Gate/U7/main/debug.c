/* Black Gate U7.EXE, resident segment 16 (file offsets 0x0121f4 to 0x01254b, 855 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include <stdio.h>
#include <stdarg.h>
#include <conio.h>
#include <dos.h>
#include <iostream.h>
#include "dosio.h"
#include "cheat.h"
#include "debug.h"

char *GameTitle = TitleString;
char *GameVersion = VersionString;
char *GameCopyright = CopyrightString;
char *StaticPath = 0;
char *GamedatPath = 0;
char *BuildStamp = BuildStampString;
unsigned char ShouldExitMainGameLoop = 0;
unsigned char DebugOutputEnabled = 0;
char TitleString[] = "Ultima VII - The Black Gate";
char VersionString[] = "ver 3.4";
char CopyrightString[] = "(C) 1991, 1992 Origin Systems Inc.";
/* a stamping tool fills the blanks after the marker */
char BuildStampString[] = "!Stamp!"
	"                                                                "
	"\nUltima VII - The Black Gate\nver 3.4\nCreated Jun 02 1992 18:50:49"
	"\n(C) 1991, 1992 Origin Systems Inc.";

static char TgCodeFormat[] = "TG Code:%0x";

void DebugPrintf(char *fmt, ...)
{
	va_list args;

	if (DebugOutputEnabled) {
		if (fmt != WorkString) {
			va_start(args, fmt);
			vsprintf(WorkString, fmt, args);
		}
		directvideo = 0;
		printf(WorkString);
	}
}

void DebugPrintfAtCoords(int x, int y, char *fmt, ...)
{
	va_list args;

	if (DebugOutputEnabled) {
		if (fmt != WorkString) {
			va_start(args, fmt);
			vsprintf(WorkString, fmt, args);
		}
		directvideo = 0;
		gotoxy(x, y);
		cprintf(WorkString);
	}
}

void DebugPrintTgCode(int code)
{
	if (DebugOutputEnabled) {
		directvideo = 0;
		printf(TgCodeFormat, code);
	}
}

void DebugPrintfWait(char *fmt, ...)
{
	va_list args;

	if (DebugOutputEnabled) {
		if (fmt != WorkString) {
			va_start(args, fmt);
			vsprintf(WorkString, fmt, args);
		}
		directvideo = 0;
		printf(WorkString);
		delay(100);
		while (kbhit())
			getch();
		while (!kbhit())
			;
		getch();
	}
}

void DebugPrintfAtCoordsWait(int x, int y, char *fmt, ...)
{
	va_list args;

	if (DebugOutputEnabled) {
		if (fmt != WorkString) {
			va_start(args, fmt);
			vsprintf(WorkString, fmt, args);
		}
		directvideo = 0;
		gotoxy(x, y);
		printf(WorkString);
		delay(100);
		while (kbhit())
			getch();
		while (!kbhit())
			;
		getch();
	}
}

void DebugPrintTgCodeWait(int code)
{
	if (DebugOutputEnabled) {
		directvideo = 0;
		printf(TgCodeFormat, code);
		delay(100);
		while (kbhit())
			getch();
		while (!kbhit())
			;
		getch();
	}
}

void CheatPrintf(char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
	}
	directvideo = 0;
	if (CheatsEnabled)
		printf(WorkString);
}

void CheatPrintfAtCoords(int x, int y, char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
	}
	directvideo = 0;
	if (CheatsEnabled) {
		gotoxy(x, y);
		cprintf(WorkString);
	}
}

void CheatPrintfWait(char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
	}
	directvideo = 0;
	if (CheatsEnabled) {
		printf(WorkString);
		delay(100);
		while (kbhit())
			getch();
		while (!kbhit())
			;
		getch();
	}
}

void CheatPrintfAtCoordsWait(int x, int y, char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
	}
	directvideo = 0;
	if (CheatsEnabled) {
		gotoxy(x, y);
		printf(WorkString);
		delay(100);
		while (kbhit())
			getch();
		while (!kbhit())
			;
		getch();
	}
}
