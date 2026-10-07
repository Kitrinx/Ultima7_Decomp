/* Serpent Isle SI.EXE, overlay segment 228 (file offsets 0x0610e0 to 0x0611a9, 201 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <string.h>
#include "u7event.h"
#include "item.h"
#include "type.h"
#include "gumpmgr.h"
#include "sprite.h"
#include "look.h"

#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define TYPE_CLASS(rec) (gItemTypeInfo[TYPE(rec)].typeClass)
#define IS_VALID(r) ((int8_t) ((r) != 0))

/* a line of text read in pieces from pos */
struct LookScanner {
	char text[80];
	int16_t pos;
	LookScanner() { text[0] = 0; pos = 0; }
};

/* bark the name of the item PickWorldItem returns */
void LookAtItem(void)
{
	/* left unset when nothing is picked; DOS usually found InGumpMode's pushed &GameInput here */
	int16_t obj = 0x6252;
	uint8_t quantity;
	int16_t type;
	uint8_t counted;

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
