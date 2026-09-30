/* Black Gate U7.EXE, resident segment 31 (file offsets 0x01907e to 0x01a039, 4027 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "plat.h"
#include <stdio.h>
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
#include "mainctrl.h"

extern int8_t CheatsEnabled;
extern uint8_t FrameRateShown;

extern void ShowDebugMenu();
extern void ShowReadableTexts();
extern void ConsoleBlankLine(int16_t, int16_t);
extern void ConsolePrintAt(int16_t, int16_t, char *, ...);
extern void ConsolePrintAndWait(int16_t, int16_t, char *, ...);
extern void CheatPlaySound();
extern void EmptyPickedContainer();

inline uint8_t WrapDir(int16_t dir) { return dir & 7; }

inline uint8_t HasTarget(NPCRef &npc) { return npc.buffer()->primaryTarget != -1; }
inline uint8_t HasTarget(NPCRef &&npc) { return HasTarget(npc); }

uint8_t DirectionForDirectionKey[11] = { 7, 0, 1, 255, 6, 255, 2, 255, 5, 4, 3 };
uint8_t DirectionForNumberKey[9] = { 5, 4, 3, 6, 255, 2, 7, 0, 1 };
uint8_t SingleStepMode = 0, AudioDisabled = 0, MovementDirection = 255;
char TildeString[] = "~";

static uint8_t TurnAndMoveVehicle(objref vehicle, uint8_t direction)
{
	uint8_t facing, moved = 0;

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

void SteerVehicle(uint8_t direction)
{
	uint8_t diff, facing, dir;
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

void UseAtPointer(int16_t mouseX, int16_t mouseY)
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

void ShowCheatKeys()
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
	CheatPrintfAtCoords(1, 16, GetGameText(6, 14));
	CheatPrintfAtCoords(1, 17, GetGameText(6, 15));
	CheatPrintfAtCoords(1, 18, GetGameText(6, 16));
	CheatPrintfAtCoords(1, 19, GetGameText(6, 17));
	CheatPrintfAtCoords(1, 20, GetGameText(6, 18));
	CheatPrintfAtCoords(1, 21, GetGameText(6, 19));
	CheatPrintfAtCoords(1, 22, GetGameText(6, 20));
	CheatPrintfAtCoords(1, 23, GetGameText(6, 21));
	CheatPrintfAtCoordsWait(1, 25, GetGameText(6, 22));
}

void ShowVersion()
{
	BeginConversation(797); /* scroll */
	FarString text;
	text.append(GameTitle);
	text.append(TildeString);
	text.append(GameCopyright);
	text.append(TildeString);
	text.append(GameVersion);
	text.show();
	EndConversation();
}

void ProcessKey(uint16_t key, int16_t mouseX, int16_t mouseY, uint8_t *steps)
{
	int16_t i, line;

	AvatarStepRequested = 0;
	if (IsNpcUnconscious(&AvatarRef)) {
		*steps = 0;
		if (PlayerActionSuspended)
			return;
		/* Keys an unconscious avatar still answers: Alt-S, Alt-X, F1, F2, Alt-1 to Alt-3 and double-click. */
		switch (key) {
		case 'S': case 'Z': case 's': case 'z':
		case 0x11f: case 0x12d: case 0x13b: case 0x13c:
		case 0x178: case 0x179: case 0x17a: case 0x203:
			break;
		default:
			return;
		}
	}
	if (*steps == 0) {
		/* Keys above 0xff are 0x100 plus a scan code; above 0x200, mouse button events. */
		switch (key) {
		case 0x202: case 0x206:     /* right button pressed or held */
			MovementDirection = (CursorFrame - CursorBase) & 7;
			*steps = GetCursorLength();
			break;
		case 0x201: case 0x205:     /* left button pressed or held */
			LookAtItem();
			break;
		case 0x204:                 /* right double-click */
			if (!CurrentVehicle) {
				Coord x, y;
				int16_t z;
				if (HasTarget(NPCRef(AvatarRef)))
					RemoveFromCombat(NPCRef(AvatarRef));
				PickWorldCoords(mouseX, mouseY, &x, &y, &z, 0);
				StartAutoroute(x, y, z, 0, 0, 0, 0);
			}
			break;
		case 0x203:                 /* left double-click */
			UseAtPointer(mouseX, mouseY);
			break;
		case 0x12d:                 /* Alt-X */
			ShouldExitMainGameLoop = DoYesNoDialog(0);
			break;
		case 'C': case 'c': case 0x12e:
			uint8_t combat;
			combat = IsAvatarInCombat();
			if (combat)
				BreakOffCombat();
			else if (!combat)
				BeginCombat();
			break;
		case 'S': case 's': case 0x11f:
			StartAndLoopNumberedDialog(DIALOG_SAVE);
			break;
		case 'A': case 'a': case 0x11e:
			if (AudioDisabled)
				SetAudioState(2, 2, 2);
			else
				SetAudioState(1, 1, 1);
			AudioDisabled = !AudioDisabled;
			break;
		case 'I': case 'i':
			OpenAndLoopItemDialog(AvatarRef);
			break;
		case 'Z': case 'z': case 0x12c:
			StartAndLoopNumberedDialog(DIALOG_STATS);
			break;
		case 'V': case 'v': case 0x12f:
			ShowVersion();
			break;
		case 'H': case 'h': case 0x123:
			if (MouseHand) {
				SpriteManager_barkOnItem(&gSpriteManager, AvatarRef.off, "Right-handed mouse", 5, 15, 0);
				MouseHand = 0;
			} else {
				SpriteManager_barkOnItem(&gSpriteManager, AvatarRef.off, "Left-handed mouse", 5, 15, 0);
				MouseHand = 1;
			}
			break;
		case 0x13b:                 /* F1 */
			if (CheatsEnabled)
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
			if (CheatsEnabled)
				TeleportByMap();
			break;
		case 0x13e:                 /* F4 */
			if (CheatsEnabled) {
				SingleStepMode = !SingleStepMode;
				gCamera.waitForKey = !gCamera.waitForKey;
			}
			break;
		case 0x13f:                 /* F5 */
			if (CheatsEnabled)
				CheatCastSpell();
			break;
		case 0x141:                 /* F7 */
			if (CheatsEnabled)
				DebugOutputEnabled = !DebugOutputEnabled;
			break;
		case 0x142:                 /* F8 */
			if (CheatsEnabled)
				ShowReadableTexts();
			break;
		case 0x143:                 /* F9 */
			if (CheatsEnabled) {
				objref selected;
				Coord x, y;
				int16_t z;
				HavePlayerSelect(&selected, &x, &y, &z);
				if ((selected.ptr()->typeFrame & 0x3ff) == 607) {   /* path */
					HidePointer();
					for (i = 1; i < 12; i++)
						ConsoleBlankLine(1, i);
					ConsolePrintAt(1, 1, "Path Data");
					ConsolePrintAt(1, 3, "Qual=%3d", (uint8_t)Item_getQuality(&selected));
					ConsolePrintAt(1, 5, "Code=%s", GetGameText(4, (uint8_t)Item_getQuality(&selected) & 31));
					for (i = 32, line = 0; i <= 128; i <<= 1, line++)
						ConsolePrintAt(1, line + 7, "%s:  %c", GetGameText(4, line + 26),
							(int8_t)((uint8_t)Item_getQuality(&selected) & i ? 'Y' : 'N'));
					ConsolePrintAndWait(1, 11, "Hit a key.");
					ShowPointer();
				} else {
					CheatPrintfAtCoordsWait(1, 1, "Ref = %d, @%x %x %x\nType = %d, Frame = %d, Qual = %d)",
						selected.off, x.value, y.value, z, selected.ptr()->typeFrame & 0x3ff,
						(selected.ptr()->typeFrame & 0x7c00) >> 10, (uint8_t)Item_getQuality(&selected));
				}
			}
			break;
		case 0x178:                 /* Alt-1 */
			if (CheatsEnabled)
				CheatPlaySound();
			break;
		case 0x179:                 /* Alt-2 */
			if (CheatsEnabled) {
				FrameRateShown = !FrameRateShown;
				ResetFrameRate();
			}
			break;
		case 0x17a:                 /* Alt-3 */
			if (CheatsEnabled) {
				int16_t sprite, z;
				objref selected;
				Coord x, y;
				CheatPrintfAtCoords(1, 1, "Sprite Number:  ");
				scanf("%d", &sprite);
				HavePlayerSelect(&selected, &x, &y, &z);
				SpriteManager_playSpriteForItem(&gSpriteManager, selected.off, 0, 0, 0, 0, sprite + 1024, 0, -1, 5);
			}
			break;
		case 0x17b:                 /* Alt-4 */
			if (CheatsEnabled)
				EmptyPickedContainer();
			break;
		case 0x17c:                 /* Alt-5 */
			SaveShapeFrameTable();
			break;
		case 'F': case 'f':
			if (CheatsEnabled)
				CheatKeyFToggle = !CheatKeyFToggle;
			break;
		case 'O': case 'o':
			if (CheatsEnabled)
				OcclusionEnabled = !OcclusionEnabled;
			break;
		case 'W': case 'w':
			if (CheatsEnabled)
				CheatKeyWToggle = !CheatKeyWToggle;
			break;
		case 'D': case 'd':
			if (CheatsEnabled)
				CheatKeyDToggle = !CheatKeyDToggle;
			break;
		default:
			if (key >= KEY_HOME && key <= KEY_PAGE_DOWN) {     /* Home to PgDn on the keypad */
				MovementDirection = DirectionForDirectionKey[key - KEY_HOME];
				*steps = 1;
			} else if (key >= (uint16_t)('1') && key <= (uint16_t)('9')) {
				MovementDirection = DirectionForNumberKey[key - '1'];
				*steps = 3;
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

void PollAndProcessKey(uint8_t *steps)
{
	uint16_t key;
	int16_t mouseX, mouseY;

	if (SingleStepMode && *steps == 0)
		while (KeyPressed() == 0)
			plat_yield();
	if (!AvatarDontMove) {
		PollKeyAndTranslateWithMouse(&key, &mouseX, &mouseY);
		ProcessKey(key, mouseX, mouseY, steps);
	}
}
