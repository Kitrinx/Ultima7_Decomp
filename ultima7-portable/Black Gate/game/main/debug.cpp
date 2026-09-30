/* Black Gate U7.EXE, resident segment 16 (file offsets 0x0121f4 to 0x01254b, 855 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "plat.h"
#include <stdio.h>
#include <stdarg.h>
#include "dosio.h"
#include "cheat.h"
#include "debug.h"
#include "u7event.h"

char *GameTitle = TitleString;
char *GameVersion = VersionString;
char *GameCopyright = CopyrightString;
char *StaticPath = 0;
char *GamedatPath = 0;
char *BuildStamp = BuildStampString;
uint8_t ShouldExitMainGameLoop = 0;
uint8_t DebugOutputEnabled = 0;
char TitleString[] = "Ultima VII - The Black Gate";
char VersionString[] = "ver 3.4";
char CopyrightString[] = "(C) 1991, 1992 Origin Systems Inc.";
/* a stamping tool fills the blanks after the marker */
char BuildStampString[] = "!Stamp!"
	"                                                                "
	"\nUltima VII - The Black Gate\nver 3.4\nCreated Jun 02 1992 18:50:49"
	"\n(C) 1991, 1992 Origin Systems Inc.";

static char TgCodeFormat[] = "TG Code:%0x";

static void LogTgCode(int16_t code)
{
	char text[20];

	sprintf(text, TgCodeFormat, code);
	plat_log(text);
}

void DebugPrintf(char *fmt, ...)
{
	va_list args;

	if (DebugOutputEnabled) {
		if (fmt != WorkString) {
			va_start(args, fmt);
			vsprintf(WorkString, fmt, args);
		}
		plat_log(WorkString);
	}
}

void DebugPrintfAtCoords(int16_t x, int16_t y, char *fmt, ...)
{
	va_list args;

	if (DebugOutputEnabled) {
		if (fmt != WorkString) {
			va_start(args, fmt);
			vsprintf(WorkString, fmt, args);
		}
		plat_log(WorkString);
	}
}

void DebugPrintTgCode(int16_t code)
{
	if (DebugOutputEnabled) {
		LogTgCode(code);
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
		plat_log(WorkString);
		plat_sleep(100);
		while (KeyPressed())
			ReadKey();
		while (!KeyPressed())
			plat_yield();
		ReadKey();
	}
}

void DebugPrintfAtCoordsWait(int16_t x, int16_t y, char *fmt, ...)
{
	va_list args;

	if (DebugOutputEnabled) {
		if (fmt != WorkString) {
			va_start(args, fmt);
			vsprintf(WorkString, fmt, args);
		}
		plat_log(WorkString);
		plat_sleep(100);
		while (KeyPressed())
			ReadKey();
		while (!KeyPressed())
			plat_yield();
		ReadKey();
	}
}

void DebugPrintTgCodeWait(int16_t code)
{
	if (DebugOutputEnabled) {
		LogTgCode(code);
		plat_sleep(100);
		while (KeyPressed())
			ReadKey();
		while (!KeyPressed())
			plat_yield();
		ReadKey();
	}
}

void CheatPrintf(char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
	}
	if (CheatsEnabled)
		plat_log(WorkString);
}

void CheatPrintfAtCoords(int16_t x, int16_t y, char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
	}
	if (CheatsEnabled) {
		plat_log(WorkString);
	}
}

void CheatPrintfWait(char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
	}
	if (CheatsEnabled) {
		plat_log(WorkString);
		plat_sleep(100);
		while (KeyPressed())
			ReadKey();
		while (!KeyPressed())
			plat_yield();
		ReadKey();
	}
}

void CheatPrintfAtCoordsWait(int16_t x, int16_t y, char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
	}
	if (CheatsEnabled) {
		plat_log(WorkString);
		plat_sleep(100);
		while (KeyPressed())
			ReadKey();
		while (!KeyPressed())
			plat_yield();
		ReadKey();
	}
}
