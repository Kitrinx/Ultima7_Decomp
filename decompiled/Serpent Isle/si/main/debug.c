/* Serpent Isle SI.EXE, resident segment 11 (file offsets 0x010af0 to 0x010ce5, 501 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include <stdio.h>
#include <stdarg.h>
#include <conio.h>
#include <dos.h>
#include "dosio.h"
#include "debug.h"

char *GameTitle = TitleString;
char *GameVersion = VersionString;
char *GameCopyright = CopyrightString;
char *StaticPath = 0;
char *GamedatPath = 0;
char *BuildStamp = BuildStampString;
unsigned char ShouldExitMainGameLoop = 0;
unsigned char DebugOutputEnabled = 0;
char TitleString[] = "Ultima VII Part II - Serpent Isle             ";
char VersionString[] = "Ver 1.02s5 Final";
char CopyrightString[] = "(C) 1993 Origin Systems Inc.";
/* Build metadata follows the reserved stamp field. */
char BuildStampString[] = "!Stamp!"
	"                                                                "
	"\nUltima VII Part II - Serpent Isle             \nVer 1.02s5 Final\nCreated Jul 14 1993 12:43:24"
	"\n(C) 1993 Origin Systems Inc.";

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
}

void CheatPrintfAtCoords(int x, int y, char *fmt, ...)
{
}

void CheatPrintfWait(char *fmt, ...)
{
}

void CheatPrintfAtCoordsWait(int x, int y, char *fmt, ...)
{
}
