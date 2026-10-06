/* Serpent Isle SI.EXE, resident segment 22 (file offsets 0x013abf to 0x014ee2, 5155 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include <stdio.h>
#include <conio.h>
#include "iteminfo.h"
#include "item.h"
#include "coord.h"
#include "mapview.h"
#include "u7manage.h"
#include "target.h"
#include "u7npc.h"
#include "use.h"
#include "u7event.h"
#include "combatai.h"
#include "sprite.h"
#include "itable.h"
#include "combat.h"
#include "look.h"
#include "lsgump.h"
#include "movepath.h"
#include "preload.h"
#include "colbuf.h"
#include "gumpmgr.h"
#include "convmgr.h"
#include "debug.h"
#include "palctrl.h"
#include "speed.h"
#include "text.h"
#include "tools.h"
#include "u7ibuf.h"
#include "barge.h"
#include "farstr.h"
#include "attack.h"
#include "bogus.h"
#include "camera.h"
#include "u7point.h"
#include "msclick.h"
#include "cullmask.h"
#include "palfade.h"
#include "npcref.h"
#include "search.h"
#include "usehook.h"
#include "keyring.h"
#include "cheat.h"
#include "mainctrl.h"

#define SHAPE_CLOTH_MAP     178
#define SHAPE_KEY_RING      485
#define SHAPE_JAWBONE       555
#define SHAPE_LOCKPICK      627
#define SHAPE_DESK_ITEM     675
#define SHAPE_SPELLBOOK     761
#define SHAPE_SCROLL        797
#define FRAME_POCKETWATCH   21
#define FN_FEED_MEMBER      1557

void CheatPlayMusic(void);
char PromptForUsecodeEvent(char *event, int *index);
void ShowTargetReport(void);

inline unsigned char WrapDir(int dir) { return dir & 7; }

inline unsigned char HasTarget(NPCRef &npc) { return npc.buffer()->primaryTarget != -1; }

unsigned char DirectionForDirectionKey[11] = { 7, 0, 1, 255, 6, 255, 2, 255, 5, 4, 3 };
unsigned char DirectionForNumberKey[9] = { 5, 4, 3, 6, 255, 2, 7, 0, 1 };
unsigned char SingleStepMode = 0, AudioDisabled = 0, MovementDirection = 255;
char TildeString[] = "~";

static unsigned char far TurnAndMoveVehicle(objref vehicle, unsigned char direction)
{
	unsigned char facing, moved = 0;

	facing = Barge_getDir(vehicle);
	if (Barge_canTurn(vehicle, direction)) {
		Barge_turn(vehicle, direction);
		if (Barge_canMove(vehicle, direction, 4)) {
			Barge_move(vehicle, direction, 4);
			moved = 1;
		} else {
			Barge_turn(vehicle, facing);
		}
	}
	return moved;
}

void far SteerVehicle(unsigned char direction)
{
	unsigned char diff, facing, dir;
	objref vehicle;

	vehicle = objref(ActiveBarge);
	if (vehicle.valid()) {
		facing = Barge_getDir(vehicle);
		diff = WrapDir(direction - facing);
		switch (diff) {
		case 4:
			dir = WrapDir(direction - 2);
			if (Barge_canTurn(vehicle, dir)) {
				Barge_turn(vehicle, dir);
				if (Barge_canMove(vehicle, direction, 4))
					Barge_move(vehicle, direction, 4);
				break;
			}
			/* fall through */
		case 0:
			if (Barge_canMove(vehicle, direction, 4))
				Barge_move(vehicle, direction, 4);
			else if (Barge_canMove(vehicle, WrapDir(direction - 1), 4))
				Barge_move(vehicle, WrapDir(direction - 1), 4);
			else if (Barge_canMove(vehicle, WrapDir(direction + 1), 4))
				Barge_move(vehicle, WrapDir(direction + 1), 4);
			break;
		case 1:
		case 7:
			if (Barge_canMove(vehicle, direction, 4))
				Barge_move(vehicle, direction, 4);
			else if (Barge_canMove(vehicle, facing, 4))
				Barge_move(vehicle, facing, 4);
			else {
				dir = WrapDir(direction + diff);
				TurnAndMoveVehicle(vehicle, dir);
			}
			break;
		case 2:
		case 6:
			if (!TurnAndMoveVehicle(vehicle, direction) && Barge_canMove(vehicle, direction, 4))
				Barge_move(vehicle, direction, 4);
			break;
		case 3:
		case 5:
			if (diff == 3)
				dir = WrapDir(direction - 1);
			else
				dir = WrapDir(direction + 1);
			if (Barge_canTurn(vehicle, dir))
				Barge_turn(vehicle, dir);
			if (Barge_canMove(vehicle, direction, 4))
				Barge_move(vehicle, direction, 4);
			else if (Barge_canMove(vehicle, dir, 4))
				Barge_move(vehicle, dir, 4);
			else if (Barge_canMove(vehicle, WrapDir(facing + 4), 4))
				Barge_move(vehicle, WrapDir(facing + 4), 4);
			break;
		}
	}
}

void far UseAtPointer(int mouseX, int mouseY)
{
	objref selected;

	FindItemAtScreenPoint(&selected, mouseX, mouseY, &MainWorldView, &gShapeManager, 0);
	if (selected.valid()) {
		if (IsNpcUnconscious(&AvatarRef)) {
			if (selected == AvatarRef)
				StartAndLoopNumberedDialog(DIALOG_SAVE);
		} else {
			Use(&objref(selected.off), Item_getX(selected), Item_getY(selected), Item_getZ(&selected));
		}
	} else {
		Use(&objref(selected.off),
			Coord((mouseX - 7) / 8 + MainWorldView.centerX - 20),
			Coord((mouseY - 7) / 8 + MainWorldView.centerY - 12), 0);
	}
}

void far ShowCheatKeys()
{
	CheatPrintfAtCoords(1, 1, GetGameText(6, 0));
	CheatPrintfAtCoords(1, 2, GetGameText(6, 1));
	CheatPrintfAtCoords(1, 3, GetGameText(6, 2));
	CheatPrintfAtCoords(1, 4, GetGameText(6, 3));
	CheatPrintfAtCoords(1, 5, GetGameText(6, 4));
	CheatPrintfAtCoords(1, 6, GetGameText(6, 5));
	CheatPrintfAtCoords(1, 7, GetGameText(6, 6));
	CheatPrintfAtCoords(1, 8, GetGameText(6, 7));
	CheatPrintfAtCoords(1, 9, GetGameText(6, 8));
	CheatPrintfAtCoords(1, 10, GetGameText(6, 9));
	CheatPrintfAtCoords(1, 11, GetGameText(6, 10));
	CheatPrintfAtCoords(1, 12, GetGameText(6, 11));
	CheatPrintfAtCoords(1, 13, GetGameText(6, 12));
	CheatPrintfAtCoords(1, 14, GetGameText(6, 13));
	CheatPrintfAtCoords(1, 15, GetGameText(6, 14));
	CheatPrintfAtCoords(1, 16, GetGameText(6, 15));
	CheatPrintfAtCoords(1, 17, GetGameText(6, 16));
	CheatPrintfAtCoords(1, 18, GetGameText(6, 17));
	CheatPrintfAtCoords(1, 19, GetGameText(6, 18));
	CheatPrintfAtCoords(1, 20, GetGameText(6, 19));
	CheatPrintfAtCoords(1, 21, GetGameText(6, 20));
	CheatPrintfAtCoords(1, 22, GetGameText(6, 21));
	CheatPrintfAtCoords(1, 23, GetGameText(6, 22));
	CheatPrintfAtCoordsWait(1, 25, GetGameText(6, 23));
}

void far ShowVersion()
{
	BeginConversation(SHAPE_SCROLL);
	FarString text;
	text.append(GameTitle);
	text.append(TildeString);
	text.append(GameCopyright);
	text.append(TildeString);
	text.append(GameVersion);
	text.show();
	EndConversation();
}

void far ProcessKey(unsigned key, int mouseX, int mouseY, unsigned char *steps)
{
	AvatarStepRequested = 0;
	if (IsNpcUnconscious(&AvatarRef)) {
		*steps = 0;
		if (PlayerActionSuspended)
			return;
		/* Keys an unconscious avatar still answers: Alt-S, Alt-X, F1, F2, Alt-1 to Alt-3, Alt-6 and double-click. */
		switch (key) {
		case 'S': case 'Z': case 's': case 'z':
		case 0x11f: case 0x12d: case 0x13b: case 0x13c:
		case 0x178: case 0x179: case 0x17a: case 0x17d: case 0x203:
			break;
		default:
			return;
		}
	}
	if (*steps == 0) {
		/* Keys above 0xff are 0x100 plus a scan code; above 0x200, mouse button events. */
		switch (key) {
		case 0x202: case 0x206:     /* right button pressed or held */
			if (!AvatarDontMove) {
				MovementDirection = (CursorFrame - CursorBase) & 7;
				*steps = GetCursorLength();
			}
			break;
		case 0x201: case 0x205:     /* left button pressed or held */
			if (InGumpMode() || !AvatarDontMove)
				LookAtItem();
			break;
		case 0x204:                 /* right double-click */
			if (!CurrentVehicle && !AvatarDontMove) {
				Coord x, y;
				int z;
				if (HasTarget(NPCRef(AvatarRef)))
					RemoveFromCombat(NPCRef(AvatarRef));
				PickWorldCoords(mouseX, mouseY, &x, &y, &z, 0);
				StartAutoroute(x, y, z, 0, 0, 0, 0, 0);
			}
			break;
		case 0x203:                 /* left double-click */
			if (!AvatarDontMove)
				UseAtPointer(mouseX, mouseY);
			break;
		case 0x12d:                 /* Alt-X */
			ShouldExitMainGameLoop = DoYesNoDialog(0);
			break;
		case 'C': case 'c': case 0x12e:
			if (!AvatarDontMove) {
				unsigned char combat;
				combat = IsAvatarInCombat();
				if (combat)
					BreakOffCombat();
				else if (!combat)
					BeginCombat();
			}
			break;
		case 'S': case 's': case 0x11f:
			if (!AvatarDontMove)
				StartAndLoopNumberedDialog(DIALOG_SAVE);
			break;
		case 'L': case 'l': case 0x126:
			if (!AvatarDontMove)
				StartAndLoopNumberedDialog(DIALOG_COMBAT);
			break;
		case 'B': case 'b': case 0x130:
			if (!AvatarDontMove) {
				AreaSearch search;
				FindItemInContainer(&search, AvatarRef, 0, SHAPE_SPELLBOOK, 255, 255);
				if (search.found())
					OpenAndLoopItemDialog(search.current);
			}
			break;
		case 'J': case 'j': case 0x124:
			if (!AvatarDontMove) {
				AreaSearch search;
				FindItemInContainer(&search, AvatarRef, 0, SHAPE_JAWBONE, 255, 255);
				if (search.found())
					OpenAndLoopItemDialog(search.current);
			}
			break;
		case 'T': case 't': case 0x114:
			if (!AvatarDontMove) {
				objref selected;
				Coord x, y;
				int z;
				HavePlayerSelect(&selected, &x, &y, &z);
				Use(&objref(selected.off), x, y, z);
			}
			break;
		case 'M': case 'm': case 0x132:
			if (!AvatarDontMove) {
				objref nobody = 0;      /* anyone in the party */
				if (CountHeldItems(1, nobody, SHAPE_CLOTH_MAP, 255, 0))
					ShowWorldMap(0);
			}
			break;
		case 'A': case 'a': case 0x11e:
			if (!AvatarDontMove) {
				ClearAllBarks(AvatarRef.off);
				if (AudioDisabled) {
					SpriteManager_barkOnItem(&gSpriteManager, AvatarRef.off, GetGameText(3, 247), 5, 15, 0);
					SetAudioState(2, 2, 2);
				} else {
					SpriteManager_barkOnItem(&gSpriteManager, AvatarRef.off, GetGameText(3, 248), 5, 15, 0);
					SetAudioState(1, 1, 1);
				}
				AudioDisabled = !AudioDisabled;
			}
			break;
		case 'I': case 'i':
			if (!AvatarDontMove)
				OpenAndLoopItemDialog(AvatarRef);
			break;
		case 'Z': case 'z': case 0x12c:
			if (!AvatarDontMove)
				StartAndLoopNumberedDialog(DIALOG_STATS);
			break;
		case 'V': case 'v': case 0x12f:
			if (!AvatarDontMove)
				ShowVersion();
			break;
		case 'H': case 'h': case 0x123:
			if (!AvatarDontMove) {
				ClearAllBarks(AvatarRef.off);
				if (MouseHand) {
					SpriteManager_barkOnItem(&gSpriteManager, AvatarRef.off, GetGameText(3, 171), 5, 15, 0);
					MouseHand = 0;
				} else {
					SpriteManager_barkOnItem(&gSpriteManager, AvatarRef.off, GetGameText(3, 172), 5, 15, 0);
					MouseHand = 1;
				}
			}
			break;
		case 'K': case 'k': case 0x125:
			if (!AvatarDontMove) {
				AreaSearch search;
				if (FindItemInContainer(&search, AvatarRef, 0, SHAPE_KEY_RING, 255, 0))
					RunUsable(1, search.current.off, SHAPE_KEY_RING);
			}
			break;
		case 'F': case 'f': case 0x121:
			if (!AvatarDontMove)
				RunUsable(0, 0, FN_FEED_MEMBER);
			break;
		case 'P': case 'p':
			if (!AvatarDontMove) {
				AreaSearch search;
				if (FindItemInContainer(&search, AvatarRef, 0, SHAPE_LOCKPICK, 255, 255))
					RunUsable(1, search.current.off, SHAPE_LOCKPICK);
			}
			break;
		case 0x13b:                 /* F1 */
			if (CheatsEnabled && !AvatarDontMove)
				ShowCheatKeys();
			break;
		case 0x13c:                 /* F2 */
			if (CheatsEnabled) {
				SetFixedPalette(1);
				ShowDebugMenu();
				RestoreGamePalette();
			}
			break;
		case 0x13d:                 /* F3 */
			if (CheatsEnabled && !AvatarDontMove)
				TeleportByMap();
			break;
		case 0x119:                 /* Alt-P */
			if (CheatsEnabled) {
				SingleStepMode = !SingleStepMode;
				gCamera.waitForKey = !gCamera.waitForKey;
			}
			break;
		case 0x13f:                 /* F5 */
			if (CheatsEnabled && !AvatarDontMove)
				CheatCastSpell();
			break;
		case 0x141:                 /* F7 */
			if (CheatsEnabled && !AvatarDontMove)
				DebugOutputEnabled = !DebugOutputEnabled;
			break;
		case 0x142:                 /* F8 */
			if (CheatsEnabled && !AvatarDontMove)
				ShowReadableTexts();
			break;
		case 0x143:                 /* F9 */
			if (CheatsEnabled && !AvatarDontMove)
				ShowTargetReport();
			break;
		case '`':
			if (CheatsEnabled && !AvatarDontMove) {
				objref selected;
				Coord x, y;
				int z;
				char event;
				int function;
				HavePlayerSelect(&selected, &x, &y, &z);
				SetFixedPalette(1);
				if (PromptForUsecodeEvent(&event, &function))
					RunUsable(event, selected.off, function);
				RestoreGamePalette();
			}
			break;
		case 0x178:                 /* Alt-1 */
			if (CheatsEnabled && !AvatarDontMove)
				CheatPlaySound();
			break;
		case 0x179:                 /* Alt-2 */
			if (CheatsEnabled && !AvatarDontMove) {
				FrameRateShown = !FrameRateShown;
				ResetFrameRate();
			}
			break;
		case 0x17a:                 /* Alt-3 */
			if (CheatsEnabled && !AvatarDontMove) {
				int sprite, z;
				objref selected;
				Coord x, y;
				CheatPrintfAtCoords(1, 1, GetGameText(3, 175));
				scanf("%d", &sprite);
				HavePlayerSelect(&selected, &x, &y, &z);
				SpriteManager_playSpriteForItem(&gSpriteManager, selected.off, 0, 0, 0, 0, sprite + 1024, 0, -1, 5);
			}
			break;
		case 0x17b:                 /* Alt-4 */
			if (CheatsEnabled && !AvatarDontMove)
				EmptyPickedContainer();
			break;
		case 0x17c:                 /* Alt-5 */
			SaveShapeFrameTable();
			break;
		case 0x17d:                 /* Alt-6 */
			if (CheatsEnabled && !AvatarDontMove)
				CheatPlayMusic();
			break;
		case 0x17e:                 /* Alt-7 */
			if (CheatsEnabled && !AvatarDontMove) {
				int z;
				Coord x, y;
				objref selected;
				HavePlayerSelect(&selected, &x, &y, &z);
				if (selected.valid()) {
					Item_clearOkayToTake(&selected);
					Item_setTemporary(&selected);
				}
			}
			break;
		case 0x17f:                 /* Alt-8 */
			if (!AvatarDontMove) {
				ClearAllBarks(AvatarRef.off);
				if (ToggleFrameTiming())
					SpriteManager_barkOnItem(&gSpriteManager, AvatarRef.off, GetGameText(3, 173), 5, 15, 0);
				else
					SpriteManager_barkOnItem(&gSpriteManager, AvatarRef.off, GetGameText(3, 174), 5, 15, 0);
			}
			break;
		case 0x180:                 /* Alt-9 */
			if (!AvatarDontMove)
				KeyRing.dump();
			break;
		case 'O': case 'o':
			if (CheatsEnabled && !AvatarDontMove)
				OcclusionEnabled = !OcclusionEnabled;
			break;
		case 'W': case 'w':
			if (!AvatarDontMove) {
				AreaSearch search;
				if (FindItemInContainer(&search, AvatarRef, 0, SHAPE_DESK_ITEM, 255, FRAME_POCKETWATCH))
					RunUsable(1, search.current.off, SHAPE_DESK_ITEM);
			}
			break;
		case 'D': case 'd':
			if (CheatsEnabled && !AvatarDontMove)
				CheatKeyDToggle = !CheatKeyDToggle;
			break;
		default:
			if (!AvatarDontMove) {
				if (key >= KEY_HOME && key <= KEY_PAGE_DOWN) {     /* Home to PgDn on the keypad */
					MovementDirection = DirectionForDirectionKey[key - KEY_HOME];
					*steps = 1;
				} else if (key >= '1' && key <= '9') {
					MovementDirection = DirectionForNumberKey[key - '1'];
					*steps = 3;
				}
			}
		}
	}
	if (*steps && !CurrentVehicle) {
		AvatarStepRequested = 1;
		if (HasTarget(NPCRef(AvatarRef)))
			RemoveFromCombat(NPCRef(AvatarRef));
	}
	if (*steps && MovementDirection != -1) {
		if (CurrentVehicle)
			SteerVehicle(MovementDirection);
		else
			StepAvatar(MovementDirection, 1);
		--*steps;
		if (*steps == 0)
			MovementDirection = -1;
	}
}

void far PollAndProcessKey(unsigned char *steps)
{
	unsigned key;
	int mouseX, mouseY;

	if (SingleStepMode && *steps == 0)
		while (kbhit() == 0)
			;
	PollKeyAndTranslateWithMouse(&key, &mouseX, &mouseY);
	ProcessKey(key, mouseX, mouseY, steps);
}
