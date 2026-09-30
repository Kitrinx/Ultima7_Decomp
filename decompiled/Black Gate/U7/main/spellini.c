/* Black Gate U7.EXE, resident segment 117 (file offsets 0x03a278 to 0x03a5d4, 860 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "objref.h"
#include "colbuf.h"
#include "gumps.h"
#include "spellbk.h"
#include "itable.h"

Spellbook::Spellbook(objref item) : bookmark(42), previous(44), next(45)
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
