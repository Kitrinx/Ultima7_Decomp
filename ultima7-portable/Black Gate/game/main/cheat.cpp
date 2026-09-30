/* Black Gate U7.EXE, overlay segment 217 (file offsets 0x056e90 to 0x05adb6, 16166 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "plat.h"
#include "u7event.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "itemrec.h"
#include "iteminfo.h"
#include "activity.h"
#include "dosio.h"
#include "type.h"
#include "npcref.h"
#include "u7npc.h"
#include "coord.h"
#include "combat.h"
#include "maps.h"
#include "sortitem.h"
#include "gtimer.h"
#include "combmode.h"
#include "debug.h"
#include "equip.h"
#include "target.h"
#include "party.h"
#include "sche_ov1.h"
#include "search.h"
#include "sounds.h"
#include "text.h"
#include "usehook.h"
#include "wihh.h"
#include "itable.h"
#include "u7manage.h"
#include "combatai.h"
#include "missile.h"
#include "misstrac.h"
#include "npcpath.h"
#include "sche.h"
#include "legalmov.h"
#include "mapview.h"
#include "cheat.h"
#include "flags.h"
#include "item.h"

inline uint8_t IsNpcType(TypeFrame typeFrame)
{
	return (ItemTypeClassFlags[gItemTypeInfo[typeFrame.type()].typeClass] & CLASS_NPC) != 0;
}

extern uint8_t KillNpcMode;
extern int8_t AvatarInCombat;
extern uint8_t Item_getQuality(objref *ref);
extern Coord Item_getX(objref &);
extern Coord Item_getY(objref &);
extern uint8_t Item_getQualityFlags(objref *);
extern void Item_setQualityFlags(objref *, uint8_t);
extern void Item_setInvisible(objref *);
extern int8_t Item_getHitPoints(objref *ref);
extern void Item_setHitPoints(objref *, uint8_t);
extern uint8_t CreateItemInContainer(objref *ref, TypeFrame type, objref container);
extern int8_t Item_delete(objref *ref);
extern uint8_t CreateItem(objref *, TypeFrame);
extern void Item_setQuality(objref *, int8_t);
extern uint8_t ZapDetachedItem(objref *ref);
extern uint8_t Item_move(objref *ref, Loc x, Loc y, int16_t z);
extern uint8_t CreateItem(objref *, TypeFrame, CellCoord, CellCoord, int16_t);
extern void Item_setOkayToTake(objref *);
extern void Item_setTemporary(objref *);
extern void Item_setFrame(objref *, int16_t);
extern uint8_t Item_hasQuantity(objref *ref);
extern void Item_storeQuantity(objref *, uint8_t);
extern uint8_t Item_hasQuality(objref *ref);

#define NPC(ref) GetNpcBufferForIbo(ref)
#define YN(c) ((c) ? 'Y' : 'N')

int8_t CheatsEnabled;
uint8_t FrameRateShown;
char PromptWordBuffer[50];
int16_t DoScheduleNpc = -1;
uint8_t ShowNpcNumbers = 0;
uint8_t ShowAvatarLocation = 0;
uint8_t PowerAvatar = 0;
uint8_t QueueToggle = 0;
uint8_t UnkBugChecking = 1;
uint8_t StatusLineRaised = 1;
uint8_t FollowersEnabled = 0;

/* prints at column x, row y; the host log has no columns or rows */
void ConsolePrintAt(int16_t x, int16_t y, char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
	}
	plat_log(WorkString);
}

/* prints at column x, row y, then waits for a key */
void ConsolePrintAndWait(int16_t x, int16_t y, char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
	}
	plat_log(WorkString);
	while (KeyPressed())
		ReadKey();
	while (!KeyPressed())
		plat_yield();
	ReadKey();
}

/* blanks 40 columns */
void ConsoleBlankLine(int16_t x, int16_t y)
{
	char line[50];

	strcpy(line, "");
	while (strlen(line) < 40)
		strcat(line, " ");
	ConsolePrintAt(x, y, "%s", line);
}

/* shows a message on the status line and waits for a key */
void ConsoleShowMessage(char *msg)
{
	char line[50];

	strcpy(line, msg);
	if (line[0])
		strcat(line, "  ");
	strcat(line, "Hit a key.");
	while (strlen(line) < 40)
		strcat(line, " ");
	ConsolePrintAt(1, 25 - StatusLineRaised, "%s", line);
	ConsolePrintAndWait(1, 23, "");
}

/* prompts on the status line and reads a word */
char *PromptForWord(char *prompt)
{
	char line[50];

	strncpy(line, prompt, 50);
	while (strlen(line) < 40)
		strcat(line, " ");
	ConsolePrintAt(1, 25 - StatusLineRaised, "%s", line);
	ConsolePrintAt(1, 23, "Select->          ");
	ConsolePrintAt(9, 23, "");
	fflush(stdin);
	scanf("%s", PromptWordBuffer);
	return PromptWordBuffer;
}

/* prompts on the status line and reads a number, in hex when asked */
int32_t PromptForLong(char *prompt, int8_t hex)
{
	int32_t value;
	char line[50];

	strncpy(line, prompt, 50);
	while (strlen(line) < 40)
		strcat(line, " ");
	ConsolePrintAt(1, 25 - StatusLineRaised, "%s", line);
	ConsolePrintAt(1, 23, "Select->          ");
	ConsolePrintAt(9, 23, "");
	fflush(stdin);
	if (hex)
		scanf("%lx", &value);
	else
		scanf("%ld", &value);
	return value;
}

int16_t PromptForIntegerWord(char *prompt)
{
	int16_t value = PromptForLong(prompt, 0);

	return value;
}

/* prompts on the status line and reads one key */
int8_t PromptForKey(char *prompt)
{
	int8_t key;
	char line[50];

	strncpy(line, prompt, 50);
	while (strlen(line) < 40)
		strcat(line, " ");
	ConsolePrintAt(1, 25 - StatusLineRaised, "%s", line);
	ConsolePrintAt(1, 23, "Select->          ");
	ConsolePrintAt(9, 23, "");
	fflush(stdin);
	key = ReadKey();
	return key;
}

/* clears the left panel */
void ClearLeftPanel(void)
{
	int16_t y;

	for (y = 1; y <= 11; y++)
		ConsoleBlankLine(1, y);
}

/* clears the right panel */
void ClearRightPanel(void)
{
	int16_t y;

	for (y = 13; y < 22; y++)
		ConsoleBlankLine(13, y);
}

/* prints in the right panel */
void PrintInRightPanel(int16_t x, int16_t y, char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
	}
	ConsolePrintAt(x, y + 13, WorkString);
}

/* prints in the left panel */
void PrintInLeftPanel(int16_t x, int16_t y, char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
	}
	ConsolePrintAt(x, y + 1, WorkString);
}

/* shows and changes an NPC's opponent and targets */
void EditNpcTargets(NPCRef *npc)
{
	objref who;
	int8_t cmd;
	int8_t key;
	int16_t number;

	for (;;) {
		ClearLeftPanel();
		ClearRightPanel();
		PrintInLeftPanel(1, 0, "Npc #%3d", (uint16_t)Item_getNpcNumber(npc));
		PrintInRightPanel(1, 0, "%s%3d", GetGameText(5, 39), GetNpcBufferForIbo(npc)->primaryTarget);
		PrintInRightPanel(1, 1, "%s%3d", GetGameText(5, 40), GetNpcBufferForIbo(npc)->secondaryTarget);
		PrintInRightPanel(1, 2, "%s%3d", GetGameText(5, 41), GetNpcBufferForIbo(npc)->oppressor);
		PrintInRightPanel(1, 8, GetGameText(5, 42));
		key = PromptForKey(GetGameText(5, 43));
		switch (cmd = key >= 'a' && key <= 'z' ? key - 32 : key) {
		case 'O':
		case 'P':
		case 'S':
			number = PromptForIntegerWord(GetGameText(5, 44));
			GetNpcIbo(&who, number);
			if (who.valid()) {
				if (cmd == 'O')
					GetNpcBufferForIbo(npc)->oppressor = (uint16_t)Item_getNpcNumber(&who);
				else if (cmd == 'P')
					Npc_setTarget(npc, (uint16_t)Item_getNpcNumber(&who), 1);
				else
					Npc_setTarget(npc, (uint16_t)Item_getNpcNumber(&who), 0);
			} else if (number == -1) {
				if (cmd == 'O')
					GetNpcBufferForIbo(npc)->oppressor = -1;
				else if (cmd == 'P')
					Npc_setTarget(npc, -1, 1);
				else
					Npc_setTarget(npc, -1, 0);
			} else
				ConsoleShowMessage(GetGameText(5, 45));
			break;
		case 'X':
			return;
		}
	}
}

/* shows and changes an NPC's attack mode */
void EditNpcAttackMode(NPCRef *npc)
{
	int8_t cmd, n, key;
	int16_t i;

	ClearLeftPanel();
	ClearRightPanel();
	for (i = 0; i < 9; i++)
		PrintInLeftPanel(1, i, "[%d]  %s", i, GetGameText(5, i + 47));
	if ((int8_t)Item_isAvatar(npc))
		PrintInLeftPanel(1, 9, "[9]  %s    ", GetGameText(5, 56));
	PrintInLeftPanel(1, 10, "[X]  Abort");
	PrintInRightPanel(1, 8, "[X]it");
	do {
		PrintInRightPanel(1, 0, "%s%s    ", GetGameText(5, 63),
			GetNpcBufferForIbo(npc)->workType == WORK_COMBAT ? GetGameText(5, Item_getQuality(npc) + 47) : "None");
		PrintInRightPanel(1, 1, "%s%s    ", GetGameText(5, 64),
			GetGameText(5, GetNpcBufferForIbo(npc)->defaultAttackMode + 47));
		key = PromptForKey("Enter command.");
		if (key >= 'a' && key <= 'z')
			cmd = key - 32;
		else
			cmd = key;
		if (cmd == 'C' || cmd == 'D') {
			do {
				n = PromptForKey("Enter no. of attack mode.");
				if (n == 'x' || n == 'X')
					break;
			} while (n < '0' || n > '9');
			if (n >= '0' && n <= '9') {
				if (cmd == 'C')
					SetAttackMode(*npc, n - '0');
				else
					GetNpcBufferForIbo(npc)->defaultAttackMode = (int16_t) n - '0';
			}
		} else if (cmd != 'X')
			ConsoleShowMessage("Invalid command.");
	} while (cmd != 'X');
}

/* moves the avatar to typed coordinates, to an NPC or to an item */
void ShowTeleportMenu(void)
{
	int8_t cmd;
	int16_t x, y, z;
	int16_t number;
	int32_t value;
	objref who;
	objref item;
	int8_t key;

	ClearLeftPanel();
	ClearRightPanel();
	PrintInLeftPanel(1, 0, GetGameText(5, 66));
	PrintInRightPanel(1, 0, GetGameText(5, 67));
	PrintInRightPanel(1, 1, GetGameText(5, 68));
	PrintInRightPanel(1, 2, GetGameText(5, 69));
	PrintInRightPanel(1, 3, GetGameText(5, 70));
	PrintInRightPanel(1, 8, GetGameText(5, 71));
	for (;;) {
		key = PromptForKey(GetGameText(5, 72));
		cmd = key >= 'a' && key <= 'z' ? key - 32 : key;
		if (cmd == 'D' || cmd == 'H' || cmd == 'N' || cmd == 'R') {
			switch (cmd) {
			case 'D':
				x = PromptForLong(GetGameText(5, 73), 0);
				y = PromptForLong(GetGameText(5, 74), 0);
				z = PromptForLong(GetGameText(5, 75), 0);
				TeleportParty(x, y, z);
				break;
			case 'H':
				x = PromptForLong(GetGameText(5, 73), 1);
				y = PromptForLong(GetGameText(5, 74), 1);
				z = PromptForLong(GetGameText(5, 75), 1);
				TeleportParty(x, y, z);
				break;
			case 'N':
				number = PromptForIntegerWord(GetGameText(5, 76));
				GetNpcIbo(&who, number);
				if (who.valid())
					TeleportParty(Item_getX(who), Item_getY(who), (int16_t)Item_getZ(&who));
				else
					ConsoleShowMessage(GetGameText(5, 77));
				break;
			case 'R':
				value = PromptForLong(GetGameText(5, 78), 0);
				if (value == -1)
					break;
				item.off = value;
				if (!item.valid()) {
					ConsoleShowMessage(GetGameText(5, 79));
					break;
				}
				TeleportParty(Item_getX(item), Item_getY(item), (int16_t)Item_getZ(&item));
				break;
			}
		} else if (cmd == 'X')
			return;
		else
			ConsoleShowMessage(GetGameText(5, 80));
	}
}

/* shows and toggles an NPC's flags */
void EditNpcFlags(objref *npc)
{
	int8_t cmd;
	int8_t key;

	for (;;) {
		ClearLeftPanel();
		ClearRightPanel();
		PrintInLeftPanel(1, 0, "%s%3d", GetGameText(5, 82), (uint16_t)Item_getNpcNumber(npc));
		PrintInRightPanel(1, 0, "%s%c", GetGameText(5, 83), YN(HasStatus(npc, NPC_ASLEEP)));
		PrintInRightPanel(1, 1, "%s%c", GetGameText(5, 84), YN(HasStatus(npc, NPC_CHARMED)));
		PrintInRightPanel(1, 2, "%s%c", GetGameText(5, 85), YN(HasStatus(npc, NPC_CURSED)));
		PrintInRightPanel(1, 3, "%s%c", GetGameText(5, 86), YN(HasStatus(npc, NPC_PARALYZED)));
		PrintInRightPanel(1, 4, "%s%c", GetGameText(5, 87), YN(HasStatus(npc, NPC_POISONED)));
		PrintInRightPanel(1, 5, "%s%c", GetGameText(5, 88), YN(HasStatus(npc, NPC_PROTECTED)));
		PrintInRightPanel(1, 6, "%s%c", GetGameText(5, 89), YN(HasStatus(npc, NPC_DEAD)));
		PrintInRightPanel(1, 7, "%s%c", GetGameText(5, 90),
			YN((uint8_t) (GetNpcBufferForIbo(npc)->typeFlagsHigh & 0x20)));
		PrintInRightPanel(14, 0, "%s%c", GetGameText(5, 91), YN(HasStatus(npc, NPC_IN_PARTY)));
		PrintInRightPanel(14, 1, "%s%c", GetGameText(5, 92),
			YN((uint8_t) (Item_getQualityFlags(npc) & QUALITY_INVISIBLE)));
		PrintInRightPanel(14, 2, "%s%c", GetGameText(5, 93),
			YN((uint8_t) (GetNpcBufferForIbo(npc)->typeFlags & NPC_FLY)));
		PrintInRightPanel(14, 3, "%s%c", GetGameText(5, 94),
			YN((uint8_t) (GetNpcBufferForIbo(npc)->typeFlags & NPC_WALK)));
		PrintInRightPanel(14, 4, "%s%c", GetGameText(5, 95),
			YN((uint8_t) (GetNpcBufferForIbo(npc)->typeFlags & NPC_SWIM)));
		PrintInRightPanel(14, 5, "%s%c", GetGameText(5, 96),
			YN((uint8_t) (GetNpcBufferForIbo(npc)->typeFlags & NPC_ETHEREAL)));
		PrintInRightPanel(14, 6, "%s%c", GetGameText(5, 97),
			YN(CombatGroups.isProtectee((uint16_t)Item_getNpcNumber(npc))));
		PrintInRightPanel(14, 7, "%s%c", GetGameText(5, 98),
			YN((uint8_t) (GetNpcBufferForIbo(npc)->typeFlagsHigh & 0x40)));
		PrintInRightPanel(27, 0, "%s%c", GetGameText(5, 99),
			YN((uint8_t) (GetNpcBufferForIbo(npc)->typeFlagsHigh & 0x80)));
		PrintInRightPanel(27, 1, "%s%c", GetGameText(5, 100),
			YN((uint8_t) (GetNpcBufferForIbo(npc)->typeFlagsHigh & 4)));
		if (!(uint16_t)Item_getNpcNumber(npc))
			PrintInRightPanel(27, 2, "%s%c", GetGameText(5, 101), Npc_isMale(npc) ? 'M' : 'F');
		PrintInRightPanel(1, 8, GetGameText(5, 102));
		key = PromptForKey(GetGameText(5, 103));
		switch (cmd = key >= 'a' && key <= 'z' ? key - 32 : key) {
		case 'A':
			if (HasStatus(npc, NPC_ASLEEP))
				ChangeStatus(npc, NPC_ASLEEP, 0);
			else
				ChangeStatus(npc, 0, NPC_ASLEEP);
			break;
		case 'B':
			if (HasStatus(npc, NPC_CHARMED))
				ChangeStatus(npc, NPC_CHARMED, 0);
			else
				ChangeStatus(npc, 0, NPC_CHARMED);
			break;
		case 'C':
			if (HasStatus(npc, NPC_CURSED))
				ChangeStatus(npc, NPC_CURSED, 0);
			else
				ChangeStatus(npc, 0, NPC_CURSED);
			break;
		case 'D':
			if (HasStatus(npc, NPC_PARALYZED))
				ChangeStatus(npc, NPC_PARALYZED, 0);
			else
				ChangeStatus(npc, 0, NPC_PARALYZED);
			break;
		case 'E':
			if (HasStatus(npc, NPC_POISONED))
				ChangeStatus(npc, NPC_POISONED, 0);
			else
				ChangeStatus(npc, 0, NPC_POISONED);
			break;
		case 'F':
			if (HasStatus(npc, NPC_PROTECTED))
				ChangeStatus(npc, NPC_PROTECTED, 0);
			else
				ChangeStatus(npc, 0, NPC_PROTECTED);
			break;
		case 'G':
			if (HasStatus(npc, NPC_DEAD))
				ChangeStatus(npc, NPC_DEAD, 0);
			else
				ChangeStatus(npc, 0, NPC_DEAD);
			break;
		case 'H':
			if ((uint8_t) (GetNpcBufferForIbo(npc)->typeFlagsHigh & 0x20))
				GetNpcBufferForIbo(npc)->typeFlagsHigh = GetNpcBufferForIbo(npc)->typeFlagsHigh & ~0x20;
			else
				GetNpcBufferForIbo(npc)->typeFlagsHigh = GetNpcBufferForIbo(npc)->typeFlagsHigh | 0x20;
			break;
		case 'I':
			if (HasStatus(npc, NPC_IN_PARTY))
				RemoveFromParty(*npc, 1);
			else
				AddToParty(*npc, 0);
			break;
		case 'J':
			if ((uint8_t) (Item_getQualityFlags(npc) & QUALITY_INVISIBLE))
				Item_setQualityFlags(npc, Item_getQualityFlags(npc) & ~1);
			else
				Item_setInvisible(npc);
			break;
		case 'K':
			if ((uint8_t) (GetNpcBufferForIbo(npc)->typeFlags & NPC_FLY))
				GetNpcBufferForIbo(npc)->typeFlags = GetNpcBufferForIbo(npc)->typeFlags & ~NPC_FLY;
			else
				GetNpcBufferForIbo(npc)->typeFlags = GetNpcBufferForIbo(npc)->typeFlags | NPC_FLY;
			break;
		case 'L':
			if ((uint8_t) (GetNpcBufferForIbo(npc)->typeFlags & NPC_WALK))
				GetNpcBufferForIbo(npc)->typeFlags = GetNpcBufferForIbo(npc)->typeFlags & ~NPC_WALK;
			else
				GetNpcBufferForIbo(npc)->typeFlags = GetNpcBufferForIbo(npc)->typeFlags | NPC_WALK;
			break;
		case 'M':
			if ((uint8_t) (GetNpcBufferForIbo(npc)->typeFlags & NPC_SWIM))
				GetNpcBufferForIbo(npc)->typeFlags = GetNpcBufferForIbo(npc)->typeFlags & ~NPC_SWIM;
			else
				GetNpcBufferForIbo(npc)->typeFlags = GetNpcBufferForIbo(npc)->typeFlags | NPC_SWIM;
			break;
		case 'N':
			if ((uint8_t) (GetNpcBufferForIbo(npc)->typeFlags & NPC_ETHEREAL))
				GetNpcBufferForIbo(npc)->typeFlags = GetNpcBufferForIbo(npc)->typeFlags & ~NPC_ETHEREAL;
			else
				GetNpcBufferForIbo(npc)->typeFlags = GetNpcBufferForIbo(npc)->typeFlags | NPC_ETHEREAL;
			break;
		case 'O':
			if (CombatGroups.isProtectee((uint16_t)Item_getNpcNumber(npc))) {
				CombatGroups.leaders[GetAlignment(npc)] = -1;
				CombatGroups.releaseProtectors(GetAlignment(npc), -1);
			} else
				CombatGroups.setLeader((uint16_t)Item_getNpcNumber(npc));
			break;
		case 'S':
			if (!(uint16_t)Item_getNpcNumber(npc)) {
				if (Npc_isMale(npc)) {
					RemapShapeRecord(721, 989); /* Avatar */
					Npc_setFemale(npc);
				} else {
					RemapShapeRecord(721, 721); /* Avatar */
					Npc_setMale(npc);
				}
			}
			break;
		case 'P':
			if ((uint8_t) (GetNpcBufferForIbo(npc)->typeFlagsHigh & 0x40))
				GetNpcBufferForIbo(npc)->typeFlagsHigh = GetNpcBufferForIbo(npc)->typeFlagsHigh & ~0x40;
			else
				GetNpcBufferForIbo(npc)->typeFlagsHigh = GetNpcBufferForIbo(npc)->typeFlagsHigh | 0x40;
			break;
		case 'Q':
			if ((uint8_t) (GetNpcBufferForIbo(npc)->typeFlagsHigh & 0x80))
				GetNpcBufferForIbo(npc)->typeFlagsHigh = GetNpcBufferForIbo(npc)->typeFlagsHigh & ~0x80;
			else
				GetNpcBufferForIbo(npc)->typeFlagsHigh = GetNpcBufferForIbo(npc)->typeFlagsHigh | 0x80;
			break;
		case 'R':
			if ((uint8_t) (GetNpcBufferForIbo(npc)->typeFlagsHigh & 4))
				GetNpcBufferForIbo(npc)->typeFlagsHigh = GetNpcBufferForIbo(npc)->typeFlagsHigh & ~4;
			else
				GetNpcBufferForIbo(npc)->typeFlagsHigh = GetNpcBufferForIbo(npc)->typeFlagsHigh | 4;
			break;
		case 'X':
			return;
		}
	}
}

/* shows and changes an NPC's attributes */
void EditNpcStats(objref *npc)
{
	int8_t cmd;
	int16_t value;
	int8_t key;

	for (;;) {
		ClearLeftPanel();
		ClearRightPanel();
		PrintInLeftPanel(1, 0, "Npc #%3d", (uint16_t)Item_getNpcNumber(npc));
		PrintInRightPanel(1, 0, "%s%3d", GetGameText(5, 23), GetNpcBufferForIbo(npc)->dexterity);
		PrintInRightPanel(1, 1, "%s%3d", GetGameText(5, 24), GetNpcBufferForIbo(npc)->food);
		PrintInRightPanel(1, 2, "%s%3d", GetGameText(5, 25), GetNpcBufferForIbo(npc)->intelligence);
		PrintInRightPanel(1, 3, "%s%3d", GetGameText(5, 26), GetNpcBufferForIbo(npc)->strength);
		PrintInRightPanel(1, 4, "%s%3d", GetGameText(5, 27), GetNpcBufferForIbo(npc)->combat);
		PrintInRightPanel(1, 5, "%s%3d", GetGameText(5, 28), GetNpcBufferForIbo(npc)->mana);
		PrintInRightPanel(1, 6, "%s%3d", GetGameText(5, 29), Item_getHitPoints(npc));
		PrintInRightPanel(1, 8, GetGameText(5, 30));
		key = PromptForKey("Enter command.");
		switch (cmd = key >= 'a' && key <= 'z' ? key - 32 : key) {
		case 'F':
			value = PromptForIntegerWord(GetGameText(5, 31));
			GetNpcBufferForIbo(npc)->food = value;
			break;
		case 'C':
			value = PromptForIntegerWord(GetGameText(5, 32));
			GetNpcBufferForIbo(npc)->combat = value;
			break;
		case 'I':
			value = PromptForIntegerWord(GetGameText(5, 33));
			GetNpcBufferForIbo(npc)->intelligence = value;
			break;
		case 'S':
			value = PromptForIntegerWord(GetGameText(5, 34));
			GetNpcBufferForIbo(npc)->strength = value;
			break;
		case 'D':
			value = PromptForIntegerWord(GetGameText(5, 35));
			GetNpcBufferForIbo(npc)->dexterity = value;
			break;
		case 'M':
			value = PromptForIntegerWord(GetGameText(5, 36));
			GetNpcBufferForIbo(npc)->mana = value;
			break;
		case 'H':
			value = PromptForIntegerWord(GetGameText(5, 37));
			Item_setHitPoints(npc, value);
			break;
		case 'X':
			return;
		}
	}
}

/* the NPC editor's main menu */
void ModifyNpc(NPCRef *npc)
{
	int8_t cmd;
	int32_t value;
	int16_t number;
	objref item;
	int8_t key;
	char *word;

	for (;;) {
		ClearLeftPanel();
		ClearRightPanel();
		PrintInLeftPanel(1, 0, "Npc #%3d", (uint16_t)Item_getNpcNumber(npc));
		PrintInRightPanel(1, 0, GetGameText(5, 0));
		PrintInRightPanel(1, 1, GetGameText(5, 1));
		PrintInRightPanel(1, 2, GetGameText(5, 2));
		PrintInRightPanel(1, 3, GetGameText(5, 3));
		PrintInRightPanel(1, 4, GetGameText(5, 4));
		PrintInRightPanel(1, 5, GetGameText(5, 5));
		PrintInRightPanel(1, 6, GetGameText(5, 6));
		PrintInRightPanel(1, 7, "%s  %c", GetGameText(5, 7), YN(KillNpcMode));
		PrintInRightPanel(1, 8, GetGameText(5, 8));
		key = PromptForKey(GetGameText(5, 9));
		switch (cmd = key >= 'a' && key <= 'z' ? key - 32 : key) {
		case 'S':
			EditNpcStats(npc);
			break;
		case 'R':
			number = PromptForIntegerWord(GetGameText(5, 10));
			CombatGroups.movePoints[(uint16_t)Item_getNpcNumber(npc)] = number;
			break;
		case 'K':
			KillNpcMode = !KillNpcMode;
			break;
		case 'E':
			value = PromptForLong(GetGameText(5, 11), 0);
			GetNpcBufferForIbo(npc)->experience = value;
			break;
		case '~':
			number = PromptForIntegerWord(GetGameText(5, 12));
			GetNpcBufferForIbo(npc)->training = number;
			break;
		case 'P':
			item = objref(GetItemInSlot(*npc, 1));
			if (!item.valid()) {
				CreateItemInContainer(&item, 602, *npc);    /* two-handed sword */
				EquipItem(item, *npc, 20, 0);
				ConsoleShowMessage(GetGameText(5, 13));
			} else {
				Item_delete(&item);
				ConsoleShowMessage(GetGameText(5, 14));
			}
			break;
		case 'N':
			EditNpcFlags(npc);
			break;
		case 'T':
			EditNpcTargets(npc);
			break;
		case 'A':
			EditNpcAttackMode(npc);
			break;
		case 'F':
			PrintInLeftPanel(1, 0, GetGameText(5, 15));
			PrintInLeftPanel(1, 1, GetGameText(5, 16));
			PrintInLeftPanel(1, 2, GetGameText(5, 17));
			PrintInLeftPanel(1, 3, GetGameText(5, 18));
			PrintInLeftPanel(1, 10, GetGameText(5, 19));
			do
				cmd = PromptForKey(GetGameText(5, 20));
			while (cmd != 'x' && cmd != 'X' && cmd < '1' && cmd > '4');
			if (cmd >= '1' && cmd <= '4')
				GetNpcBufferForIbo(npc)->setAlignment((int16_t)cmd - '1');
			break;
		case 'B':
			PrintInLeftPanel(1, 0, "0 %10s 11 %10s 22 %10s", GetGameText(3, 0), GetGameText(3, 11),
				GetGameText(3, 22));
			PrintInLeftPanel(1, 1, "1 %10s 12 %10s 23 %10s", GetGameText(3, 1), GetGameText(3, 12),
				GetGameText(3, 23));
			PrintInLeftPanel(1, 2, "2 %10s 13 %10s 24 %10s", GetGameText(3, 2), GetGameText(3, 13),
				GetGameText(3, 24));
			PrintInLeftPanel(1, 3, "3 %10s 14 %10s 25 %10s", GetGameText(3, 3), GetGameText(3, 14),
				GetGameText(3, 25));
			PrintInLeftPanel(1, 4, "4 %10s 15 %10s 26 %10s", GetGameText(3, 4), GetGameText(3, 15),
				GetGameText(3, 26));
			PrintInLeftPanel(1, 5, "5 %10s 16 %10s 27 %10s", GetGameText(3, 5), GetGameText(3, 16),
				GetGameText(3, 27));
			PrintInLeftPanel(1, 6, "6 %10s 17 %10s 28 %10s", GetGameText(3, 6), GetGameText(3, 17),
				GetGameText(3, 28));
			PrintInLeftPanel(1, 7, "7 %10s 18 %10s 29 %10s", GetGameText(3, 7), GetGameText(3, 18),
				GetGameText(3, 29));
			PrintInLeftPanel(1, 8, "8 %10s 19 %10s 30 %10s", GetGameText(3, 8), GetGameText(3, 19),
				GetGameText(3, 30));
			PrintInLeftPanel(1, 9, "9 %10s 20 %10s 31 %10s", GetGameText(3, 9), GetGameText(3, 20),
				GetGameText(3, 31));
			PrintInLeftPanel(1, 10, "10%10s 21 %10s [X]it", GetGameText(3, 10), GetGameText(3, 21));
			do
				word = PromptForWord(GetGameText(5, 21));
			while (*word != 'x' && *word != 'X' && atoi(word) < 0 && atoi(word) > 31);
			if (atoi(word) >= 0 && atoi(word) <= 31) {
				Npc_setSchedule(npc, atoi(word));
				GetNpcBufferForIbo(npc)->schedules[GetNpcBufferForIbo(npc)->currentSchedule].state = 1;
			}
			break;
		case 'W':
			if ((uint8_t)(GetNpcBufferForIbo(npc)->typeFlagsHigh & 1))
				GetNpcBufferForIbo(npc)->typeFlagsHigh = GetNpcBufferForIbo(npc)->typeFlagsHigh & 0xfe;
			else
				GetNpcBufferForIbo(npc)->typeFlagsHigh = GetNpcBufferForIbo(npc)->typeFlagsHigh | 1;
			break;
		case 'X':
			return;
		}
	}
}

/* shows an NPC's schedule */
void DumpNpcActivity(objref *npc)
{
	int8_t cmd;
	int16_t i;
	int16_t current;

	current = GetNpcBufferForIbo(npc)->currentSchedule;
	ClearLeftPanel();
	ClearRightPanel();
	PrintInLeftPanel(1, 0, "%s%d", GetGameText(5, 105), (uint16_t)Item_getNpcNumber(npc));
	PrintInLeftPanel(1, 2, "  Busy=%c Disabled=%c", YN((uint8_t) (Item_getQualityFlags(npc) & QUALITY_BUSY)),
		YN(IsNpcUnconscious(npc)));
	if (HasStatus(npc, NPC_IN_PARTY))
		PrintInLeftPanel(21, 2, "Sit Ref=%u", PartySitRefs[GetPartyIndex(*npc)]);
	PrintInLeftPanel(1, 3, "  A=%u B=%u C=%u R=%u", GetNpcBufferForIbo(npc)->scheduleValue,
		GetNpcBufferForIbo(npc)->scheduleToggle, GetNpcBufferForIbo(npc)->scheduleC, GetNpcBufferForIbo(npc)->result);
	PrintInLeftPanel(1, 4, "Dx2=%d Dy2=%d", GetNpcBufferForIbo(npc)->sVr[0], GetNpcBufferForIbo(npc)->sVr[1]);
	for (i = 0; i <= current; i++) {
		GetNpcBufferForIbo(npc)->currentSchedule = i;
		PrintInLeftPanel(1, i + 6, "%c%s", GetNpcBufferForIbo(npc)->currentSchedule == current ? '>' : '+',
			GetGameText(3, GetNpcBufferForIbo(npc)->schedules[GetNpcBufferForIbo(npc)->currentSchedule].kind));
		PrintInLeftPanel(13, i + 6, "S=%u",
			GetNpcBufferForIbo(npc)->schedules[GetNpcBufferForIbo(npc)->currentSchedule].state);
		PrintInLeftPanel(19, i + 6, "X=%u",
			GetNpcBufferForIbo(npc)->schedules[GetNpcBufferForIbo(npc)->currentSchedule].x);
		PrintInLeftPanel(27, i + 6, "Y=%u",
			GetNpcBufferForIbo(npc)->schedules[GetNpcBufferForIbo(npc)->currentSchedule].y);
		PrintInLeftPanel(35, i + 6, "Z=%u",
			GetNpcBufferForIbo(npc)->schedules[GetNpcBufferForIbo(npc)->currentSchedule].counter);
	}
	GetNpcBufferForIbo(npc)->currentSchedule = current;
	PrintInRightPanel(1, 0, GetGameText(5, 106));
	do {
		int8_t key = PromptForKey(GetGameText(5, 107));

		cmd = key >= 'a' && key <= 'z' ? key - 32 : key;
	} while (cmd != 'X');
}

/* the debug menu */
void ShowDebugMenu(void)
{
	uint8_t workType;
	objref item;
	int16_t number;
	int16_t i;
	int8_t cmd;
	int8_t inRange;
	char buf[4];
	NPCRef who;
	objref target;
	uint8_t sx, sy, region;
	Coord x, y;
	int8_t key;
	int16_t day;

	HidePointer();
	for (;;) {
		ClearLeftPanel();
		ClearRightPanel();
		PrintInRightPanel(1, 0, "%s %3s", GetGameText(5, 114), HackMoverEnabled ? "Yes" : "No");
		if (DoScheduleNpc == -1)
			strcpy(buf, GetGameText(5, 115));
		else
			sprintf(buf, "%d", DoScheduleNpc);
		PrintInRightPanel(1, 1, "%s %3s%s %3s", GetGameText(5, 116), buf,
			GetGameText(5, 117), ShowAvatarLocation ? "On" : "Off");
		PrintInRightPanel(1, 2, "%s", GetGameText(5, 118));
		PrintInRightPanel(1, 3, "%s %3s%s", GetGameText(5, 119), ShowNpcNumbers ? "On" : "Off",
			GetGameText(5, 120));
		PrintInRightPanel(1, 4, "%s %3s", GetGameText(5, 121), UnkBugChecking ? "Yes" : "No");
		PrintInRightPanel(1, 5, "%s %3s%s %3s", GetGameText(5, 122), PowerAvatar ? "On" : "Off",
			GetGameText(5, 123), QueueToggle ? "On" : "Off");
		PrintInRightPanel(1, 6, "%s", GetGameText(5, 124));
		PrintInRightPanel(1, 7, "%s %3d%s", GetGameText(5, 125), GameTime.rate, GetGameText(5, 126));
		PrintInRightPanel(1, 8, "%s\t%3s%s", GetGameText(5, 127), FollowersEnabled ? "Yes" : "No",
			GetGameText(5, 128));
		PrintInLeftPanel(1, 0, "%s  %2d.%d %s", GetGameText(5, 129),
			GameTime.getHour() <= 12 ? GameTime.getHour() : GameTime.getHour() - 12,
			GameTime.getMinute(), GameTime.getHour() < 12 ? "AM" : "PM");
		PrintInLeftPanel(1, 1, GameVersion);
		key = PromptForKey(GetGameText(5, 130));
		switch (cmd = key >= 'a' && key <= 'z' ? key - 32 : key) {
		case 'F':
			for (i = 1; i <= 8; i++) {
				if (i != 6) {
					GetNpcIbo(&who, i);
					if (FollowersEnabled) {
						RemoveFromParty(who, 1);
					} else {
						AddToParty(who, 0);
					}
				}
			}
			FollowersEnabled = !FollowersEnabled;
			break;
		case 'A':
			number = PromptForIntegerWord(GetGameText(5, 131));
			GetNpcIbo(&who, number);
			if (who.valid())
				DumpNpcActivity(&who);
			else
				ConsoleShowMessage(GetGameText(5, 132));
			break;
		case 'H':
			HackMoverEnabled = !HackMoverEnabled;
			break;
		case 'U':
			UnkBugChecking = !UnkBugChecking;
			break;
		case '=':
			if ((int16_t)GameTime.rate < 25)
				GameTime.rate = GameTime.rate + 1;
			break;
		case '-':
			if ((int16_t)GameTime.rate > 1)
				GameTime.rate = GameTime.rate - 1;
			break;
		case 'C':
			number = PromptForIntegerWord(GetGameText(5, 133));
			if (number == -1) {
				ConsoleShowMessage(GetGameText(5, 134));
				break;
			}
			if (IsNpcType(number & 0x3ff)) {
				ConsoleShowMessage(GetGameText(5, 134));
				break;
			}
			for (i = 0; i < 10; i++) {
				if (CanWalkAt(Coord(Item_getX(AvatarRef).value + DirDeltaX[GetFacing(&AvatarRef)] * i),
					Coord(Item_getY(AvatarRef).value + DirDeltaY[GetFacing(&AvatarRef)]),
					Item_getZ(&AvatarRef), item.ptr()->typeFrame)) {
					if (CreateItem(&item, number,
						Coord(Item_getX(AvatarRef).value + DirDeltaX[GetFacing(&AvatarRef)] * i),
						Coord(Item_getY(AvatarRef).value + DirDeltaY[GetFacing(&AvatarRef)]),
						Item_getZ(&AvatarRef))) {
						Item_setOkayToTake(&item);
						Item_setTemporary(&item);
					} else
						ConsoleShowMessage(GetGameText(5, 135));
					break;
				}
			}
			if (!item.valid())
				break;
			if (i == 10) {
				ConsoleShowMessage(GetGameText(5, 136));
				break;
			}
			number = PromptForIntegerWord(GetGameText(5, 137));
			if (number == -1) {
				Item_delete(&item);
				ConsoleShowMessage(GetGameText(5, 138));
				break;
			}
			Item_setFrame(&item, number);
			if (Item_hasQuantity(&item)) {
				number = PromptForIntegerWord(GetGameText(5, 139));
				Item_storeQuantity(&item, number);
			}
			if (Item_hasQuality(&item)) {
				number = PromptForIntegerWord(GetGameText(5, 140));
				Item_setQuality(&item, number);
			}
			ConsoleShowMessage(GetGameText(5, 141));
			break;
		case 'S':
			PrintInLeftPanel(1, 0, "%s  %2d.%d %s", GetGameText(5, 142),
				GameTime.getHour() <= 12 ? GameTime.getHour() : GameTime.getHour() - 12,
				GameTime.getMinute(), GameTime.getHour() < 12 ? "AM" : "PM");
			number = PromptForIntegerWord(GetGameText(5, 143));
			day = PromptForIntegerWord(GetGameText(5, 144));
			if (number != -1 && day != -1) {
				GameTime.ticks = number * TICKS_PER_HOUR;
				GameTime.days = day;
				UpdateNpcSchedules();
			} else
				ConsoleShowMessage(GetGameText(5, 145));
			break;
		case 'P':
			PowerAvatar = !PowerAvatar;
			break;
		case 'I':
			number = PromptForIntegerWord(GetGameText(5, 146));
			GetNpcIbo(&who, number);
			if (who.valid()) {
				ClearLeftPanel();
				PrintInLeftPanel(1, 0, "%s%d", GetGameText(5, 147), (uint16_t)Item_getNpcNumber(&who));
				PrintInLeftPanel(1, 1, "Bsy=%c Ded=%c Cnj=%c Sum=%c Pth=%c Sld=%c IF=%c",
					YN((uint8_t)(Item_getQualityFlags(&who) & QUALITY_BUSY)),
					YN(HasStatus(&who, NPC_DEAD)),
					YN((uint8_t)(NPC(&who)->typeFlagsHigh & 0x40)),
					YN((uint8_t)(NPC(&who)->typeFlagsHigh & 0x80)),
					YN(HasPath(who)),
					YN((uint8_t)gItemTypeInfo[who.ptr()->typeFrame & 0x3ff].solid),
					YN(CanVisit(&who)));
				PrintInLeftPanel(1, 2, "HP=%d S=%d I=%d CS=%d D=%d MP=%d F=%d",
					Item_getHitPoints(&who),
					NPC(&who)->strength,
					NPC(&who)->intelligence,
					NPC(&who)->combat,
					NPC(&who)->dexterity,
					NPC(&who)->mana,
					NPC(&who)->food);
				PrintInLeftPanel(1, 3, "(x,y,z)=(%d,%d,%d) TP=%d Xp=%ld",
					(int16_t)Item_getX(who), (int16_t)Item_getY(who), Item_getZ(&who),
					NPC(&who)->training, NPC(&who)->experience);
				PrintInLeftPanel(1, 4, "Acty=%s FL=%c WA=%c SW=%c ET=%c",
					GetGameText(3, NPC(&who)->workType),
					YN((uint8_t)(NPC(&who)->typeFlags & NPC_FLY)),
					YN((uint8_t)(NPC(&who)->typeFlags & NPC_WALK)),
					YN((uint8_t)(NPC(&who)->typeFlags & NPC_SWIM)),
					YN((uint8_t)(NPC(&who)->typeFlags & NPC_ETHEREAL)));
				PrintInLeftPanel(1, 5, "As=%c Ch=%c Cu=%c Pa=%c Po=%c Pr=%c Fp=%d,%d,%d",
					YN(HasStatus(&who, NPC_ASLEEP)),
					YN(HasStatus(&who, NPC_CHARMED)),
					YN(HasStatus(&who, NPC_CURSED)),
					YN(HasStatus(&who, NPC_PARALYZED)),
					YN(HasStatus(&who, NPC_POISONED)),
					YN(HasStatus(&who, NPC_PROTECTED)),
					GetFootprintX(*(TypeFrame *)&who.ptr()->typeFrame) + 1,
					GetFootprintY(*(TypeFrame *)&who.ptr()->typeFrame) + 1,
					gItemTypeInfo[who.ptr()->typeFrame & 0x3ff].height);
				PrintInLeftPanel(1, 7, "AM=%s DAM=%s Aln=%s",
					NPC(&who)->workType == WORK_COMBAT ? GetGameText(5, Item_getQuality(&who) + 47) : "None     ",
					GetGameText(5, NPC(&who)->defaultAttackMode + 47),
					GetGameText(5, GetAlignment(&who) + 58));
				PrintInLeftPanel(1, 8, "PT=%d ST=%d Op=%d WP=%c IA=%c Py=%c Pd=%c",
					NPC(&who)->primaryTarget,
					NPC(&who)->secondaryTarget,
					NPC(&who)->oppressor,
					YN((uint8_t)(NPC(&who)->typeFlagsHigh & 1)),
					YN((uint8_t)(NPC(&who)->typeFlagsHigh & 0x20)),
					YN(HasStatus(&who, NPC_IN_PARTY)),
					YN(CombatGroups.isProtectee((uint16_t)Item_getNpcNumber(&who))));
				PrintInLeftPanel(1, 9, "I-Vr=(%d,%d) S-Vr=(%d,%d) D/R=%d",
					NPC(&who)->iVr[0], NPC(&who)->iVr[1],
					NPC(&who)->sVr[0], NPC(&who)->sVr[1],
					(uint8_t)(NPC(&who)->typeFlags & 7));
				GetNpcIbo(&target, GetCombatTarget(&who));
				if (target.valid()) {
					if ((uint16_t)(GetMissileDistance(who, target)) <= GetAttackRange(&who))
						inRange = 'Y';
					else
						inRange = 'N';
				} else
					inRange = '*';
				PrintInLeftPanel(1, 10, "CD=%d Rg=%d IR=%c LOF=%c RB=%c CW=%c",
					CombatGroups.movePoints[(uint16_t)Item_getNpcNumber(&who)],
					GetAttackRange(&who),
					inRange,
					target.valid() ? YN(HasLineOfFire(who, target)) : '*',
					YN(CombatGroups.partyAttacked),
					YN(CombatGroups.battleMusic | AvatarInCombat));
				ConsoleShowMessage("");
			} else
				ConsoleShowMessage(GetGameText(5, 148));
			break;
		case 'T':
			ShowTeleportMenu();
			break;
		case 'X':
			ShowPointer();
			return;
		case 'N':
			ShowNpcNumbers = !ShowNpcNumbers;
			break;
		case 'L':
			ShowAvatarLocation = !ShowAvatarLocation;
			break;
		case 'G':
			PrintInLeftPanel(1, 0, GetGameText(5, 149));
			PrintInLeftPanel(1, 1, GetGameText(5, 150));
			PrintInLeftPanel(1, 10, GetGameText(5, 151));
			for (;;) {
				cmd = PromptForKey(GetGameText(5, 152));
				if (cmd == 'I')
					GameFlags.show();
				else if (cmd == 'S')
					GameFlags.edit();
				else if (cmd == 'x' || cmd == 'X')
					return;
				else
					ConsoleShowMessage(GetGameText(5, 153));
			}
		case 'B':
			number = PromptForIntegerWord(GetGameText(5, 154));
			if (number >= 256) {
				ConsoleShowMessage(GetGameText(5, 155));
				break;
			}
			GetNpcIbo(&who, number);
			if (who.valid()) {
				for (i = 0; i < 8; i++) {
					workType = Schedule_getEntryWorkType(&ScheduleTable, number, i);
					if (workType != 255) {
						sx = Schedule_getEntryX(&ScheduleTable, (uint16_t)Item_getNpcNumber(&who), i);
						sy = Schedule_getEntryY(&ScheduleTable, (uint16_t)Item_getNpcNumber(&who), i);
						region = Schedule_getEntryRegion(&ScheduleTable, (uint16_t)Item_getNpcNumber(&who), i);
						x = RegionX[region] + sx;
						y = RegionY[region] + sy;
					}
					if (workType == 255)
						PrintInLeftPanel(1, i + 1, "%2d %s:",
							i * 3 - (i > 0 ? 12 : 0) - (i > 4 ? 12 : 0) + 12, i < 4 ? "AM" : "PM");
					else
						PrintInLeftPanel(1, i + 1, "%2d %s:  (0x%3x,0x%3x) - %s",
							i * 3 - (i > 0 ? 12 : 0) - (i > 4 ? 12 : 0) + 12, i < 4 ? "AM" : "PM",
							(int16_t)x, (int16_t)y, GetGameText(3, workType));
				}
				PrintInLeftPanel(1, 10, "%s  %s", GetGameText(5, 156), GetGameText(3, NPC(&who)->workType));
				ConsoleShowMessage("");
			} else
				ConsoleShowMessage(GetGameText(5, 157));
			break;
		case 'D':
			DoScheduleNpc = number = PromptForIntegerWord(GetGameText(5, 158));
			break;
		case 'M':
			number = PromptForIntegerWord(GetGameText(5, 159));
			GetNpcIbo(&who, number);
			if (who.valid())
				ModifyNpc(&who);
			else
				ConsoleShowMessage(GetGameText(5, 160));
			break;
		case 'Q':
			QueueToggle = !QueueToggle;
			break;
		case 'E':
			ConsoleBlankLine(1, 25 - StatusLineRaised);
			StatusLineRaised = !StatusLineRaised;
			break;
		}
	}
}

/* creates a readable item and shows its text for a range of qualities */
void ShowReadableTexts(void)
{
	int8_t key;
	int16_t first;
	objref thing;
	int16_t quality;
	int16_t last;

	switch (key = PromptForKey(GetGameText(5, 109))) {
	case 't':
	case 'T':
		CreateItem(&thing, TypeFrame(715));    /* tombstone */
		break;
	case 'p':
	case 'P':
		CreateItem(&thing, TypeFrame(820));    /* plaque */
		break;
	case 'b':
	case 'B':
		CreateItem(&thing, TypeFrame(642));    /* book */
		break;
	case 'c':
	case 'C':
		CreateItem(&thing, TypeFrame(797));    /* scroll */
		break;
	case 's':
	case 'S':
		break;
	default:
		return;
	}
	first = PromptForIntegerWord(GetGameText(5, 110));
	last = PromptForIntegerWord(GetGameText(5, 111));
	for (quality = first; quality < last + 1; quality++) {
		ConsolePrintAt(1, 1, "%s%d", GetGameText(5, 109), quality);
		Item_setQuality(&thing, quality);
		RunUsable(1, thing, -1);
	}
	ZapDetachedItem(&thing);
}

/* plays a sound effect by number */
void CheatPlaySound(void)
{
	int16_t number = PromptForIntegerWord("Sound # ");

	PlaySoundAtItem(number, AvatarRef);
}

/* moves everything inside a picked container out to the container's spot */
void EmptyPickedContainer(void)
{
	AreaSearch found;
	int16_t z;
	objref container;
	objref thing;
	Coord x, y;

	HavePlayerSelect(&container, &x, &y, &z);
	if (container.valid()
		&& (uint8_t) (ItemTypeClassFlags[gItemTypeInfo[container.type()].typeClass] & CLASS_CONTENTS)) {
		FindItemInContainer(&found, container, 0x100, -1, 255, 255);
		while (found.found()) {
			thing = found.current;
			FindItem(&found);
			Item_move(&thing, x, y, z);
		}
	}
}
