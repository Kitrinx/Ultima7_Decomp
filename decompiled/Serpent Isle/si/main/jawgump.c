/* Serpent Isle SI.EXE, resident segment 105 (file offsets 0x037f51 to 0x038238, 743 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "item.h"
#include "colbuf.h"
#include "gumps.h"

objref ToothSlot::object() { return tooth; }

JawboneGump::JawboneGump(objref item)
{
	displayed = item;
	initialize();
}

JawboneGump::~JawboneGump() {}
