/* Serpent Isle SI.EXE, overlay segment 214 (file offsets 0x054b60 to 0x059f17, 21431 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
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
#include "u7sound.h"
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
#include "schedit.h"
#include "legalmov.h"
#include "mapview.h"
#include "palctrl.h"
#include "cheat.h"
#include "flags.h"
#include "item.h"

inline unsigned char IsNpcType(TypeFrame typeFrame)
{
	return (ItemTypeClassFlags[gItemTypeInfo[typeFrame.type()].typeClass] & CLASS_NPC) != 0;
}

extern unsigned char KillNpcMode;
extern char AvatarInCombat;
extern unsigned char far Item_getQuality(objref *ref);
extern Coord far Item_getX(objref &);
extern Coord far Item_getY(objref &);
extern unsigned char far Item_getQualityFlags(objref *);
extern void far Item_setQualityFlags(objref *, unsigned char);
extern void far Item_setInvisible(objref *);
extern char far Item_getHitPoints(objref *ref);
extern void far Item_setHitPoints(objref *, unsigned char);
extern unsigned char far CreateItemInContainer(objref *ref, TypeFrame type, objref container);
extern char far Item_delete(objref *ref);
extern unsigned char far CreateItem(objref *, TypeFrame);
extern void far Item_setQuality(objref *, char);
extern unsigned char far ZapDetachedItem(objref *ref);
extern unsigned char far Item_move(objref *ref, Loc x, Loc y, int z);
extern unsigned char far CreateItem(objref *, TypeFrame, CellCoord, CellCoord, int);
extern void far Item_setOkayToTake(objref *);
extern void far Item_setTemporary(objref *);
extern void far Item_setFrame(objref *, int);
extern unsigned char far Item_hasQuantity(objref *ref);
extern void far Item_storeQuantity(objref *, unsigned char);
extern unsigned char far Item_hasQuality(objref *ref);

/* Not yet in a header. */
int far GetClothingWarmth(objref npc);
void far SetPolymorph(objref npc, unsigned shape);
void far DismissRegionalGuards(void);
void far GetRegionPosition(int *x, int *y, int *map);
void far RevertSchedule(int npc);
extern int LowFn;
extern int HighFn;

#define NPC(ref) GetNpcBufferForIbo(ref)

/* back to mode 13h without clearing the screen, and the game's palette */
inline void RestoreGameScreen(void)
{
	_AL = 0x93;
	_AH = 0;
	geninterrupt(0x10);
	SetFixedPalette(0);
}
#define YN(c) ((c) ? 'Y' : 'N')

char CheatsEnabled;
unsigned char FrameRateShown;
char PromptWordBuffer[50];
int DoScheduleNpc = -1;
unsigned char ShowNpcNumbers = 0;
unsigned char ShowAvatarLocation = 0;
unsigned char PowerAvatar = 0;
unsigned char QueueToggle = 0;
unsigned char UnkBugChecking = 1;
unsigned char StatusLineRaised = 1;
unsigned char FollowersEnabled = 0;

/* prints at column x, row y */
void ConsolePrintAt(int x, int y, char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
	}
	directvideo = 0;
	gotoxy(x, y);
	cprintf(WorkString);
}

/* prints at column x, row y, then waits for a key */
void ConsolePrintAndWait(int x, int y, char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
	}
	directvideo = 0;
	gotoxy(x, y);
	printf(WorkString);
	while (kbhit())
		getch();
	while (!kbhit())
		;
	getch();
}

/* blanks 40 columns */
void ConsoleBlankLine(int x, int y)
{
	char line[56];

	strcpy(line, "");
	while (strlen(line) < 50)
		strcat(line, " ");
	ConsolePrintAt(x, y, GetGameText(3, 95), line);
}

/* shows a message on the status line and waits for a key */
void ConsoleShowMessage(char *msg)
{
	char line[50];

	strcpy(line, msg);
	if (line[0])
		strcat(line, "  ");
	strcat(line, GetGameText(3, 89));
	while (strlen(line) < 40)
		strcat(line, " ");
	ConsolePrintAt(1, 25 - StatusLineRaised, GetGameText(3, 95), line);
	ConsolePrintAndWait(1, 23, "");
}

/* prompts on the status line and reads a word */
char *PromptForWord(char *prompt)
{
	char line[50];

	strncpy(line, prompt, 50);
	while (strlen(line) < 40)
		strcat(line, " ");
	ConsolePrintAt(1, 25 - StatusLineRaised, GetGameText(3, 95), line);
	ConsolePrintAt(1, 23, GetGameText(3, 96));
	ConsolePrintAt(9, 23, "");
	fflush(stdin);
	scanf(GetGameText(3, 95), PromptWordBuffer);
	return PromptWordBuffer;
}

/* prompts on the status line and reads a number, in hex when asked */
long PromptForLong(char *prompt, char hex)
{
	long value;
	char line[50];

	strncpy(line, prompt, 50);
	while (strlen(line) < 40)
		strcat(line, " ");
	ConsolePrintAt(1, 25 - StatusLineRaised, GetGameText(3, 95), line);
	ConsolePrintAt(1, 23, GetGameText(3, 96));
	ConsolePrintAt(9, 23, "");
	fflush(stdin);
	if (hex)
		scanf(GetGameText(3, 97), &value);
	else
		scanf(GetGameText(3, 98), &value);
	return value;
}

int PromptForIntegerWord(char *prompt)
{
	int value = PromptForLong(prompt, 0);

	return value;
}

/* prompts on the status line and reads one key */
char PromptForKey(char *prompt)
{
	char key;
	char line[50];

	strncpy(line, prompt, 50);
	while (strlen(line) < 40)
		strcat(line, " ");
	ConsolePrintAt(1, 25 - StatusLineRaised, GetGameText(3, 95), line);
	ConsolePrintAt(1, 23, GetGameText(3, 96));
	ConsolePrintAt(9, 23, "");
	fflush(stdin);
	key = getche();
	return key;
}

/* clears the left panel */
void ClearLeftPanel(void)
{
	int y;

	for (y = 1; y <= 11; y++)
		ConsoleBlankLine(1, y);
}

/* clears the right panel */
void ClearRightPanel(void)
{
	int y;

	for (y = 13; y < 23; y++)
		ConsoleBlankLine(1, y);
}

/* prints in the right panel */
void PrintInRightPanel(int x, int y, char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
	}
	ConsolePrintAt(x, y + 13, WorkString);
}

/* prints in the left panel */
void PrintInLeftPanel(int x, int y, char *fmt, ...)
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
	char cmd;
	char key;
	int number;

	for (;;) {
		ClearLeftPanel();
		ClearRightPanel();
		PrintInLeftPanel(1, 0, GetGameText(3, 99), (unsigned)Item_getNpcNumber(npc));
		PrintInRightPanel(1, 0, GetGameText(3, 100), GetGameText(5, 39), GetNpcBufferForIbo(npc)->primaryTarget);
		PrintInRightPanel(1, 1, GetGameText(3, 100), GetGameText(5, 40), GetNpcBufferForIbo(npc)->secondaryTarget);
		PrintInRightPanel(1, 2, GetGameText(3, 100), GetGameText(5, 41), GetNpcBufferForIbo(npc)->oppressor);
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
					GetNpcBufferForIbo(npc)->oppressor = (unsigned)Item_getNpcNumber(&who);
				else if (cmd == 'P')
					Npc_setTarget(npc, (unsigned)Item_getNpcNumber(&who), 1);
				else
					Npc_setTarget(npc, (unsigned)Item_getNpcNumber(&who), 0);
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
	char cmd, n, key;
	int i;

	ClearLeftPanel();
	ClearRightPanel();
	for (i = 0; i < 9; i++)
		PrintInLeftPanel(1, i, GetGameText(3, 101), i, GetGameText(5, i + 47));
	if ((char)Item_isAvatar(npc))
		PrintInLeftPanel(1, 9, GetGameText(3, 102), GetGameText(5, 56));
	PrintInLeftPanel(1, 10, GetGameText(3, 103));
	PrintInRightPanel(1, 8, GetGameText(3, 104));
	do {
		PrintInRightPanel(1, 0, GetGameText(3, 105), GetGameText(5, 63),
			GetNpcBufferForIbo(npc)->workType == WORK_COMBAT ? GetGameText(5, Item_getQuality(npc) + 47) : "None");
		PrintInRightPanel(1, 1, GetGameText(3, 105), GetGameText(5, 64),
			GetGameText(5, GetNpcBufferForIbo(npc)->defaultAttackMode + 47));
		key = PromptForKey(GetGameText(5, 179));
		if (key >= 'a' && key <= 'z')
			cmd = key - 32;
		else
			cmd = key;
		if (cmd == 'C' || cmd == 'D') {
			do {
				n = PromptForKey(GetGameText(5, 180));
				if (n == 'x' || n == 'X')
					break;
			} while (n < '0' || n > '9');
			if (n >= '0' && n <= '9') {
				if (cmd == 'C')
					SetAttackMode(*npc, n - '0');
				else
					GetNpcBufferForIbo(npc)->defaultAttackMode = (int) n - '0';
			}
		} else if (cmd != 'X')
			ConsoleShowMessage(GetGameText(5, 181));
	} while (cmd != 'X');
}

/* moves the avatar to typed coordinates, to an NPC or to an item */
void ShowTeleportMenu(void)
{
	char cmd;
	int x, y;
	int number;
	long value;
	objref who;
	objref item;
	char key;

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
				TeleportParty(x, y, 0);
				break;
			case 'H':
				x = PromptForLong(GetGameText(5, 73), 1);
				y = PromptForLong(GetGameText(5, 74), 1);
				TeleportParty(x, y, 0);
				break;
			case 'N':
				number = PromptForIntegerWord(GetGameText(5, 76));
				GetNpcIbo(&who, number);
				if (who.valid())
					TeleportParty(Item_getX(who), Item_getY(who), (int)Item_getZ(&who));
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
				TeleportParty(Item_getX(item), Item_getY(item), (int)Item_getZ(&item));
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
	char cmd;
	char key;
	int number;

	for (;;) {
		ClearLeftPanel();
		ClearRightPanel();
		PrintInLeftPanel(1, 0, GetGameText(3, 100), GetGameText(5, 82), Item_getNpcNumber(npc));
		PrintInRightPanel(1, 0, GetGameText(3, 106), GetGameText(5, 83), YN(HasStatus(npc, NPC_ASLEEP)));
		PrintInRightPanel(1, 1, GetGameText(3, 106), GetGameText(5, 84), YN(HasStatus(npc, NPC_CHARMED)));
		PrintInRightPanel(1, 2, GetGameText(3, 106), GetGameText(5, 85), YN(HasStatus(npc, NPC_CURSED)));
		PrintInRightPanel(1, 3, GetGameText(3, 106), GetGameText(5, 86), YN(HasStatus(npc, NPC_PARALYZED)));
		PrintInRightPanel(1, 4, GetGameText(3, 106), GetGameText(5, 87), YN(HasStatus(npc, NPC_POISONED)));
		PrintInRightPanel(1, 5, GetGameText(3, 106), GetGameText(5, 88), YN(HasStatus(npc, NPC_PROTECTED)));
		PrintInRightPanel(1, 6, GetGameText(3, 106), GetGameText(5, 89), YN(HasStatus(npc, NPC_DEAD)));
		PrintInRightPanel(1, 7, GetGameText(3, 106), GetGameText(5, 90),
			YN((unsigned char) (GetNpcBufferForIbo(npc)->typeFlagsHigh & 0x20)));
		PrintInRightPanel(14, 0, GetGameText(3, 106), GetGameText(5, 91), YN(HasStatus(npc, NPC_IN_PARTY)));
		PrintInRightPanel(14, 1, GetGameText(3, 106), GetGameText(5, 92),
			YN((unsigned char) (Item_getQualityFlags(npc) & QUALITY_INVISIBLE)));
		PrintInRightPanel(14, 2, GetGameText(3, 106), GetGameText(5, 93),
			YN((unsigned char) (GetNpcBufferForIbo(npc)->typeFlags & NPC_FLY)));
		PrintInRightPanel(14, 3, GetGameText(3, 106), GetGameText(5, 94),
			YN((unsigned char) (GetNpcBufferForIbo(npc)->typeFlags & NPC_WALK)));
		PrintInRightPanel(14, 4, GetGameText(3, 106), GetGameText(5, 95),
			YN((unsigned char) (GetNpcBufferForIbo(npc)->typeFlags & NPC_SWIM)));
		PrintInRightPanel(14, 5, GetGameText(3, 106), GetGameText(5, 96),
			YN((unsigned char) (GetNpcBufferForIbo(npc)->typeFlags & NPC_ETHEREAL)));
		PrintInRightPanel(14, 6, GetGameText(3, 106), GetGameText(5, 97),
			YN(CombatGroups.isProtectee(Item_getNpcNumber(npc))));
		PrintInRightPanel(14, 7, GetGameText(3, 106), GetGameText(5, 98),
			YN((unsigned char) (GetNpcBufferForIbo(npc)->typeFlagsHigh & 0x40)));
		PrintInRightPanel(27, 0, GetGameText(3, 106), GetGameText(5, 99),
			YN((unsigned char) (GetNpcBufferForIbo(npc)->typeFlagsHigh & 0x80)));
		PrintInRightPanel(27, 1, GetGameText(3, 106), GetGameText(5, 100),
			YN((unsigned char) (GetNpcBufferForIbo(npc)->typeFlagsHigh & 4)));
		if (!Item_getNpcNumber(npc))
			PrintInRightPanel(27, 2, GetGameText(3, 106), GetGameText(5, 101), Npc_isMale(npc) ? 'M' : 'F');
		else
			PrintInRightPanel(27, 2, GetGameText(3, 106), GetGameText(5, 102), YN(Npc_hasMetFlag(npc)));
		if (!(unsigned char)Item_isAvatar(npc))
			PrintInRightPanel(27, 3, GetGameText(3, 106), GetGameText(5, 103), YN(Npc_hasNoCastFlag(npc)));
		else {
			char skin[4];

			switch (Npc_getSkinColor(npc)) {
			case 0:
				strcpy(skin, GetGameText(3, 108));
				break;
			case 1:
				strcpy(skin, GetGameText(3, 109));
				break;
			case 2:
				strcpy(skin, GetGameText(3, 110));
				break;
			case 3:
				strcpy(skin, GetGameText(3, 111));
				break;
			}
			skin[3] = 0;
			PrintInRightPanel(27, 3, GetGameText(3, 107), GetGameText(5, 104), skin);
		}
		if (!(unsigned char)Item_isAvatar(npc))
			PrintInRightPanel(27, 4, GetGameText(3, 112), GetGameText(5, 105), Npc_getMagic(npc));
		else
			PrintInRightPanel(27, 4, GetGameText(3, 106), GetGameText(5, 106), YN(Npc_hasReadFlag(npc)));
		if (!(unsigned char)Item_isAvatar(npc))
			PrintInRightPanel(27, 5, GetGameText(3, 106), GetGameText(5, 107), YN(Npc_hasZombieFlag(npc)));
		else
			PrintInRightPanel(27, 5, GetGameText(3, 106), GetGameText(5, 108), YN(Npc_hasFreezeFlag(npc)));
		if (HasStatus(npc, NPC_IN_PARTY) && npc->type() != 747 && npc->type() != 658) {
			PrintInRightPanel(27, 6, GetGameText(3, 112), GetGameText(5, 109), Npc_getTemperature(npc));
			PrintInRightPanel(27, 7, GetGameText(3, 113), GetGameText(5, 110), GetClothingWarmth(*npc));
		}
		PrintInRightPanel(27, 8, GetGameText(3, 106), GetGameText(5, 111), YN(Npc_hasPolymorphFlag(npc)));
		PrintInRightPanel(14, 8, GetGameText(3, 106), GetGameText(5, 112), YN(Npc_hasTournamentFlag(npc)));
		if ((char)Item_isAvatar(npc)) {
			PrintInRightPanel(27, 9, GetGameText(3, 106), GetGameText(5, 113), YN(Npc_hasPetraFlag(npc)));
			PrintInRightPanel(14, 9, GetGameText(3, 106), GetGameText(5, 120), YN(FindShapeRecord(721) > 1029));
		}
		PrintInRightPanel(1, 8, GetGameText(5, 116));
		key = PromptForKey(GetGameText(5, 117));
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
			if ((unsigned char) (GetNpcBufferForIbo(npc)->typeFlagsHigh & 0x20))
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
			if ((unsigned char) (Item_getQualityFlags(npc) & QUALITY_INVISIBLE))
				Item_setQualityFlags(npc, Item_getQualityFlags(npc) & ~1);
			else
				Item_setInvisible(npc);
			break;
		case 'K':
			if ((unsigned char) (GetNpcBufferForIbo(npc)->typeFlags & NPC_FLY))
				GetNpcBufferForIbo(npc)->typeFlags = GetNpcBufferForIbo(npc)->typeFlags & ~NPC_FLY;
			else
				GetNpcBufferForIbo(npc)->typeFlags = GetNpcBufferForIbo(npc)->typeFlags | NPC_FLY;
			break;
		case 'L':
			if ((unsigned char) (GetNpcBufferForIbo(npc)->typeFlags & NPC_WALK))
				GetNpcBufferForIbo(npc)->typeFlags = GetNpcBufferForIbo(npc)->typeFlags & ~NPC_WALK;
			else
				GetNpcBufferForIbo(npc)->typeFlags = GetNpcBufferForIbo(npc)->typeFlags | NPC_WALK;
			break;
		case 'M':
			if ((unsigned char) (GetNpcBufferForIbo(npc)->typeFlags & NPC_SWIM))
				GetNpcBufferForIbo(npc)->typeFlags = GetNpcBufferForIbo(npc)->typeFlags & ~NPC_SWIM;
			else
				GetNpcBufferForIbo(npc)->typeFlags = GetNpcBufferForIbo(npc)->typeFlags | NPC_SWIM;
			break;
		case 'N':
			if ((unsigned char) (GetNpcBufferForIbo(npc)->typeFlags & NPC_ETHEREAL))
				GetNpcBufferForIbo(npc)->typeFlags = GetNpcBufferForIbo(npc)->typeFlags & ~NPC_ETHEREAL;
			else
				GetNpcBufferForIbo(npc)->typeFlags = GetNpcBufferForIbo(npc)->typeFlags | NPC_ETHEREAL;
			break;
		case 'O':
			if (CombatGroups.isProtectee(Item_getNpcNumber(npc))) {
				CombatGroups.leaders[GetAlignment(npc)] = -1;
				CombatGroups.releaseProtectors(GetAlignment(npc), -1);
			} else
				CombatGroups.setLeader(Item_getNpcNumber(npc));
			break;
		case 'S':
			if (!Item_getNpcNumber(npc)) {
				if (Npc_isMale(npc)) {
					Npc_setFemale(npc);
					FindAvatarNpc();
				} else {
					Npc_setMale(npc);
					FindAvatarNpc();
				}
			}
			break;
		case 'P':
			if ((unsigned char) (GetNpcBufferForIbo(npc)->typeFlagsHigh & 0x40))
				GetNpcBufferForIbo(npc)->typeFlagsHigh = GetNpcBufferForIbo(npc)->typeFlagsHigh & ~0x40;
			else
				GetNpcBufferForIbo(npc)->typeFlagsHigh = GetNpcBufferForIbo(npc)->typeFlagsHigh | 0x40;
			break;
		case 'Q':
			if ((unsigned char) (GetNpcBufferForIbo(npc)->typeFlagsHigh & 0x80))
				GetNpcBufferForIbo(npc)->typeFlagsHigh = GetNpcBufferForIbo(npc)->typeFlagsHigh & ~0x80;
			else
				GetNpcBufferForIbo(npc)->typeFlagsHigh = GetNpcBufferForIbo(npc)->typeFlagsHigh | 0x80;
			break;
		case 'R':
			if ((unsigned char) (GetNpcBufferForIbo(npc)->typeFlagsHigh & 4))
				GetNpcBufferForIbo(npc)->typeFlagsHigh = GetNpcBufferForIbo(npc)->typeFlagsHigh & ~4;
			else
				GetNpcBufferForIbo(npc)->typeFlagsHigh = GetNpcBufferForIbo(npc)->typeFlagsHigh | 4;
			break;
		case 'T':
			if (!(unsigned char)Item_isAvatar(npc)) {
				if (Npc_hasMetFlag(npc))
					Npc_clearMetFlag(npc);
				else
					Npc_setMetFlag(npc);
			}
			break;
		case 'U':
			if (!(unsigned char)Item_isAvatar(npc)) {
				if (Npc_hasNoCastFlag(npc))
					Npc_clearNoCastFlag(npc);
				else
					Npc_setNoCastFlag(npc);
			}
			break;
		case 'W':
			if ((char)Item_isAvatar(npc)) {
				if (Npc_hasFreezeFlag(npc))
					Npc_clearFreezeFlag(npc);
				else
					Npc_setFreezeFlag(npc);
			}
			break;
		case 'V':
			if (!(unsigned char)Item_isAvatar(npc)) {
				cmd = PromptForIntegerWord(GetGameText(5, 114));
				if (cmd >= 0 && cmd < 32)
					Npc_setMagic(npc, cmd);
			}
			break;
		case 'Y':
			if (HasStatus(npc, NPC_IN_PARTY) && npc->type() != 658 && npc->type() != 747) {
				cmd = PromptForIntegerWord(GetGameText(5, 115));
				if (cmd >= 0 && cmd < 64)
					Npc_setTemperature(npc, cmd);
			}
			break;
		case '1':
			if ((char)Item_isAvatar(npc)) {
				cmd = PromptForIntegerWord(GetGameText(5, 118));
				if (cmd >= 0 && cmd < 4) {
					Npc_setSkinColor(npc, cmd);
					FindAvatarNpc();
				}
			}
			break;
		case '2':
			if (Npc_hasPolymorphFlag(npc))
				SetPolymorph(*npc, npc->type());
			else {
				number = PromptForIntegerWord(GetGameText(5, 119));
				if (number >= 0 && number < 1044)
					SetPolymorph(*npc, number);
			}
			break;
		case '3':
			if (Npc_hasTournamentFlag(npc))
				Npc_clearTournamentFlag(npc);
			else
				Npc_setTournamentFlag(npc);
			break;
		case '4':
			if ((char)Item_isAvatar(npc)) {
				if (Npc_hasReadFlag(npc))
					Npc_clearReadFlag(npc);
				else
					Npc_setReadFlag(npc);
			}
			break;
		case '5':
			if ((char)Item_isAvatar(npc)) {
				DismissRegionalGuards();
				if (Npc_hasPetraFlag(npc))
					Npc_clearPetraFlag(npc);
				else {
					Npc_setPetraFlag(npc);
					Npc_setFemale(npc);
				}
			}
			break;
		case '6':
			int type;

			type = PromptForLong(GetGameText(3, 114), 0);
			number = FindShapeRecord(type);
			DebugPrintfWait(GetGameText(3, 115), number);
			break;
		case '7':
			if ((char)Item_isAvatar(npc)) {
				if (FindShapeRecord(721) > 1029)
					FindAvatarNpc();
				else {
					int shape = (2 - Npc_getSkinColor(npc)) * 2 + 1030;

					if (!Npc_isMale(npc))
						shape++;
					RemapShapeRecord(721, shape);
				}
			}
			break;
		case 'Z':
			if (!(unsigned char)Item_isAvatar(npc)) {
				if (Npc_hasZombieFlag(npc))
					Npc_clearZombieFlag(npc);
				else
					Npc_setZombieFlag(npc);
			}
			break;
		case 'X':
			return;
		}
	}
}

/* shows and changes an NPC's attributes */
void EditNpcStats(objref *npc)
{
	char cmd;
	int value;
	char key;

	for (;;) {
		ClearLeftPanel();
		ClearRightPanel();
		PrintInLeftPanel(1, 0, GetGameText(3, 116), Item_getNpcNumber(npc));
		PrintInRightPanel(1, 0, GetGameText(3, 100), GetGameText(5, 23), GetNpcBufferForIbo(npc)->dexterity);
		PrintInRightPanel(1, 1, GetGameText(3, 100), GetGameText(5, 24), GetNpcBufferForIbo(npc)->food);
		PrintInRightPanel(1, 2, GetGameText(3, 100), GetGameText(5, 25),
			(unsigned char)(GetNpcBufferForIbo(npc)->intelligence & 0x1f));
		PrintInRightPanel(1, 3, GetGameText(3, 100), GetGameText(5, 26),
			(unsigned char)(GetNpcBufferForIbo(npc)->strength & 0x1f));
		PrintInRightPanel(1, 4, GetGameText(3, 100), GetGameText(5, 27),
			(unsigned char)(GetNpcBufferForIbo(npc)->combat & 0x1f));
		if ((char)Item_isAvatar(npc))
			PrintInRightPanel(1, 5, GetGameText(3, 100), GetGameText(5, 28),
				(unsigned char)(GetNpcBufferForIbo(npc)->mana & 0x1f));
		PrintInRightPanel(1, 6, GetGameText(3, 100), GetGameText(5, 29), Item_getHitPoints(npc));
		PrintInRightPanel(1, 8, GetGameText(5, 30));
		key = PromptForKey(GetGameText(5, 179));
		switch (cmd = key >= 'a' && key <= 'z' ? key - 32 : key) {
		case 'F':
			value = PromptForIntegerWord(GetGameText(5, 31));
			GetNpcBufferForIbo(npc)->food = value;
			break;
		case 'C':
			value = PromptForIntegerWord(GetGameText(5, 32));
			GetNpcBufferForIbo(npc)->combat = (unsigned char)value | GetNpcBufferForIbo(npc)->combat & 0xe0;
			break;
		case 'I':
			value = PromptForIntegerWord(GetGameText(5, 33));
			GetNpcBufferForIbo(npc)->intelligence =
				(unsigned char)value | GetNpcBufferForIbo(npc)->intelligence & 0xe0;
			break;
		case 'S':
			value = PromptForIntegerWord(GetGameText(5, 34));
			GetNpcBufferForIbo(npc)->strength = GetNpcBufferForIbo(npc)->strength & 0xe0 | (unsigned char)value;
			break;
		case 'D':
			value = PromptForIntegerWord(GetGameText(5, 35));
			GetNpcBufferForIbo(npc)->dexterity = value;
			break;
		case 'M':
			if ((char)Item_isAvatar(npc)) {
				value = PromptForIntegerWord(GetGameText(5, 36));
				GetNpcBufferForIbo(npc)->mana = (unsigned char)value | GetNpcBufferForIbo(npc)->mana & 0xe0;
			}
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
	char cmd;
	long value;
	int number;
	objref item;
	char key;
	char *word;

	for (;;) {
		ClearLeftPanel();
		ClearRightPanel();
		PrintInLeftPanel(1, 0, GetGameText(3, 116), Item_getNpcNumber(npc));
		PrintInRightPanel(1, 0, GetGameText(5, 0));
		PrintInRightPanel(1, 1, GetGameText(5, 1));
		PrintInRightPanel(1, 2, GetGameText(5, 2));
		PrintInRightPanel(1, 3, GetGameText(5, 3));
		PrintInRightPanel(1, 4, GetGameText(5, 4));
		PrintInRightPanel(1, 5, GetGameText(5, 5));
		PrintInRightPanel(1, 6, GetGameText(5, 6));
		PrintInRightPanel(1, 7, GetGameText(3, 117), GetGameText(5, 7), YN(KillNpcMode));
		PrintInRightPanel(1, 8, GetGameText(5, 8));
		key = PromptForKey(GetGameText(5, 9));
		switch (cmd = key >= 'a' && key <= 'z' ? key - 32 : key) {
		case 'S':
			EditNpcStats(npc);
			break;
		case 'R':
			number = PromptForIntegerWord(GetGameText(5, 10));
			CombatGroups.movePoints[(unsigned)Item_getNpcNumber(npc)] = number;
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
				GetNpcBufferForIbo(npc)->setAlignment((int)cmd - '1');
			break;
		case 'B':
			PrintInLeftPanel(1, 0, GetGameText(3, 160), GetGameText(3, 11), GetGameText(3, 22));
			PrintInLeftPanel(1, 1, GetGameText(3, 161), GetGameText(3, 12), GetGameText(3, 23));
			PrintInLeftPanel(1, 2, GetGameText(3, 162), GetGameText(3, 13), GetGameText(3, 24));
			PrintInLeftPanel(1, 3, GetGameText(3, 163), GetGameText(3, 14), GetGameText(3, 25));
			PrintInLeftPanel(1, 4, GetGameText(3, 164), GetGameText(3, 15), GetGameText(3, 26));
			PrintInLeftPanel(1, 5, GetGameText(3, 165), GetGameText(3, 16), GetGameText(3, 27));
			PrintInLeftPanel(1, 6, GetGameText(3, 166), GetGameText(3, 17), GetGameText(3, 28));
			PrintInLeftPanel(1, 7, GetGameText(3, 167), GetGameText(3, 18), GetGameText(3, 29));
			PrintInLeftPanel(1, 8, GetGameText(3, 168), GetGameText(3, 19), GetGameText(3, 30));
			PrintInLeftPanel(1, 9, GetGameText(3, 169), GetGameText(3, 20), GetGameText(3, 31));
			PrintInLeftPanel(1, 10, GetGameText(3, 170), GetGameText(3, 21));
			do
				word = PromptForWord(GetGameText(5, 21));
			while (*word != 'x' && *word != 'X' && atoi(word) < 0 && atoi(word) > 31);
			if (atoi(word) >= 0 && atoi(word) <= 31) {
				Npc_setSchedule(npc, atoi(word));
				GetNpcBufferForIbo(npc)->schedules[GetNpcBufferForIbo(npc)->currentSchedule].state = 1;
			}
			break;
		case 'W':
			if ((unsigned char)(GetNpcBufferForIbo(npc)->typeFlagsHigh & 1))
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
	char cmd;
	int i;
	int current;

	current = GetNpcBufferForIbo(npc)->currentSchedule;
	ClearLeftPanel();
	ClearRightPanel();
	PrintInLeftPanel(1, 0, GetGameText(3, 118), GetGameText(5, 122), Item_getNpcNumber(npc));
	PrintInLeftPanel(1, 2, GetGameText(3, 119), YN((unsigned char) (Item_getQualityFlags(npc) & QUALITY_BUSY)),
		YN(IsNpcUnconscious(npc)));
	if (HasStatus(npc, NPC_IN_PARTY))
		PrintInLeftPanel(21, 2, GetGameText(3, 120), PartySitRefs[GetPartyIndex(*npc)]);
	PrintInLeftPanel(1, 3, GetGameText(3, 121), GetNpcBufferForIbo(npc)->scheduleValue,
		GetNpcBufferForIbo(npc)->scheduleToggle, GetNpcBufferForIbo(npc)->scheduleC, GetNpcBufferForIbo(npc)->result);
	PrintInLeftPanel(1, 4, GetGameText(3, 122), Npc_getScheduleVariable0(npc), Npc_getScheduleVariable1(npc));
	for (i = 0; i <= current; i++) {
		GetNpcBufferForIbo(npc)->currentSchedule = i;
		PrintInLeftPanel(1, i + 6, GetGameText(3, 123), GetNpcBufferForIbo(npc)->currentSchedule == current ? '>' : '+',
			GetGameText(3, GetNpcBufferForIbo(npc)->schedules[GetNpcBufferForIbo(npc)->currentSchedule].kind));
		PrintInLeftPanel(13, i + 6, GetGameText(3, 124),
			GetNpcBufferForIbo(npc)->schedules[GetNpcBufferForIbo(npc)->currentSchedule].state);
		PrintInLeftPanel(19, i + 6, GetGameText(3, 125),
			GetNpcBufferForIbo(npc)->schedules[GetNpcBufferForIbo(npc)->currentSchedule].x);
		PrintInLeftPanel(27, i + 6, GetGameText(3, 126),
			GetNpcBufferForIbo(npc)->schedules[GetNpcBufferForIbo(npc)->currentSchedule].y);
		PrintInLeftPanel(35, i + 6, GetGameText(3, 127),
			GetNpcBufferForIbo(npc)->schedules[GetNpcBufferForIbo(npc)->currentSchedule].counter);
	}
	GetNpcBufferForIbo(npc)->currentSchedule = current;
	PrintInRightPanel(1, 0, GetGameText(5, 123));
	do {
		char key = PromptForKey(GetGameText(5, 124));

		cmd = key >= 'a' && key <= 'z' ? key - 32 : key;
	} while (cmd != 'X');
}

/* the debug menu */
void ShowDebugMenu(void)
{
	unsigned char workType;
	objref item;
	int number;
	int i;
	char cmd;
	char inRange;
	char buf[4];
	NPCRef who;
	objref target;
	unsigned char sx, sy, region;
	Coord x, y;
	int day;

	textmode(C80);
	textcolor(15);
	HidePointer();
	for (;;) {
		ClearLeftPanel();
		ClearRightPanel();
		PrintInRightPanel(1, 0, GetGameText(3, 128), GetGameText(5, 131), HackMoverEnabled ? "Yes" : "No");
		if (DoScheduleNpc == -1)
			strcpy(buf, GetGameText(5, 132));
		else
			sprintf(buf, GetGameText(3, 129), DoScheduleNpc);
		PrintInRightPanel(1, 1, GetGameText(3, 130), GetGameText(5, 133), buf,
			GetGameText(5, 134), ShowAvatarLocation ? "On" : "Off");
		PrintInRightPanel(1, 2, GetGameText(3, 95), GetGameText(5, 135));
		PrintInRightPanel(1, 3, GetGameText(3, 131), GetGameText(5, 136), ShowNpcNumbers ? "On" : "Off",
			GetGameText(5, 137));
		PrintInRightPanel(1, 4, GetGameText(3, 132), GetGameText(5, 138), UnkBugChecking ? "Yes" : "No");
		PrintInRightPanel(1, 5, GetGameText(3, 133), GetGameText(5, 139), PowerAvatar ? "On" : "Off",
			GetGameText(5, 140), QueueToggle ? "On" : "Off");
		PrintInRightPanel(1, 6, GetGameText(3, 95), GetGameText(5, 141));
		PrintInRightPanel(1, 7, GetGameText(3, 134), GetGameText(5, 142), GameTime.rate, GetGameText(5, 143));
		PrintInRightPanel(1, 8, GetGameText(3, 135), GetGameText(5, 144), FollowersEnabled ? "Yes" : "No",
			GetGameText(5, 145));
		PrintInLeftPanel(1, 0, GetGameText(3, 136), GetGameText(5, 146),
			GameTime.getHour() <= 12 ? GameTime.getHour() : GameTime.getHour() - 12,
			GameTime.getMinute(), GameTime.getHour() < 12 ? "AM" : "PM");
		PrintInLeftPanel(1, 1, GetGameText(3, 95), GameVersion);
		PrintInLeftPanel(20, 1, GetGameText(3, 118), " fn:  ", ItemFreeCount);
		PrintInLeftPanel(35, 1, GetGameText(3, 118), "<fn:  ", LowFn);
		PrintInLeftPanel(50, 1, GetGameText(3, 118), ">fn:  ", HighFn);
		PrintInLeftPanel(1, 2, GetGameText(3, 137), GetGameText(5, 182),
			Item_getX(AvatarRef).value, Item_getY(AvatarRef).value);
		PrintInLeftPanel(1, 3, GetGameText(3, 138), GetGameText(5, 183),
			Item_getX(AvatarRef).value, Item_getY(AvatarRef).value);
		PrintInLeftPanel(1, 4, GetGameText(3, 139), GetGameText(5, 184), Item_getZ(&AvatarRef));
		char key = PromptForKey(GetGameText(5, 147));
		switch (cmd = key >= 'a' && key <= 'z' ? key - 32 : key) {
		case 'F':
			for (i = 1; i <= 4; i++) {
				GetNpcIbo(&who, i);
				if (FollowersEnabled) {
					RemoveFromParty(who, 1);
				} else {
					AddToParty(who, 0);
				}
			}
			FollowersEnabled = !FollowersEnabled;
			break;
		case 'A':
			number = PromptForIntegerWord(GetGameText(5, 148));
			GetNpcIbo(&who, number);
			if (who.valid())
				DumpNpcActivity(&who);
			else
				ConsoleShowMessage(GetGameText(5, 149));
			break;
		case 'H':
			HackMoverEnabled = !HackMoverEnabled;
			break;
		case 'U':
			UnkBugChecking = !UnkBugChecking;
			break;
		case '=':
			if ((int)GameTime.rate < 25)
				GameTime.rate = GameTime.rate + 1;
			break;
		case '-':
			if ((int)GameTime.rate > 1)
				GameTime.rate = GameTime.rate - 1;
			break;
		case 'C':
			number = PromptForIntegerWord(GetGameText(5, 150));
			if (number == -1) {
				ConsoleShowMessage(GetGameText(5, 151));
				break;
			}
			if (IsNpcType(number & 0x3ff)) {
				ConsoleShowMessage(GetGameText(5, 151));
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
						ConsoleShowMessage(GetGameText(5, 152));
					break;
				}
			}
			if (!item.valid())
				break;
			if (i == 10) {
				ConsoleShowMessage(GetGameText(5, 153));
				break;
			}
			number = PromptForIntegerWord(GetGameText(5, 154));
			if (number == -1) {
				Item_delete(&item);
				ConsoleShowMessage(GetGameText(5, 155));
				break;
			}
			Item_setFrame(&item, number);
			if (Item_hasQuantity(&item)) {
				number = PromptForIntegerWord(GetGameText(5, 156));
				Item_storeQuantity(&item, number);
			}
			if (Item_hasQuality(&item)) {
				number = PromptForIntegerWord(GetGameText(5, 157));
				Item_setQuality(&item, number);
			}
			ConsoleShowMessage(GetGameText(5, 158));
			break;
		case 'S':
			PrintInLeftPanel(1, 0, GetGameText(3, 140), GetGameText(5, 159),
				GameTime.getHour() <= 12 ? GameTime.getHour() : GameTime.getHour() - 12,
				GameTime.getMinute(), GameTime.getHour() < 12 ? "AM" : "PM");
			number = PromptForIntegerWord(GetGameText(5, 160));
			day = PromptForIntegerWord(GetGameText(5, 161));
			if (number != -1 && day != -1) {
				GameTime.ticks = number * TICKS_PER_HOUR;
				GameTime.days = day;
				UpdateNpcSchedules();
			} else
				ConsoleShowMessage(GetGameText(5, 162));
			break;
		case 'P':
			PowerAvatar = !PowerAvatar;
			break;
		case 'I':
			number = PromptForIntegerWord(GetGameText(5, 163));
			GetNpcIbo(&who, number);
			if (who.valid()) {
				ClearLeftPanel();
				PrintInLeftPanel(1, 0, GetGameText(3, 118), GetGameText(5, 164), Item_getNpcNumber(&who));
				PrintInLeftPanel(1, 1, GetGameText(3, 141),
					YN((unsigned char)(Item_getQualityFlags(&who) & QUALITY_BUSY)),
					YN(HasStatus(&who, NPC_DEAD)),
					YN((unsigned char)(NPC(&who)->typeFlagsHigh & 0x40)),
					YN((unsigned char)(NPC(&who)->typeFlagsHigh & 0x80)),
					YN(HasPath(who)),
					YN((unsigned char)gItemTypeInfo[who.ptr()->typeFrame & 0x3ff].solid),
					YN(CanVisit(&who)));
				PrintInLeftPanel(1, 2, GetGameText(3, 142),
					Item_getHitPoints(&who),
					(unsigned char)(NPC(&who)->strength & 0x1f),
					(unsigned char)(NPC(&who)->intelligence & 0x1f),
					(unsigned char)(NPC(&who)->combat & 0x1f),
					NPC(&who)->dexterity,
					(unsigned char)(NPC(&who)->mana & 0x1f),
					NPC(&who)->food);
				PrintInLeftPanel(1, 3, GetGameText(3, 143),
					(int)Item_getX(who), (int)Item_getY(who), Item_getZ(&who),
					NPC(&who)->training, NPC(&who)->experience);
				PrintInLeftPanel(1, 4, GetGameText(3, 144),
					YN((unsigned char)(NPC(&who)->typeFlags & NPC_FLY)),
					YN((unsigned char)(NPC(&who)->typeFlags & NPC_WALK)),
					YN((unsigned char)(NPC(&who)->typeFlags & NPC_SWIM)),
					YN((unsigned char)(NPC(&who)->typeFlags & NPC_ETHEREAL)),
					GetGameText(3, NPC(&who)->workType),
					NPC(&who)->schedules[NPC(&who)->currentSchedule].state,
					NPC(&who)->schedules[NPC(&who)->currentSchedule].x,
					NPC(&who)->schedules[NPC(&who)->currentSchedule].y,
					NPC(&who)->schedules[NPC(&who)->currentSchedule].counter);
				PrintInLeftPanel(1, 5, GetGameText(3, 145),
					YN(HasStatus(&who, NPC_ASLEEP)),
					YN(HasStatus(&who, NPC_CHARMED)),
					YN(HasStatus(&who, NPC_CURSED)),
					YN(HasStatus(&who, NPC_PARALYZED)),
					YN(HasStatus(&who, NPC_POISONED)),
					YN(HasStatus(&who, NPC_PROTECTED)),
					GetFootprintX(*(TypeFrame far *)&who.ptr()->typeFrame) + 1,
					GetFootprintY(*(TypeFrame far *)&who.ptr()->typeFrame) + 1,
					gItemTypeInfo[who.ptr()->typeFrame & 0x3ff].height);
				PrintInLeftPanel(1, 7, GetGameText(3, 146),
					NPC(&who)->workType == WORK_COMBAT ? GetGameText(5, Item_getQuality(&who) + 47) : "None     ",
					GetGameText(5, NPC(&who)->defaultAttackMode + 47),
					GetGameText(5, GetAlignment(&who) + 58));
				PrintInLeftPanel(1, 8, GetGameText(3, 148),
					NPC(&who)->primaryTarget,
					NPC(&who)->secondaryTarget,
					NPC(&who)->oppressor,
					YN((unsigned char)(NPC(&who)->typeFlagsHigh & 1)),
					YN((unsigned char)(NPC(&who)->typeFlagsHigh & 0x20)),
					YN(HasStatus(&who, NPC_IN_PARTY)),
					YN(CombatGroups.isProtectee(Item_getNpcNumber(&who))));
				PrintInLeftPanel(1, 9, GetGameText(3, 149),
					NPC(&who)->iVr[0], NPC(&who)->iVr[1],
					Npc_getScheduleVariable0(&who), Npc_getScheduleVariable1(&who),
					Npc_getMovementRadius(&who));
				GetNpcIbo(&target, GetCombatTarget(&who));
				if (target.valid()) {
					if (GetMissileDistance(who, target) <= GetAttackRange(&who))
						inRange = 'Y';
					else
						inRange = 'N';
				} else
					inRange = '*';
				PrintInLeftPanel(1, 10, GetGameText(3, 150),
					CombatGroups.movePoints[Item_getNpcNumber(&who)],
					GetAttackRange(&who),
					inRange,
					target.valid() ? YN(HasLineOfFire(who, target)) : '*',
					YN(CombatGroups.partyAttacked),
					YN(CombatGroups.battleMusic | AvatarInCombat));
				ConsoleShowMessage("");
			} else
				ConsoleShowMessage(GetGameText(5, 165));
			break;
		case 'T':
			ShowTeleportMenu();
			break;
		case 'X':
			ShowPointer();
			goto leave;
		case 'N':
			ShowNpcNumbers = !ShowNpcNumbers;
			break;
		case 'L':
			ShowAvatarLocation = !ShowAvatarLocation;
			break;
		case 'G':
			ClearLeftPanel();
			ClearRightPanel();
			PrintInLeftPanel(1, 0, GetGameText(5, 166));
			PrintInLeftPanel(1, 1, GetGameText(5, 167));
			PrintInLeftPanel(1, 10, GetGameText(5, 168));
			char option;

			for (;;) {
				option = PromptForKey(GetGameText(5, 169));
				cmd = option >= 'a' && option <= 'z' ? option - 32 : option;
				if (cmd == 'I')
					GameFlags.show();
				else if (cmd == 'S')
					GameFlags.edit();
				else if (cmd == 'x' || cmd == 'X')
					goto leave;
				else
					ConsoleShowMessage(GetGameText(5, 170));
				ConsoleBlankLine(1, 4);
				ConsoleBlankLine(1, 5);
				ConsoleBlankLine(1, 6);
			}
		case 'B':
			number = PromptForIntegerWord(GetGameText(5, 171));
			if (number >= 256) {
				ConsoleShowMessage(GetGameText(5, 172));
				break;
			}
			GetNpcIbo(&who, number);
			if (who.valid()) {
				ClearLeftPanel();
				cmd = 's';
				while (cmd != 'x') {
					for (i = 0; i < 8; i++) {
						workType = Schedule_getEntryWorkType(&ScheduleTable, number, i);
						if (workType != 255) {
							sx = Schedule_getEntryX(&ScheduleTable, Item_getNpcNumber(&who), i);
							sy = Schedule_getEntryY(&ScheduleTable, Item_getNpcNumber(&who), i);
							region = Schedule_getEntryRegion(&ScheduleTable, Item_getNpcNumber(&who), i);
							x = RegionX[region] + sx;
							y = RegionY[region] + sy;
						}
						if (workType == 255)
							PrintInLeftPanel(1, i + 1, GetGameText(3, 151),
								i * 3 - (i > 0 ? 12 : 0) - (i > 4 ? 12 : 0) + 12,
								i < 4 ? GetGameText(3, 152) : GetGameText(3, 153));
						else
							PrintInLeftPanel(1, i + 1, GetGameText(3, 154),
								i * 3 - (i > 0 ? 12 : 0) - (i > 4 ? 12 : 0) + 12,
								i < 4 ? GetGameText(3, 152) : GetGameText(3, 153),
								(int)x, (int)y, GetGameText(3, workType));
					}
					PrintInLeftPanel(1, 10, GetGameText(3, 155), GetGameText(5, 173),
						GetGameText(3, NPC(&who)->workType));
					do
						cmd = PromptForKey(GetGameText(5, 185));
					while ((cmd < '0' || cmd > '7') && cmd != 'x' && cmd != 'r');
					if (cmd != 'x' && cmd != 'r') {
						if (cmd == '0')
							i = 0;
						else
							i = cmd - '0';
						workType = Schedule_getEntryWorkType(&ScheduleTable, number, i);
						if (workType == 255) {
							Schedule_insertEntry(&ScheduleTable, number);
							sy = (unsigned char)(*(ScheduleTable.index + (unsigned char)number + 1)
								- ScheduleTable.index[(unsigned char)number]) - 1;
							Schedule_setEntryTime(&ScheduleTable, number, sy, i);
						}
						do
							cmd = PromptForKey(GetGameText(5, 209));
						while (cmd != 'c' && cmd != 'l' && cmd != 'a');
						switch (cmd = cmd >= 'a' && cmd <= 'z' ? cmd - 32 : cmd) {
						case 'A':
							workType = 35;
							do
								workType = PromptForIntegerWord(GetGameText(5, 204));
							while (workType < 0 || workType > 31);
							Schedule_setEntryWorkType(&ScheduleTable, number,
								Schedule_findEntry(&ScheduleTable, number, i), workType);
							break;
						case 'L': {
							int placeX, placeY, placeRegion;

							placeX = PromptForLong(GetGameText(5, 206), 0);
							placeY = PromptForLong(GetGameText(5, 207), 0);
							placeRegion = 0;
							GetRegionPosition(&placeX, &placeY, &placeRegion);
							Schedule_setEntryPlace(&ScheduleTable, number,
								Schedule_findEntry(&ScheduleTable, number, i), placeX, placeY, placeRegion);
							break;
						}
						case 'C':
							Schedule_removeEntry(&ScheduleTable, number,
								Schedule_findEntry(&ScheduleTable, number, i));
							break;
						}
					} else if (cmd == 'r') {
						cmd = PromptForKey(GetGameText(5, 208));
						if (cmd == 'y')
							RevertSchedule(number);
					}
				}
			} else
				ConsoleShowMessage(GetGameText(5, 174));
			break;
		case 'D':
			DoScheduleNpc = number = PromptForIntegerWord(GetGameText(5, 175));
			break;
		case 'M':
			number = PromptForIntegerWord(GetGameText(5, 176));
			GetNpcIbo(&who, number);
			if (who.valid())
				ModifyNpc(&who);
			else
				ConsoleShowMessage(GetGameText(5, 177));
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
leave:
	RestoreGameScreen();
}

/* creates a readable item and shows its text for a range of qualities */
void ShowReadableTexts(void)
{
	char key;
	int first;
	objref thing;
	int quality;
	int last;

	switch (key = PromptForKey(GetGameText(5, 126))) {
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
	first = PromptForIntegerWord(GetGameText(5, 127));
	last = PromptForIntegerWord(GetGameText(5, 128));
	for (quality = first; quality < last + 1; quality++) {
		ConsolePrintAt(1, 1, GetGameText(3, 118), GetGameText(5, 126), quality);
		Item_setQuality(&thing, quality);
		RunUsable(1, thing, -1);
	}
	ZapDetachedItem(&thing);
}

/* plays a sound effect by number */
void CheatPlaySound(void)
{
	int number = PromptForIntegerWord(GetGameText(5, 205));

	PlaySoundAtItem(number, AvatarRef);
}

/* plays a music track by number */
void CheatPlayMusic(void)
{
	int number = PromptForIntegerWord(GetGameText(5, 205));

	PlayMusic(number);
}

/* moves everything inside a picked container out to the container's spot */
void EmptyPickedContainer(void)
{
	AreaSearch found;
	int z;
	objref container;
	objref thing;
	Coord x, y;

	HavePlayerSelect(&container, &x, &y, &z);
	if (container.valid()
		&& (unsigned char) (ItemTypeClassFlags[gItemTypeInfo[container.type()].typeClass] & CLASS_CONTENTS)) {
		FindItemInContainer(&found, container, 0x100, -1, 255, 255);
		while (found.found()) {
			thing = found.current;
			FindItem(&found);
			Item_move(&thing, x, y, z);
		}
	}
}

/* asks which usecode event to run, and optionally for which usable index; 0 when abandoned */
char PromptForUsecodeEvent(char *event, int *index)
{
	char key;
	char choice;
	char reply;

	textmode(C80);
	textcolor(15);
	ClearLeftPanel();
	ClearRightPanel();
	PrintInLeftPanel(1, 0, GetGameText(3, 95), GetGameText(5, 186));
	PrintInLeftPanel(1, 1, GetGameText(3, 95), GetGameText(5, 187));
	PrintInLeftPanel(1, 2, GetGameText(3, 95), GetGameText(5, 188));
	PrintInLeftPanel(1, 3, GetGameText(3, 95), GetGameText(5, 189));
	PrintInLeftPanel(1, 4, GetGameText(3, 95), GetGameText(5, 190));
	PrintInLeftPanel(1, 5, GetGameText(3, 95), GetGameText(5, 191));
	PrintInLeftPanel(23, 1, GetGameText(3, 95), GetGameText(5, 192));
	PrintInLeftPanel(23, 2, GetGameText(3, 95), GetGameText(5, 193));
	PrintInLeftPanel(23, 3, GetGameText(3, 95), GetGameText(5, 194));
	PrintInLeftPanel(23, 4, GetGameText(3, 95), GetGameText(5, 195));
	PrintInLeftPanel(23, 5, GetGameText(3, 95), GetGameText(5, 196));
	PrintInLeftPanel(23, 6, GetGameText(3, 95), GetGameText(5, 197));
	do {
		key = PromptForKey(GetGameText(5, 198));
		*event = key >= 'a' && key <= 'z' ? key - 32 : key;
	} while ((*event < '0' || *event > '9') && *event != 'X');
	if (*event == 'X') {
		RestoreGameScreen();
		return 0;
	}
	*event = atoi(event);
	ClearLeftPanel();
	PrintInLeftPanel(1, 1, GetGameText(3, 95), GetGameText(5, 199));
	PrintInLeftPanel(1, 2, GetGameText(3, 95), GetGameText(5, 200));
	PrintInLeftPanel(1, 3, GetGameText(3, 95), GetGameText(5, 201));
	do {
		reply = PromptForKey(GetGameText(5, 202));
		choice = reply >= 'a' && reply <= 'z' ? reply - 32 : reply;
	} while (choice != 'X' && choice != 'Y' && choice != 'N');
	if (choice == 'X') {
		RestoreGameScreen();
		return 0;
	}
	*index = -1;
	if (choice == 'Y')
		*index = PromptForLong(GetGameText(5, 203), 0);
	RestoreGameScreen();
	return 1;
}
