/* Serpent Isle SI.EXE, resident segment 100 (file offsets 0x036ad2 to 0x036b5a, 136 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "npcref.h"
#include "ucvalue.h"

Node::~Node()
{
}

/* the item a usecode number stands for: UC_AVATAR for the avatar, other negative numbers for NPCs */
int GetItemRef(Node *ref)
{
	int npc;
	int item;

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
