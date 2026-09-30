#ifndef SPELL_H
#define SPELL_H

#include "npcref.h"

struct objref;

/* How many of each reagent an NPC carries, and so how often each spell can be cast. */
class ReagentCounter {
	int16_t unusedField1;
public:
	ReagentCounter();
	void countReagentsInPossession(NPCRef npc);
	int16_t getCastCount(int16_t spell);
};

void SetSpellbookBookmark(NPCRef book, int8_t bookmark);
uint8_t DoesSpellbookHaveSpell(objref *book, uint8_t spell);
void AddSpellToSpellbook(objref *book, uint8_t spell);
void RemoveSpellFromSpellbook(int16_t *book, uint8_t spell);
void ClearSpellbook(int16_t *book);
int8_t GetSpellbookBookmark(objref *book);

extern uint8_t ReagentCounts[8];

#endif
