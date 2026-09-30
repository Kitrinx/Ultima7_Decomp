/* Black Gate U7.EXE, overlay segment 236 (file offsets 0x06cc60 to 0x06d1a9, 1353 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "plat.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lowlevel.h"
#include "dosio.h"
#include "view.h"
#include "u7event.h"
#include "vooalloc.h"
#include "debug.h"
#include "redscrn.h"
#include "systimer.h"
#include "errors.h"
#include "cheat.h"
#include "colbuf.h"
#include "easyfile.h"
#include "random.h"
#include "freexmm.h"
#include "initwp.h"
#include "memfree.h"
#include "routine.h"
#include "sysusage.h"
#include "text.h"
#include "u7point.h"
#include "vidmode.h"
#include "creeper.h"
#include "screen.h"
#include "init.h"
#include "worldpal.h"
#include "gtimer.h"
#include "colorreg.h"

extern uint8_t SaveLoadActive;
extern "C" int16_t StartFarHeap(int16_t unused);
extern "C" void CloseFarHeap(int16_t unused);
extern "C" int32_t OpenExtendedMemory(void);
extern "C" int32_t GetExtendedMemorySize(void);

inline void Release(char *p) { if (p) delete p; }

uint8_t GameDisplayMode;
uint16_t _ovrbuffer = 6400;     /* overlay buffer, in paragraphs */
uint16_t _stklen = 6000;
char MonoFontSize[3] = { '6', 'x', '7' };
char MonoFontFileName[] = "MONO.FNT";
char MouseFileName[] = "MOUSE.V00";

View ScreenView;
View Viewport;
VoodooBlock VoodooXmsBlock = { 0 };
MemInfo OriginalFreeMemoryStats;
ScreenPalette GameScreen;
uint8_t ShuttingDown = 0;     /* shut down, or shutting down */
char *EndStatsFileName = "ENDSTATS.DAT";
uint8_t ReportingError = 0;       /* an error is being reported */
char NumberLineFormat[] = "%d\n";
uint8_t PlainErrors = 0;      /* report errors without the apology */

/* undo the start-up, once */
void RestoreSystem(void)
{
	char buf[256];

	if (ShuttingDown == 0) {
		ShuttingDown = 1;
		HideCursor();
		if (HackMoverEnabled) {
			strcpy(buf, WorkString);
			plat_log(SprintfMemoryUsage(&OriginalFreeMemoryStats));
			strcpy(WorkString, buf);
		}
		ShutDownSound();
		SystemTimer.SysTimer::~SysTimer();
		RedScreenPicture.RedScreen::~RedScreen();
		CloseFarHeap(0);
		ShutdownXMM();
	}
}

/* show the error and quit */
void FatalError(char *fmt, ...)
{
	va_list args;
	char message[400];

	if (ShuttingDown == 0) {
		ReportingError = 1;
		if (fmt != WorkString) {
			va_start(args, fmt);
			vsprintf(WorkString, fmt, args);
		}
		message[0] = 0;
		if (!PlainErrors)
			strcat(message, "Sorry, an error has occured.\nPlease write down the following\ninformation:\n\n");
		sprintf(message + strlen(message), "%-80.80s\n\n", WorkString);
		if (!PlainErrors)
			strcat(message, "Consult your manual.  If the problem\npersists, please call Origin Customer\n"
				"Service. We are sorry for the inconvenience\n");
		RunFatalHook();
		plat_fatal(message);
	}
}

void AssertFail(char *file, int16_t line)
{
	if (ShuttingDown == 0)
		FatalError("@%s, Line %d\n", file, line);
}

void HaltWithMessage(char *file, int16_t line, char *fmt, ...)
{
	va_list args;
	char buf[100];

	if (ShuttingDown == 0) {
		ReportingError = 1;
		if (fmt != WorkString) {
			va_start(args, fmt);
			vsprintf(WorkString, fmt, args);
		}
		sprintf(buf, "Halting @%s, Line %d\n", file, line);
		strcat(WorkString, buf);
		FatalError(WorkString);
	}
}

/* bring up memory, the screen, the mouse and the sound */
extern "C" void InitEnvironment(void)
{
	char *unusedBuffer;
	int16_t i;

	SaveLoadActive = 1;
	VoodooXmsBlock.base = OpenExtendedMemory();
	VoodooXmsBlock.free = GetExtendedMemorySize();
	VoodooXmsBlock.unusedFlag = 1;
	VoodooXmsBlock.used = 0;
	if (!VoodooXmsBlock.valid())
		FatalError("No Voodoo!");
	StartFarHeap(0);
	GetMemoryInfo(&OriginalFreeMemoryStats);
	SetFatalHook(RestoreSystem);
	SetDisplayMode(GameDisplayMode);
	FillView(&ScreenView, IdentityColorBytes[0]);
	LogMemoryUsage("Start Environment");
	unusedBuffer = new char[64];
	LogMemoryUsage("Workstring");
	Release(WorkString);
	WorkstringSize = 300;
	WorkString = new char[WorkstringSize];
	memset(WorkString, 0xff, WorkstringSize);     /* the original filled 4000 bytes, past the end */
	LogMemoryUsage("vscreen");
	Viewport.clip.x0 = 0;
	Viewport.clip.y0 = 0;
	Viewport.clip.x1 = SCREEN_WIDTH - 1;
	Viewport.clip.y1 = SCREEN_HEIGHT - 1;
	LogMemoryUsage("vports");
	AllocateDrawBuffer(&Viewport, 0, DRAW_IN_XMS);
	LogMemoryUsage("palette");
	Creeper_loadPalettes((Creeper *) &GameScreen);
	GameScreen.state = PAL_DAY;
	for (i = 0; i < 100; i++)
		GenerateRandomIntegerInRange(i + 1);
	LogMemoryUsage("use index");
	InitUsecodeIndex();
	LogMemoryUsage("mouse1");
	SystemTimer.install();
	LogMemoryUsage("mouse2");
	GameInput.initialize();
	LogMemoryUsage("Sound");
	if (!InitSound())
		ReportError(0xe201);
	ViewportFirstRow = GetRowAddress(0, Viewport.rowTable);
	LogMemoryUsage("Text Cache");
	InitTextCache();
	SaveLoadActive = 0;
}

extern "C" void ShutDown(void)
{
	RestoreSystem();
}

void QuitToDos(void)
{
	RestoreSystem();
	plat_exit(3);
}

/* save the date the game ended in ENDSTATS.DAT and quit */
void EndGame(void)
{
	GameDate d;

	GameTime.getDate(&d);
	d.save(BuildPath(StaticPath, EndStatsFileName, 0));
	RestoreSystem();
	plat_exit(4);
}

void ExitForEndgame(void)
{
	RestoreSystem();
	plat_exit(9);
}
