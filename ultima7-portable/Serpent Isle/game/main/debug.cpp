/* Serpent Isle SI.EXE, resident segment 11 (file offsets 0x010af0 to 0x010ce5, 501 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "plat.h"
#include <stdio.h>
#include <stdarg.h>
#include "dosio.h"
#include "debug.h"
#include "u7event.h"

char *const GameTitle = (char *)TitleString;
char *const GameVersion = (char *)VersionString;
char *const GameCopyright = (char *)CopyrightString;
char *StaticPath = 0;
char *GamedatPath = 0;
char *const BuildStamp = (char *)BuildStampString;
uint8_t ShouldExitMainGameLoop = 0;
uint8_t DebugOutputEnabled = 0;
const char TitleString[] = "Ultima VII Part II - Serpent Isle             ";
const char VersionString[] = "Ver 1.02s5 Final";
const char CopyrightString[] = "(C) 1993 Origin Systems Inc.";
/* Build metadata follows the reserved stamp field. */
const char BuildStampString[] = "!Stamp!"
	"                                                                "
	"\nUltima VII Part II - Serpent Isle             \nVer 1.02s5 Final\nCreated Jul 14 1993 12:43:24"
	"\n(C) 1993 Origin Systems Inc.";

static const char TgCodeFormat[] = "TG Code:%0x";

/* printf in the DOS game: onto the game screen at the text cursor, where \n started a new line;
 * copied to the log. */
void ConsoleWrite(const char *text)
{
	char line[128];
	const char *at;
	int16_t n = 0;

	for (at = text; *at; at++) {
		if (*at != '\n')
			line[n++] = *at;
		if (*at == '\n' || n == (int16_t) sizeof line - 1) {
			line[n] = '\0';
			plat_console_write(line);
			n = 0;
		}
		if (*at == '\n')
			plat_console_write("\r\n");
	}
	line[n] = '\0';
	plat_console_write(line);
	plat_log(text);
}

/* gotoxy and cprintf: onto the game screen at column x, row y; copied to the log. */
void ConsoleWriteAt(int16_t x, int16_t y, const char *text)
{
	plat_console_goto(x, y);
	plat_console_cputs(text);
	plat_log(text);
	plat_log("\n");
}

static void LogTgCode(int16_t code)
{
	char text[20];

	sprintf(text, TgCodeFormat, code);
	ConsoleWrite(text);
}

void DebugPrintf(char *fmt, ...)
{
	va_list args;

	if (DebugOutputEnabled) {
		if (fmt != WorkString) {
			va_start(args, fmt);
			vsprintf(WorkString, fmt, args);
		}
		ConsoleWrite(WorkString);
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
		ConsoleWriteAt(x, y, WorkString);
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
		ConsoleWrite(WorkString);
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
		plat_console_goto(x, y);
		ConsoleWrite(WorkString);
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
}

/* A line typed on the game screen, as DOS read one: echoed, Backspace erases, Enter ends it. */
static void ConsoleReadLine(char *line, int16_t size)
{
	int16_t length = 0;
	char echo[2] = {0, 0};
	int16_t key;

	for (;;) {
		key = ReadKey();
		if (key == '\r') {
			ConsoleWrite("\n");
			break;
		}
		if (key == '\b') {
			if (length > 0) {
				length--;
				plat_console_write("\b \b");
			}
		} else if (key >= ' ' && key < 0x7f && length < size - 1) {
			line[length++] = (char) key;
			echo[0] = (char) key;
			plat_console_write(echo);
		}
	}
	line[length] = '\0';
}

/* The next word typed, as scanf("%s") read it: blank lines are skipped. */
void ConsoleReadWord(char *word, int16_t size)
{
	char line[128];
	char *start;
	size_t length;

	for (;;) {
		ConsoleReadLine(line, sizeof line);
		start = line + strspn(line, " \t");
		length = strcspn(start, " \t");
		if (length == 0)
			continue;
		if (length >= (size_t) size)
			length = size - 1;
		memcpy(word, start, length);
		word[length] = '\0';
		return;
	}
}

/* The next number typed, as scanf("%ld") or "%lx" read it. */
int32_t ConsoleReadNumber(int8_t hex)
{
	char word[32];

	ConsoleReadWord(word, sizeof word);
	return (int32_t) strtol(word, 0, hex ? 16 : 10);
}

void CheatPrintfAtCoords(int16_t x, int16_t y, char *fmt, ...)
{
}

void CheatPrintfWait(char *fmt, ...)
{
}

void CheatPrintfAtCoordsWait(int16_t x, int16_t y, char *fmt, ...)
{
}

extern "C" void ResetDebugGlobals(void)
{
	StaticPath = 0;
	GamedatPath = 0;
	ShouldExitMainGameLoop = 0;
	DebugOutputEnabled = 0;
}
