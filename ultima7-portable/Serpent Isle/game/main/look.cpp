/* Serpent Isle SI.EXE, overlay segment 227 (file offsets 0x05f530 to 0x060fbd, 6797 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 * Filename and folder inferred.
 */

#include "u7port.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "dosio.h"
#include "text.h"
#include "item.h"
#include "type.h"
#include "npcref.h"
#include "death.h"
#include "u7manage.h"

#define ITEM(off) ((struct ItemRecord *) ItemAt((off)))

/* a line of text read in pieces from pos */
struct LookScanner {
	char text[80];
	int16_t pos;
	LookScanner() { text[0] = 0; pos = 0; }
	uint8_t empty() { return strcmp(text, "") == 0; }
	uint8_t has(int8_t c) { return strchr(text, c) != 0; }
};

#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define FRAME(rec) (((rec)->typeFrame & 0x7c00) >> 10)
#define TYPE_CLASS(rec) (gItemTypeInfo[TYPE(rec)].typeClass)

char *DescriptFileName = "DESCRIPT.FLX";

uint8_t HasNamedNpcBody(uint16_t npcNumber)
{
	if (npcNumber) {
		if (npcNumber < 5)
			return 1;
		if (npcNumber > 12 && npcNumber < 21)
			return 1;
		if (npcNumber > 25 && npcNumber < 66)
			return 1;
		if (npcNumber > 66 && npcNumber < 82)
			return 1;
		if (npcNumber > 12 && npcNumber < 21)
			return 1;
		if (npcNumber > 12 && npcNumber < 21)
			return 1;
		if (npcNumber > 142 && npcNumber < 151)
			return 1;
		if (npcNumber > 151 && npcNumber < 160)
			return 1;
		switch (npcNumber) {
		case 22:
		case 23:
		case 24:
		case 127:
		case 128:
		case 163:
		case 168:
		case 169:
		case 170:
		case 181:
		case 182:
		case 195:
		case 207:
		case 209:
		case 211:
		case 212:
		case 213:
		case 217:
		case 222:
		case 267:
		case 272:
		case 275:
		case 280:
		case 287:
		case 294:
			return 1;
		}
	}
	return 0;
}

/* add one character to the end of s */
void AppendChar(char *s, int8_t c)
{
	char *end = s + strlen(s);

	*end = c;
	end++;
	*end = 0;
}

/* add text entry table[n] to s, n held to the last of count entries */
void AppendTextFromTable(char *s, int16_t n, int16_t count, int16_t *table)
{
	if (n >= count)
		n = count - 1;
	strcat(s, GetGameText(2, table[n]));
}

/* add text entry first + n to s, n held to the last of count entries */
void AppendTextEntry(char *s, int16_t n, int16_t count, int16_t first)
{
	if (n >= count)
		n = count - 1;
	strcat(s, GetGameText(2, first + n));
}

/* add text entry same to s when a equals b, else entry other */
void AppendTextChoice(char *s, int16_t a, int16_t b, int16_t same, int16_t other)
{
	if (a == b)
		strcat(s, GetGameText(2, same));
	else
		strcat(s, GetGameText(2, other));
}

/* copy text to out up to the next of the delimiters; step past it and return it */
int8_t ScanToDelimiter(LookScanner *sc, char *out, char *delims)
{
	char *s;

	s = sc->text + sc->pos;
	while (*s != 0 && strchr(delims, *s) == 0) {
		AppendChar(out, *s);
		s++;
		sc->pos++;
	}
	if (*s != 0)
		sc->pos++;
	return *s;
}

/* print into the work string, then copy at most 80 characters to out */
void FormatName(char *out, char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
	}
	strncpy(out, WorkString, 80);
}

/* An item's name, including its frame, quality and quantity. */
void ProduceItemDisplayName(char *out, objref *item, uint8_t quantity)
{
	LookScanner name, base, result, article, single, plural;
	uint16_t frame = FRAME(ITEM(item->off));
	uint8_t quality = 0;
	char npcName[20];

	if (Item_hasQuality(item))
		quality = (uint8_t)Item_getQuality(item);
	if (item->isNpc() && !NPCRef(item->off).isAvatar()) {
		NPCRef npc(item->off);
		if (Npc_hasPolymorphFlag(&npc)) {
			int16_t shape = FindShapeRecord(TYPE(ITEM(item->off)));
			if (shape == 721 || shape == 989 || shape > 1023 && shape < 1030)
				strcat(result.text, GetGameText(2, 325));
			else if (shape > 1029) {
				if (TYPE(ITEM(item->off)) == 721)
					strcat(result.text, GetGameText(2, 325));
				else if (shape % 2)
					strcat(result.text, GetGameText(2, 383));
				else
					strcat(result.text, GetGameText(2, 382));
			} else
				strcat(name.text, GetGameText(0, shape));
		} else if (Npc_hasMetFlag(&npc)) {
			char *s = GetNpcBufferForIbo(&npc)->name;
			for (int8_t i = 0; i < 20; i++)
				npcName[i] = s[i];
			FormatName(name.text, npcName);
		} else
			strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
	} else {
		switch (TYPE(ITEM(item->off))) {
		case 842:
			AppendTextEntry(name.text, frame, 0x10, 0);
			break;
		case 467:
			if (frame < 2)
				strcat(result.text, GetGameText(2, 129));
			else if (frame < 4)
				strcat(result.text, GetGameText(2, 130));
			else if (frame == 5)
				strcat(result.text, GetGameText(2, 131));
			else if (frame == 6)
				strcat(result.text, GetGameText(2, 132));
			break;
		case 450:
			AppendTextEntry(name.text, frame, 0xd, 0x85);
			break;
		case 404:
			AppendTextEntry(name.text, frame, 5, 0x140);
			break;
		case 300:
			if (frame < 2)
				strcat(result.text, GetGameText(2, 310));
			else if (frame < 4)
				strcat(result.text, GetGameText(2, 311));
			else if (frame < 6)
				strcat(result.text, GetGameText(2, 312));
			else if (frame < 8)
				strcat(result.text, GetGameText(2, 313));
			else if (frame == 8 || frame == 12 || frame == 16 || frame == 20)
				strcat(result.text, GetGameText(2, 314));
			else if (frame == 9 || frame == 13 || frame == 17 || frame == 21)
				strcat(result.text, GetGameText(2, 315));
			else if (frame == 10 || frame == 14 || frame == 18 || frame == 22)
				strcat(result.text, GetGameText(2, 316));
			else if (frame == 11 || frame == 15 || frame == 19 || frame == 23)
				strcat(result.text, GetGameText(2, 317));
			else if (frame & 1)
				strcat(result.text, GetGameText(2, 319));
			else
				strcat(result.text, GetGameText(2, 318));
			break;
		case 648:
			if (frame == 2)
				strcat(result.text, GetGameText(2, 348));
			else if (frame == 3)
				strcat(result.text, GetGameText(2, 349));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 587:
			AppendTextEntry(name.text, frame, 7, 0x9c);
			break;
		case 1004:
			AppendTextEntry(name.text, frame, 5, 0xa6);
			break;
		case 542:
			AppendTextEntry(name.text, frame, 3, 0xa3);
			break;
		case 577:
			if (frame < 3)
				strcat(result.text, GetGameText(2, 171));
			else if (frame < 6)
				strcat(result.text, GetGameText(2, 172));
			else if (frame == 6)
				strcat(result.text, GetGameText(2, 173));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 955:
			AppendTextEntry(name.text, frame, 0xa, 0xae);
			break;
		case 863:
			AppendTextEntry(name.text, frame, 0x13, 0x16b);
			break;
		case 999:
			AppendTextEntry(name.text, frame, 0xa, 0x15f);
			break;
		case 262:
			AppendTextEntry(name.text, frame, 4, 0x92);
			break;
		case 209:
			AppendTextEntry(name.text, frame, 0x18, 0xc0);
			break;
		case 178:
			AppendTextEntry(name.text, frame, 6, 0x96);
			break;
		case 244:
			if (frame < 2)
				strcat(result.text, GetGameText(2, 216));
			else
				strcat(result.text, GetGameText(2, 217));
			break;
		case 696:
		case 1011:
			if (frame == 1)
				strcat(result.text, GetGameText(2, 218));
			else if (frame == 2)
				strcat(result.text, GetGameText(2, 219));
			else if (frame == 4)
				strcat(result.text, GetGameText(2, 220));
			else if (frame == 5)
				strcat(result.text, GetGameText(2, 221));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 912:
			if (frame == 24)
				strcat(result.text, GetGameText(2, 222));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 370:
			if (frame == 19)
				strcat(result.text, GetGameText(2, 270));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 210:
			if (frame == 4)
				strcat(result.text, GetGameText(2, 384));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 400:
		case 402:
		case 414:
		case 762:
		case 778:
		case 892:
				if (HasNamedNpcBody(GetBodyNpc(*item))) {
					objref npc;
					GetNpcIbo(&npc, GetBodyNpc(*item));
					char *s = GetNpcBufferForIbo(&npc)->name;
					for (int8_t i = 0; i < 20; i++)
						npcName[i] = s[i];
					if (Npc_hasMetFlag(&npc)) {
						FormatName(name.text, npcName);
						strcat(name.text, GetGameText(2, 362));
					} else
						strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
				} else
					strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 799:
			AppendTextEntry(name.text, frame, 0x13, 0xdf);
			break;
		case 616:
			if (frame == 1)
				strcat(result.text, GetGameText(2, 242));
			else if (frame == 9)
				strcat(result.text, GetGameText(2, 243));
			else if (frame == 16)
				strcat(result.text, GetGameText(2, 244));
			else if (frame == 17)
				strcat(result.text, GetGameText(2, 245));
			else if (frame == 18)
				strcat(result.text, GetGameText(2, 246));
			else if (frame == 20)
				strcat(result.text, GetGameText(2, 247));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 727:
			if (frame < 2)
				strcat(result.text, GetGameText(2, 345));
			else if (frame == 2)
				strcat(result.text, GetGameText(2, 346));
			else
				strcat(result.text, GetGameText(2, 347));
			break;
		case 479:
			if (frame < 2)
				strcat(result.text, GetGameText(2, 248));
			else if (frame < 4)
				strcat(result.text, GetGameText(2, 249));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 285:
			if (frame == 6)
				strcat(result.text, GetGameText(2, 250));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 756:
			if (frame > 1 && frame < 4)
				strcat(result.text, GetGameText(2, 251));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 338:
		case 997:
			strcat(name.text, GetGameText(2, TYPE(ITEM(item->off)) == 338 ? 343 : 344));
			/* Add the shared frame suffix. */
		case 336:
			AppendTextEntry(name.text, frame, 0x11, 0x146);
			break;
		case 292:
			if (frame > 16 && frame < 23 && frame != 20 && frame != 21)
				strcat(result.text, GetGameText(2, 252));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 800:
			if (frame > 3 && frame < 6)
				strcat(result.text, GetGameText(2, 253));
			else if (frame > 5 && frame < 8)
				strcat(result.text, GetGameText(2, 254));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 227:
			AppendTextEntry(name.text, frame, 6, 0xff);
			break;
		case 243:
			if (frame == 7)
				strcat(result.text, GetGameText(2, 261));
			else if (frame == 8)
				strcat(result.text, GetGameText(2, 262));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 330:
			if (frame < 8)
				strcat(result.text, GetGameText(2, 263));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 483:
			if (frame == 2)
				strcat(result.text, GetGameText(2, 264));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 286:
			if (frame == 1 || frame == 2)
				strcat(result.text, GetGameText(2, 361));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 919:
			if (frame == 6)
				strcat(result.text, GetGameText(2, 266));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 303:
		case 876:
			if (frame > 9 && frame < 15)
				strcat(result.text, GetGameText(2, 267));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 935:
		case 936:
			if (frame == 2)
				strcat(result.text, GetGameText(2, 267));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 282:
			if (frame > 5 && frame < 10)
				strcat(result.text, GetGameText(2, 268));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 378:
			if (frame == 3 || frame > 7 && frame < 12)
				strcat(result.text, GetGameText(2, 270));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 415:
			if (frame > 20 && frame < 24)
				strcat(result.text, GetGameText(2, 271));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 869:
			if (frame == 11)
				strcat(result.text, GetGameText(2, 272));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 544:
			if (frame == 14)
				strcat(result.text, GetGameText(2, 273));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 687:
			if (frame == 11)
				strcat(result.text, GetGameText(2, 273));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 812:
			if (frame == 2)
				strcat(result.text, GetGameText(2, 274));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 787:
			if (frame == 8 || frame == 9)
				strcat(result.text, GetGameText(2, 275));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 383:
			if (frame == 1)
				strcat(result.text, GetGameText(2, 276));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 505:
			if (frame == 2)
				strcat(result.text, GetGameText(2, 277));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 519:
			if (frame == 1)
				strcat(result.text, GetGameText(2, 278));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 513:
			if (frame == 5)
				strcat(result.text, GetGameText(2, 269));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 309:
			if (frame == 2)
				strcat(result.text, GetGameText(2, 309));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 538:
			if (frame == 3 || frame > 7 || frame < 12)
				strcat(result.text, GetGameText(2, 270));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 693:
			if (frame == 1)
				strcat(result.text, GetGameText(2, 279));
			else if (frame == 4 || frame == 5)
				strcat(result.text, GetGameText(2, 280));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 718:
			if (frame == 8)
				strcat(result.text, GetGameText(2, 281));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 717:
			if (frame == 9)
				strcat(result.text, GetGameText(2, 282));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 944:
			if (frame == 5)
				strcat(result.text, GetGameText(2, 283));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 321:
			if (frame == 11)
				strcat(result.text, GetGameText(2, 284));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 797:
			if (frame == 2 || frame == 3)
				strcat(result.text, GetGameText(2, 285));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 927:
			if (frame > 3 && frame < 8)
				strcat(result.text, GetGameText(2, 286));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 977:
			if (frame == 0 || frame == 8)
				strcat(result.text, GetGameText(2, 287));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 442:
			if (frame == 2)
				strcat(result.text, GetGameText(2, 288));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 688:
			if (frame == 2 || frame == 3)
				strcat(result.text, GetGameText(2, 289));
			else if (frame == 4 || frame == 5)
				strcat(result.text, GetGameText(2, 290));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 1016:
			if (frame == 20 || frame == 21)
				strcat(result.text, GetGameText(2, 291));
			else if (frame == 22)
				strcat(result.text, GetGameText(2, 292));
			else if (frame == 23)
				strcat(result.text, GetGameText(2, 293));
			else if (frame == 18 || frame == 19)
				strcat(result.text, GetGameText(2, 294));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 245:
			if (frame == 2)
				strcat(result.text, GetGameText(2, 295));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 998:
			if (frame < 2)
				strcat(result.text, GetGameText(2, 296));
			else if (frame < 4)
				strcat(result.text, GetGameText(2, 297));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 677:
			AppendTextEntry(name.text, frame, 2, 0x12a);
			break;
		case 673:
			if (frame > 8 && frame < 19)
				strcat(result.text, GetGameText(2, 300));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 788:
			if (frame > 5 && frame < 10)
				strcat(result.text, GetGameText(2, 275));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 1002:
			if (frame == 1)
				strcat(result.text, GetGameText(2, 301));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 890:
			if (frame == 6)
				strcat(result.text, GetGameText(2, 302));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 641:
			if (frame == 23)
				strcat(result.text, GetGameText(2, 303));
			else if (frame == 21)
				strcat(result.text, GetGameText(2, 304));
			else if (frame == 22)
				strcat(result.text, GetGameText(2, 305));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 914:
			if (frame < 2) {
				if (quality > 0 && quality < 218) {
					objref npc;
					GetNpcIbo(&npc, quality);
					char *s = GetNpcBufferForIbo(&npc)->name;
					for (int8_t i = 0; i < 20; i++)
						npcName[i] = s[i];
					FormatName(name.text, npcName);
					strcat(name.text, GetGameText(2, 350));
				} else
					strcat(result.text, GetGameText(2, 306));
			} else if (frame == 2)
				strcat(result.text, GetGameText(2, 307));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 656:
			if (frame == 1)
				strcat(result.text, GetGameText(2, 308));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 649:
			AppendTextEntry(name.text, frame, 0xe, 0x73);
			break;
		case 445:
			AppendTextEntry(name.text, frame, 8, 0x6b);
			break;
		case 296:
			AppendTextEntry(name.text, frame, 4, 0xb8);
			break;
		case 887:
			AppendTextEntry(name.text, frame, 6, 0xbc);
			break;
		case 692:
			if (frame == 2)
				strcat(result.text, GetGameText(2, 190));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 839:
			if (frame == 1)
				strcat(result.text, GetGameText(2, 191));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 810:
			if (quality == 0)
				AppendTextEntry(name.text, frame, 9, 91);
			else if (frame == 1)
				AppendTextEntry(name.text, quality - 9, 7, 100);
			else if (frame == 2)
				strcat(result.text, GetGameText(2, 100));
			else
				AppendTextEntry(name.text, frame, 9, 91);
			break;
		case 377:
			AppendTextEntry(name.text, frame, 0x20, 0x10);
			break;
		case 650:
			AppendTextChoice(name.text, frame, 1, 0x30, 0x31);
			break;
		case 675:
			AppendTextEntry(name.text, frame, 0x1c, 0x32);
			break;
		case 151:
			if (frame == 17)
				strcat(result.text, GetGameText(2, 385));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 152:
			if (frame == 22)
				strcat(result.text, GetGameText(2, 385));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 518:
			if (frame == 26)
				strcat(result.text, GetGameText(2, 386));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 475:
			if (frame == 10)
				strcat(result.text, GetGameText(2, 387));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 728:
			if (frame == 4)
				strcat(result.text, GetGameText(2, 387));
			else
				strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		case 721:
		case 989:
			strcat(result.text, GetGameText(2, 0x4e));
			break;
		default:
			strcat(name.text, GetGameText(0, TYPE(ITEM(item->off))));
			break;
		}
	}
	if (!name.empty()) {
		if (!name.has('/'))
			strncpy(base.text, name.text, 80);
		else {
			name.pos = 0;
			if (ScanToDelimiter(&name, article.text, "/") && ScanToDelimiter(&name, base.text, "/")) {
				ScanToDelimiter(&name, single.text, "/");
				ScanToDelimiter(&name, plural.text, "/");
			}
		}
		if (quantity > 1)
			FormatName(result.text, "%d %s%s", quantity, base.text, plural.text);
		else
			FormatName(result.text, "%s %s%s", article.text, base.text, single.text);
	}
	strncpy(out, result.text, 80);
	if ((int16_t) strlen(result.text) >= 80)
		out[79] = 0;
}

extern "C" void ResetLookGlobals(void)
{
	DescriptFileName = "DESCRIPT.FLX";
}
