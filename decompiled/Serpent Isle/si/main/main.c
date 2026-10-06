/* Serpent Isle SI.EXE, resident segment 21 (file offsets 0x01383c to 0x013abf, 643 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "iteminfo.h"
#include "objref.h"
#include "u7event.h"
#include "cheat.h"
#include "attack.h"
#include "preload.h"
#include "gtimer.h"
#include "u7point.h"
#include "savegame.h"
#include "actqueue.h"
#include "sche.h"
#include "itable.h"
#include "movepath.h"
#include "partymov.h"
#include "speed.h"
#include "camera.h"
#include "mainctrl.h"
#include "colbuf.h"
#include "init.h"
#include "item.h"
#include "u7sound.h"

extern objref AvatarRef;
extern unsigned char ShouldExitMainGameLoop;

unsigned char RestartRequested = 0;
unsigned char EndgameRequested = 0;
unsigned char PendingSteps = 0;
unsigned char EndgameQuitRequested = 0;
int TimeAdvanceRate = 0;

void far AdvanceTime(void)
{
	int savedRate;
	int i;

	savedRate = GameTime.rate;
	GameTime.rate = TimeAdvanceRate;
	GameTime.tick();
	GameTime.rate = savedRate;
	UpdateNpcSchedules();
	for (i = 0; i < 30; i++) {
		UpdateNPCs(0);
		GameTime.tick();
		RunActionQueue();
	}
	TimeAdvanceRate = 0;
}

void far RunGameStep(void)
{
	FlushKeyboard();
	SetPointerZ(Item_getZ(&AvatarRef));
	if (ShowAvatarLocation)
		ConsolePrintAt(1, 1, "L=(%x,%x,%x)", (int) Item_getX(AvatarRef),
			(int) Item_getY(AvatarRef), Item_getZ(&AvatarRef));
	AutorouteActive = ContinueAutoroute();
	UpdatePartyFollow();
	if ((char) (GameTime.ticks % TICKS_PER_HOUR == 0) || !PendingSteps)
		UpdateNPCStatus();
	RunActionQueue();
	UpdateNPCs(PendingSteps > 0);
	if (!PendingSteps && TimeAdvanceRate == 0) {
		WaitForFrame();
		if (FrameRateShown)
			MeasureFrameRate();
		DrawWorld(&gCamera);
	}
	GameTime.tick();
	if (TimeAdvanceRate == 0)
		PollAndProcessKey(&PendingSteps);
	else
		AdvanceTime();
	if (MusicChangeDue)
		PlayMusic(RequestedMusic);
}

extern "C" void far MainGameLoop(void)
{
	while (!ShouldExitMainGameLoop) {
		AdjustCursorZ(Item_getZ(&AvatarRef) - CursorCenterZ);
		gCamera.setTarget(AvatarRef);
		DrawWorld(&gCamera);
		/* clear the avatar's scripted flag */
		Item_setQualityFlags(&AvatarRef, Item_getQualityFlags(&AvatarRef) & 0xdf);
		RestartRequested = 0;
		EndgameRequested = 0;
		while (!ShouldExitMainGameLoop && !RestartRequested && !EndgameRequested && !EndgameQuitRequested)
			RunGameStep();
		if (RestartRequested) {
			SaveGameArgs();
			SaveGameFiles.deleteGameDirectory();
			QuitToDos();
		}
		if (EndgameRequested)
			EndGame();
		if (EndgameQuitRequested)
			ExitForEndgame();
	}
}
