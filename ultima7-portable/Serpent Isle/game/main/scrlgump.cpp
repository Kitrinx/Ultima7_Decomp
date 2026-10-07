/* Serpent Isle SI.EXE, resident segment 106 (file offsets 0x038238 to 0x038400, 456 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "item.h"
#include "colbuf.h"
#include "gumps.h"

SpellScrollGump::SpellScrollGump(objref item)
	: picture(0)
{
	displayed = item;
	initialize();
}
