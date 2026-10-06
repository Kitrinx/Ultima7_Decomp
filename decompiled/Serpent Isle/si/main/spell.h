#ifndef SPELL_H
#define SPELL_H

#include "npcref.h"

struct objref;

/* How many of each reagent an NPC carries, and so how often each spell can be cast. */
class ReagentCounter {
	int unusedField1;
public:
	ReagentCounter();
	void countReagentsInPossession(NPCRef npc);
	int getCastCount(int spell);
};

void SetSpellbookBookmark(NPCRef book, char bookmark);
unsigned char DoesSpellbookHaveSpell(objref *book, unsigned char spell);
void AddSpellToSpellbook(objref *book, unsigned char spell);
void RemoveSpellFromSpellbook(int *book, unsigned char spell);
void ClearSpellbook(int *book);
char GetSpellbookBookmark(objref *book);

extern unsigned char ReagentCounts[11];

#endif
