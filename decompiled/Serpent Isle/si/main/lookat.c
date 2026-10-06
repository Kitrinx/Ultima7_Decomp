/* Serpent Isle SI.EXE, overlay segment 228 (file offsets 0x0610e0 to 0x0611a9, 201 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 */

#include <string.h>
#include "u7event.h"
#include "item.h"
#include "type.h"
#include "gumpmgr.h"
#include "sprite.h"
#include "look.h"

#define MK_FP(seg, ofs) ((void _seg *) (seg) + (void near *) (ofs))
#define ITEM(off) ((struct ItemRecord far *) MK_FP(ItemBufferSegment, (off)))
#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define TYPE_CLASS(rec) (gItemTypeInfo[TYPE(rec)].typeClass)
#define IS_VALID(r) ((char) ((r) != 0))

/* a line of text read in pieces from pos */
struct LookScanner {
	char text[80];
	int pos;
	LookScanner() { text[0] = 0; pos = 0; }
};

/* bark the name of the item PickWorldItem returns */
void far LookAtItem(void)
{
	int obj;
	unsigned char quantity;
	int type;
	unsigned char counted;

	if (!PickWorldItem((objref *)&obj)) {
		MouseState unusedState;

		if (IS_VALID(obj)) {
			type = TYPE(ITEM(obj));
			counted = TYPE_CLASS(ITEM(obj)) == TYPE_CLASS_QUANTITY;
			if (!counted)
				quantity = 0;
			else
				quantity = Item_getQuantity((objref *)&obj);
			LookScanner name;
			ProduceItemDisplayName(name.text, &objref(obj), quantity);
			SpriteManager_barkOnItem(&gSpriteManager, obj, name.text, 0, 15, 1);
		}
	}
}
