/* Black Gate U7.EXE, overlay segment 260 (file offsets 0x07b100 to 0x07b511, 1041 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "objref.h"
#include "bogus.h"
#include "npcref.h"
#include "item.h"
#include "type.h"
#include "spell.h"

#define SUB(off) ((struct SpellbookRecord *) ItemAt((off)))
#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define TYPE_CLASS(rec) (gItemTypeInfo[TYPE(rec)].typeClass)
#define IS_CLASS(rec, c) ((int8_t) (TYPE_CLASS(rec) == (c)))
#define IS_VALID(r) ((int8_t) ((r) != 0))

#define SPELLBOOK   8   /* type class */
#define REAGENT     842 /* type; the frame is the reagent */

/* A spellbook's spell record: one bit per spell, eight to a circle. The first five circles are in
 * the book's own record, the rest in the record chained after it. */
struct SpellbookRecord {
	uint8_t bits[7];
	int8_t bookmark;
};

/* The reagents each spell needs, one bit per reagent. */
extern const uint8_t SpellReagents[72] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x38, 0x30, 0x44, 0x50, 0xc0, 0x80, 0x04, 0x30,
	0x82, 0x09, 0x81, 0x88, 0x38, 0xb0, 0x0b, 0xcf,
	0x94, 0x70, 0x0e, 0xb8, 0x44, 0x0c, 0x07, 0x45,
	0x48, 0x89, 0x0b, 0x9c, 0x0b, 0x82, 0xce, 0x82,
	0x45, 0x1a, 0xd1, 0x8b, 0x78, 0x06, 0xc1, 0x64,
	0x1c, 0xee, 0xc9, 0x83, 0x8e, 0x45, 0x61, 0x8a,
	0x48, 0x85, 0xcb, 0xc9, 0x8e, 0x4d, 0x29, 0x78,
	0xff, 0x8e, 0x3e, 0x0f, 0xf0, 0x1a, 0x0d, 0x1a
};
uint8_t ReagentCounts[8] = { 0 };
extern uint8_t SpellCastCounts[];

ReagentCounter::ReagentCounter()
{
}

/* Counts the reagents the NPC carries (none unless it is the avatar), then how often each spell can
 * be cast. */
void ReagentCounter::countReagentsInPossession(NPCRef npc)
{
	int16_t i = 0;
	int16_t spell = 0;
	int8_t have = 0;

	if (IS_VALID(npc.off) && (int8_t)Item_isAvatar((objref *)&npc.off))
		have = 1;
	for (i = 0; i < 8; i++)
		ReagentCounts[i] = have ? CountHeldItems(0, npc.off, REAGENT, 255, i) : 0;
	for (spell = 0; spell < 72; spell++)
		SpellCastCounts[spell] = getCastCount(spell);
}

/* How often a spell can be cast: the smallest count among the reagents it needs. */
int16_t ReagentCounter::getCastCount(int16_t spell)
{
	uint8_t need = SpellReagents[spell];
	int16_t count = 255;
	int16_t i;

	for (i = 0; i < 8; i++)
		if ((need >> i) % 2 != 0 && ReagentCounts[i] < count)
			count = ReagentCounts[i];
	return count;
}

uint8_t DoesSpellbookHaveSpell(objref *book, uint8_t spell)
{
	uint8_t bits = 0;
	int16_t circle = spell >> 3;
	int16_t record;
	int16_t innerRecord;

	if (IS_CLASS(ITEM(book->off), SPELLBOOK)) {
		record = ITEM(book->off)->data.extra;
		if (circle < 5)
			bits = SUB(record)->bits[circle];
		else if (circle < 12) {
			innerRecord = ITEM(record)->data.extra;
			bits = SUB(innerRecord)->bits[circle - 5];
		}
	}
	return bits & (1 << (spell & 7));
}

void AddSpellToSpellbook(objref *book, uint8_t spell)
{
	uint8_t bit = 1 << (spell & 7);
	int16_t circle = spell >> 3;
	int16_t record;
	int16_t innerRecord;

	if (IS_CLASS(ITEM(book->off), SPELLBOOK)) {
		record = ITEM(book->off)->data.extra;
		if (circle < 5)
			SUB(record)->bits[circle] |= bit;
		else if (circle < 12) {
			innerRecord = ITEM(record)->data.extra;
			SUB(innerRecord)->bits[circle - 5] |= bit;
		}
	}
}

void RemoveSpellFromSpellbook(int16_t *book, uint8_t spell)
{
	uint8_t bit = 1 << (spell & 7);
	int16_t circle = spell >> 3;
	int16_t record;
	int16_t innerRecord;

	if (IS_CLASS(ITEM(*book), SPELLBOOK)) {
		record = ITEM(*book)->data.extra;
		if (circle < 5)
			SUB(record)->bits[circle] &= ~bit;
		else if (circle < 12) {
			innerRecord = ITEM(record)->data.extra;
			SUB(innerRecord)->bits[circle - 5] &= ~bit;
		}
	}
}

/* Forgets every spell. */
void ClearSpellbook(int16_t *book)
{
	int16_t i;
	int16_t record;
	int16_t innerRecord;

	if (IS_CLASS(ITEM(*book), SPELLBOOK)) {
		record = ITEM(*book)->data.extra;
		for (i = 0; i < 5; i++)
			SUB(record)->bits[i] = 0;
		innerRecord = ITEM(record)->data.extra;
		for (i = 0; i < 7; i++)
			SUB(innerRecord)->bits[i] = 0;
	}
}

int8_t GetSpellbookBookmark(objref *book)
{
	int8_t bookmark = -1;
	int16_t record;
	int16_t innerRecord;

	if (IS_CLASS(ITEM(book->off), SPELLBOOK)) {
		record = ITEM(book->off)->data.extra;
		innerRecord = ITEM(record)->data.extra;
		bookmark = SUB(innerRecord)->bookmark;
	}
	return bookmark;
}

void SetSpellbookBookmark(NPCRef book, int8_t bookmark)
{
	int16_t record;
	int16_t innerRecord;

	if (IS_CLASS(ITEM(book.off), SPELLBOOK)) {
		record = ITEM(book.off)->data.extra;
		innerRecord = ITEM(record)->data.extra;
		SUB(innerRecord)->bookmark = bookmark;
	}
}

extern "C" void ResetSpellGlobals(void)
{
	memset(ReagentCounts, 0, sizeof(ReagentCounts));
}
