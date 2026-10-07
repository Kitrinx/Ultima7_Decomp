/* Serpent Isle SI.EXE, resident segment 100 (file offsets 0x036ad2 to 0x036b5a, 136 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "npcref.h"
#include "ucvalue.h"

Node::~Node()
{
}

/* the item a usecode number stands for: UC_AVATAR for the avatar, other negative numbers for NPCs */
int16_t GetItemRef(Node *ref)
{
	int16_t npc;
	int16_t item;

	item = ref->toInt();
	if (item == UC_AVATAR)
		return AvatarRef.off;
	if (item < 0 && item > UC_AVATAR) {
		item = -item;
		GetNpcIbo((objref *)&npc, item);
		return npc;
	}
	return item;
}
