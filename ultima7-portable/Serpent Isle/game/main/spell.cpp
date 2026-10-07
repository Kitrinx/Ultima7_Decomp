/* Serpent Isle SI.EXE, overlay segment 241 (file offsets 0x069160 to 0x06955d, 1021 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Filename and folder inferred.
 * Counter writes begin one byte before SpellCastCounts.
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
extern const uint16_t SpellReagents[72] = {
	0x038, 0x030, 0x044, 0x050, 0x0c0, 0x080, 0x004, 0x00b,
	0x030, 0x082, 0x084, 0x481, 0x088, 0x070, 0x038, 0x0b0,
	0x430, 0x094, 0x009, 0x011, 0x0b8, 0x044, 0x045, 0x0c9,
	0x00a, 0x050, 0x088, 0x2da, 0x09c, 0x082, 0x041, 0x082,
	0x048, 0x0d1, 0x08b, 0x078, 0x006, 0x064, 0x0ce, 0x099,
	0x045, 0x01c, 0x01c, 0x4c1, 0x4c9, 0x483, 0x0a2, 0x2c8,
	0x0c9, 0x08e, 0x030, 0x029, 0x086, 0x078, 0x04b, 0x089,
	0x440, 0x185, 0x0cb, 0x00b, 0x652, 0x653, 0x098, 0x10d,
	0x08e, 0x13e, 0x00f, 0x187, 0x0d9, 0x05a, 0x01a, 0x784
};
uint8_t ReagentCounts[11] = { 0 };
extern uint8_t SpellCastCounts[];
extern uint8_t SpellCastCountPad;

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
	for (i = 0; i < 11; i++)
		ReagentCounts[i] = have ? CountHeldItems(0, npc.off, REAGENT, 255, i) : 0;
	for (spell = 0; spell < 72; spell++)
		*(spell != 0 ? &SpellCastCounts[spell - 1] : &SpellCastCountPad) = getCastCount(spell);
}

/* How often a spell can be cast: the smallest count among the reagents it needs. */
int16_t ReagentCounter::getCastCount(int16_t spell)
{
	uint16_t need = SpellReagents[spell];
	int16_t count = 255;
	int16_t i;

	for (i = 0; i < 11; i++)
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
