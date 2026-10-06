/* Serpent Isle SI.EXE, resident segment 104 (file offsets 0x037bf5 to 0x037f51, 860 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "objref.h"
#include "colbuf.h"
#include "gumps.h"
#include "spellbk.h"
#include "itable.h"

Spellbook::Spellbook(objref item) : bookmark(37), previous(39), next(40)
{
	initialize(item);
}

Spellbook::~Spellbook()
{
	SetSpellbookBookmark(displayed, selectedSpell);
	for (int i = 0; i < 8; i++)
		if (spells[i])
			delete spells[i];
}

unsigned char Spellbook::findPosition(objref, int *, int *)
{
	return 0;
}

ProportionalTextPrinter SpellbookTextPrinter;
