/* Black Gate U7.EXE, resident segment 115 (file offsets 0x03969e to 0x039726, 136 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
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
