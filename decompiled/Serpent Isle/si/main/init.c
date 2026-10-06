/* Serpent Isle SI.EXE, overlay segment 224 (file offsets 0x05e4b0 to 0x05ea6f, 1471 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

#include <stdarg.h>
#include <dos.h>
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
#include "xmminit.h"

/* the BIOS video mode to go back to; 3 is color text */
struct VideoMode {
	unsigned char mode;
	VideoMode() : mode(3) {}
};

extern unsigned char SaveLoadActive;
extern "C" int StartFarHeap(int unused);
extern "C" void CloseFarHeap(int unused);
void far InitDebugConsole(void);
void far ResetFrameTiming(void);

inline void Release(char *p) { if (p) delete p; }

unsigned char GameDisplayMode;
unsigned _ovrbuffer = 6400;     /* overlay buffer, in paragraphs */
unsigned _stklen = 6000;
char MonoFontSize[3] = { '6', 'x', '7' };
char MonoFontFileName[] = "MONO.FNT";
char MouseFileName[] = "MOUSE.V00";

View ScreenView;
View Viewport;
HookRecord *InterruptHookList = 0;
VoodooBlock VoodooXmsBlock = { 0 };
VideoMode OriginalVideoMode;
MemInfo OriginalFreeMemoryStats;
ScreenPalette GameScreen;
CtrlBreakTrap CtrlBreakHook;
CtrlCTrap CtrlCHook;
BiosHook SystemServicesHook;
DivideTrap DivideErrorHook;
unsigned char ShuttingDown = 0;     /* shut down, or shutting down */
char *EndStatsFileName = "ENDSTATS.DAT";
unsigned char ReportingError = 0;       /* an error is being reported */
char NumberLineFormat[] = "%d\n";
unsigned char PlainErrors = 0;      /* report errors without the apology */

/* undo the start-up, once */
void far RestoreSystem(void)
{
	char buf[256];

	if (ShuttingDown == 0) {
		ShuttingDown = 1;
		HideCursor();
		if (!ReportingError)
			SetVideoMode(&OriginalVideoMode.mode);
		if (HackMoverEnabled) {
			strcpy(buf, WorkString);
			printf("%s", SprintfMemoryUsage(&OriginalFreeMemoryStats));
			strcpy(WorkString, buf);
		}
		ShutDownSound();
		SystemTimer.SysTimer::~SysTimer();
		RedScreenPicture.RedScreen::~RedScreen();
		CloseFarHeap(0);
		ShutdownXMM();
	}
}

/* back to text mode, print the error and quit */
void far FatalError(char *fmt, ...)
{
	va_list args;

	if (ShuttingDown == 0) {
		ReportingError = 1;
		OriginalVideoMode.mode = 3;
		SetVideoMode(&OriginalVideoMode.mode);
		if (fmt != WorkString) {
			va_start(args, fmt);
			vsprintf(WorkString, fmt, args);
		}
		if (!PlainErrors)
		printf("Sorry, an error has occurred.\nPlease write down the following\ninformation:\n\n");
		printf("%-80.80s\n\n", WorkString);
		if (!PlainErrors)
			printf("Consult your manual.  If the problem\npersists, please call Origin Customer\n"
				"Service. We are sorry for the inconvenience\n");
		RunFatalHook();
		exit(1);
	}
}

void far AssertFail(char *file, int line)
{
	if (ShuttingDown == 0)
		FatalError("@%s, Line %d\n", file, line);
}

void far HaltWithMessage(char *file, int line, char *fmt, ...)
{
	va_list args;
	char buf[100];

	if (ShuttingDown == 0) {
		ReportingError = 1;
		OriginalVideoMode.mode = 3;
		SetVideoMode(&OriginalVideoMode.mode);
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
extern "C" void far InitEnvironment(void)
{
	unsigned long overlaySize = 0;
	int overlayStatus = 0xff01;
	char *unusedBuffer;
	char statusMessage[80];
	int i;

	SaveLoadActive = 1;
	InitDebugConsole();
	GetVideoMode(&OriginalVideoMode.mode);
	VoodooXmsBlock.base = OpenExtendedMemory();
	VoodooXmsBlock.free = GetExtendedMemorySize();
	VoodooXmsBlock.unusedFlag = 1;
	VoodooXmsBlock.used = 0;
	if (!VoodooXmsBlock.valid())
		FatalError("No Voodoo!");
	if (XMSPresent && OverlayMemoryKb) {
		overlaySize = OverlayMemoryKb << 10;
		overlayStatus = _OvrInitExt(0L, overlaySize);
	}
	sprintf(statusMessage, "Status: %d Size: %luv\n\n", overlayStatus, overlaySize);
	SetDisplayMode(GameDisplayMode);
	FillView(&ScreenView, IdentityColorBytes[0]);
	LogMemoryUsage("Starting Memory");
	StartFarHeap(0);
	GetMemoryInfo(&OriginalFreeMemoryStats);
	SetFatalHook(RestoreSystem);
	SetDosVerify(0);
	LogMemoryUsage("Start Environment");
	unusedBuffer = new char[64];
	LogMemoryUsage("Workstring");
	Release(WorkString);
	WorkstringSize = 300;
	WorkString = new char[WorkstringSize];
	memset(WorkString, -1, 300);
	LogMemoryUsage("vscreen");
	Viewport.clip.x = 0;
	Viewport.clip.y = 0;
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
	ViewportFirstRow = GetRowAddress(0, Viewport.rowTable.get());
	LogMemoryUsage("Text Cache");
	InitTextCache();
	ResetFrameTiming();
	SaveLoadActive = 0;
}

extern "C" void far ShutDown(void)
{
	RestoreSystem();
}

void far QuitToDos(void)
{
	RestoreSystem();
	exit(3);
}

/* save the date the game ended in ENDSTATS.DAT and quit */
void far EndGame(void)
{
	GameDate d;

	GameTime.getDate(&d);
	d.save(BuildPath(StaticPath, EndStatsFileName, 0));
	RestoreSystem();
	exit(4);
}

void far ExitForEndgame(void)
{
	RestoreSystem();
	exit(9);
}
