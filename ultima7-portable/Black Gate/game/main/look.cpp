/* Black Gate U7.EXE, overlay segment 243 (file offsets 0x06fbf0 to 0x07005b, 1131 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "dosio.h"
#include "sprite.h"
#include "text.h"
#include "u7event.h"
#include "item.h"
#include "type.h"
#include "gumpmgr.h"

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
#define IS_VALID(r) ((int8_t) ((r) != 0))

char *DescriptFileName = "DESCRIPT.FLX";

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

/* The name of a type with frame and quantity, at most 79 characters. Names
 * read "article/base/singular/plural"; one without '/' is all base. */
void ProduceItemDisplayName(char *out, uint16_t type, uint8_t quantity, int16_t frame)
{
	LookScanner name, base, result, article, single, plural;

	switch (type) {
	case 842:   /* reagent */
		AppendTextEntry(name.text, frame, 8, 0);
		break;
	case 955:   /* amulet */
		AppendTextEntry(name.text, frame, 3, 8);
		break;
	case 377:   /* food item */
		AppendTextEntry(name.text, frame, 32, 11);
		break;
	case 650:   /* sextant */
		AppendTextChoice(name.text, frame, 1, 43, 44);
		break;
	case 675:   /* desk item */
		AppendTextEntry(name.text, frame, 21, 45);
		break;
	case 721:   /* Avatar */
	case 989:   /* Avatar */
		strcat(result.text, GetGameText(2, 66));
		break;
	default:
		strcat(name.text, GetGameText(0, type));
		break;
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

/* bark the name of the item PickWorldItem returns */
void LookAtItem(void)
{
	int16_t obj;
	uint8_t quantity;
	int16_t type, frame;
	uint8_t counted;

	if (!PickWorldItem((objref *)&obj)) {
		MouseState unusedState;

		unusedState = *GetLastMouseState();
		if (IS_VALID(obj)) {
			type = TYPE(ITEM(obj));
			frame = FRAME(ITEM(obj));
			counted = TYPE_CLASS(ITEM(obj)) == TYPE_CLASS_QUANTITY;
			if (!counted)
				quantity = 0;
			else
				quantity = Item_getQuantity((objref *)&obj);
			LookScanner name;
			ProduceItemDisplayName(name.text, type, quantity, frame);
			SpriteManager_barkOnItem(&gSpriteManager, obj, name.text, 0, 15, 1);
		}
	}
}

extern "C" void ResetLookGlobals(void)
{
	DescriptFileName = "DESCRIPT.FLX";
}
