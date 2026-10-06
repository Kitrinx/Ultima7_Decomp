/* Serpent Isle SI.EXE, overlay segment 215 (file offsets 0x05a570 to 0x05aff0, 2688 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 */

#include "itemrec.h"
#include "iteminfo.h"
#include "item.h"
#include "coord.h"
#include "target.h"
#include "itable.h"
#include "text.h"
#include "debug.h"
#include "trigger.h"
#include "cheat.h"

#define SHAPE_EGG   275
#define SHAPE_PATH  607

#define EGG(r) ((EggRecord far *)MK_FP(ItemBufferSegment, ITEM((r).off)->data.extra))
#define YN(c) ((c) ? 'Y' : 'N')

/* F9: describe the item the player picks, in full for an egg or a path. */
void ShowTargetReport(void)
{
	objref selected;
	Coord x, y;
	int z;
	int i, line;

	HavePlayerSelect(&selected, &x, &y, &z);
	if ((selected.ptr()->typeFrame & 0x3ff) == SHAPE_EGG) {
		objref egg = selected;
		HidePointer();
		for (i = 1; i < 18; i++)
			ConsoleBlankLine(1, i);
		ConsolePrintAt(1, 1, GetGameText(3, 55), selected.off, x.value, y.value, x.value, y.value, z);
		ConsolePrintAt(1, 3, GetGameText(3, 56), GetGameText(7, (unsigned char)EGG(egg)->kind));
		ConsolePrintAt(1, 4, GetGameText(3, 57), GetGameText(7, EGG(egg)->criteria + 11));
		ConsolePrintAt(1, 5, GetGameText(3, 58), EGG(egg)->distance);
		ConsolePrintAt(1, 6, GetGameText(3, 59), EGG(egg)->probability);
		ConsolePrintAt(1, 7, GetGameText(3, 60), YN(EGG(egg)->nocturnal));
		ConsolePrintAt(1, 8, GetGameText(3, 61), YN(EGG(egg)->once));
		ConsolePrintAt(1, 9, GetGameText(3, 62), YN(EGG(egg)->hatched));
		ConsolePrintAt(1, 10, GetGameText(3, 63), YN(EGG(egg)->repeat));
		switch ((unsigned char)EGG(egg)->kind) {
		case 0:
		case EGG_MONSTER:
			ConsolePrintAt(1, 13, GetGameText(3, 64), GetGameText(0, EGG(egg)->data2.word & 0x3ff));
			ConsolePrintAt(1, 14, GetGameText(3, 65), (unsigned char)(EGG(egg)->data1.bytes.first >> 2));
			ConsolePrintAt(1, 15, GetGameText(3, 66),
				GetGameText(5, (unsigned char)(EGG(egg)->data1.bytes.first & 3) + 58));
			ConsolePrintAt(1, 16, GetGameText(3, 67), GetGameText(3, EGG(egg)->data1.bytes.second));
			break;
		case EGG_MUSIC:
			ConsolePrintAt(1, 13, GetGameText(3, 68), EGG(egg)->data1.bytes.first);
			ConsolePrintAt(1, 14, GetGameText(3, 69), YN(EGG(egg)->data1.bytes.second));
			break;
		case EGG_SFX:
			ConsolePrintAt(1, 13, GetGameText(3, 70), EGG(egg)->data1.bytes.first);
			ConsolePrintAt(1, 14, GetGameText(3, 71), YN(EGG(egg)->data1.bytes.second));
			break;
		case EGG_SPEECH:
			ConsolePrintAt(1, 13, GetGameText(3, 72), EGG(egg)->data1.bytes.first);
			break;
		case EGG_USECODE:
			ConsolePrintAt(1, 13, GetGameText(3, 73), EGG(egg)->data1.bytes.first);
			ConsolePrintAt(1, 14, GetGameText(3, 74), EGG(egg)->data1.bytes.second);
			ConsolePrintAt(1, 15, GetGameText(3, 75), EGG(egg)->data2.word);
			break;
		case EGG_MISSILE:
			ConsolePrintAt(1, 13, GetGameText(3, 76), GetGameText(0, EGG(egg)->data1.word & 0x3ff));
			ConsolePrintAt(1, 14, GetGameText(3, 77), GetGameText(7, EGG(egg)->data2.bytes.first + 19));
			ConsolePrintAt(1, 15, GetGameText(3, 78), EGG(egg)->data2.bytes.second);
			break;
		case EGG_TELEPORT:
			ConsolePrintAt(1, 13, GetGameText(3, 79), EGG(egg)->data1.bytes.first);
			ConsolePrintAt(1, 14, GetGameText(3, 80), EGG(egg)->data1.bytes.second);
			ConsolePrintAt(1, 15, GetGameText(3, 81), EGG(egg)->data2.bytes.first);
			ConsolePrintAt(1, 16, GetGameText(3, 82), EGG(egg)->data2.bytes.second);
			break;
		case EGG_WEATHER:
			ConsolePrintAt(1, 13, GetGameText(3, 83), GetGameText(7, EGG(egg)->data1.bytes.first + 27));
			if (EGG(egg)->data1.bytes.second)
				ConsolePrintAt(1, 14, GetGameText(3, 84), EGG(egg)->data1.bytes.second);
			else
				ConsolePrintAt(1, 14, GetGameText(3, 85));
			break;
		case 9:
			ConsolePrintAt(1, 13, GetGameText(3, 86), EGG(egg)->data1.bytes.first);
			ConsolePrintAt(1, 14, GetGameText(3, 87), EGG(egg)->data1.bytes.second);
			break;
		case EGG_BUTTON:
			ConsolePrintAt(1, 13, GetGameText(3, 88), EGG(egg)->data1.bytes.first);
			break;
		}
		ConsolePrintAndWait(1, 17, GetGameText(3, 89));
		ShowPointer();
	} else if ((selected.ptr()->typeFrame & 0x3ff) == SHAPE_PATH) {
		HidePointer();
		for (i = 1; i < 12; i++)
			ConsoleBlankLine(1, i);
		ConsolePrintAt(1, 1, GetGameText(3, 90));
		ConsolePrintAt(1, 3, GetGameText(3, 91), (unsigned char)Item_getQuality(&selected));
		ConsolePrintAt(1, 5, GetGameText(3, 92), GetGameText(4, (unsigned char)Item_getQuality(&selected) & 31));
		for (line = 0, i = 32; i <= 128; i <<= 1, line++)
			ConsolePrintAt(1, line + 7, GetGameText(3, 93), GetGameText(4, line + 26),
				(char)YN((unsigned char)Item_getQuality(&selected) & i));
		ConsolePrintAndWait(1, 11, GetGameText(3, 89));
		ShowPointer();
	} else {
		CheatPrintfAtCoords(1, 1, GetGameText(3, 94), selected.off);
		CheatPrintfAtCoords(1, 2, GetGameText(3, 176), x.value, y.value, z);
		CheatPrintfAtCoords(1, 3, GetGameText(3, 177), x.value, y.value, z);
		CheatPrintfAtCoords(1, 4, GetGameText(3, 178), selected.ptr()->typeFrame & 0x3ff,
			(selected.ptr()->typeFrame & 0x7c00) >> 10);
		CheatPrintfAtCoords(1, 5, GetGameText(3, 179), Item_getHitPoints(&selected),
			(unsigned char)Item_getQuality(&selected), Item_getQuantity(&selected),
			YN((unsigned char)(Item_getQualityFlags(&selected) & QUALITY_BUSY)));
		CheatPrintfAtCoords(1, 6, GetGameText(3, 180), YN(Item_isOkayToTake(&selected)),
			YN(IsTemporary(&selected)));
		CheatPrintfAtCoordsWait(1, 8, GetGameText(3, 89));
	}
}
