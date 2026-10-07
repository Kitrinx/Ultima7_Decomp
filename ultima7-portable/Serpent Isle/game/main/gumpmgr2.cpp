/* Serpent Isle SI.EXE, overlay segment 327 (file offsets 0x0941e0 to 0x094bac, 2508 bytes).
 * Borland C++ 2.0 -mm -O -P -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "u7manage.h"
#include "item.h"
#include "npcref.h"
#include "coord.h"
#include "colbuf.h"
#include "gumps.h"
#include "spellbk.h"
#include "dosio.h"
#include "easyfile.h"
#include "gumpmgr.h"
#include "worldgmp.h"
#include "itable.h"
#include "datanode.h"
#include "camera.h"
#include "cheat.h"
#include "oops.h"
#include "mouse.h"
#include "bogus.h"
#include "itemrec.h"
#include "combat.h"
#include "crime.h"
#include "palctrl.h"
#include "u7point.h"
#include "use.h"
#include "u7npc.h"
#include "u7event.h"
#include "debug.h"
#include "cast.h"
#include "itemcmd.h"
#include "itemovr1.h"
#include "look.h"
#include "text.h"
#include "palfade.h"
#include "partymov.h"
#include "sortitem.h"
#include "sounds.h"
#include "spell.h"
#include "trigger.h"
#include "usehook.h"
#include "voice.h"
#include "type.h"
#include "preload.h"
#include "systimer.h"
#include "iteminfo.h"
#include "collide.h"
#include "u7map.h"

#define SHAPE_JAWBONE       555
#define SHAPE_SPELL_SCROLL  715
#define SHAPE_KEY           641

uint8_t AddKeyToKeyring(NPCRef owner, objref key);

#define TYPE(p) ((p)->typeFrame & 0x3ff)
#define TYPE_CLASS(p) (gItemTypeInfo[TYPE(p)].typeClass)
#define IS_CLASS(p, f) ((int8_t) (TYPE_CLASS(p) == (f)))

uint8_t DisplayItemDialog(objref obj, uint8_t stats)
{
	GumpNode *node = 0;
	InventoryGump *doll;
	JawboneGump *jawbone;
	ContainerGump *container;
	StatsGump *statsGump;
	Spellbook *book;
	CombatStatsGump *combat;
	SpellScrollGump *scroll;
	Gump *dialog;

	if (obj.valid() && !Item_canBeOpened(obj))
		return 0;
	if (IsNpcUnconscious(&AvatarRef) && !stats)
		return 0;
	int16_t n;

	while (List_stepForward(&OpenItemDialogsList, (DoubleLink **) &node)) {
		if (node->dialog->object() == obj
			&& ((!node->dialog->stats && !stats) || (node->dialog->stats && stats))) {
			OpenItemDialogsList.bringToFront(node);
			node->dialog->Control::hide();
			RedrawDialogs();
			node->dialog->Control::show();
			return 0;
		}
	}
	if (stats) {
		statsGump = new StatsGump(obj);
		if (!statsGump)
			return 0;
		dialog = statsGump;
		n = GetPartyMemberIndex(&NPCRef(obj));
		if (n > 8)
			n = 7;
		dialog->moveTo(PaperdollPositions[n]->x + 3, PaperdollPositions[n]->y + 3);
	} else if (!obj.valid()) {
		combat = new CombatStatsGump;
		if (!combat)
			return 0;
		dialog = combat;
	} else if (IS_CLASS(ITEM(obj.off), TYPE_CLASS_HUMAN)) {
		doll = new InventoryGump(NPCRef(obj));
		if (!doll)
			return 0;
		dialog = doll;
		n = GetPartyMemberIndex(&NPCRef(obj));
		if (n > 8)
			n = 7;
		dialog->moveTo(PaperdollPositions[n]->x, PaperdollPositions[n]->y);
	} else if (IS_CLASS(ITEM(obj.off), TYPE_CLASS_SPELLBOOK)) {
		book = new Spellbook(obj);
		if (!book)
			return 0;
		dialog = book;
	} else if (TYPE(ITEM(obj.off)) == SHAPE_JAWBONE) {
		jawbone = new JawboneGump(obj);
		if (!jawbone)
			return 0;
		dialog = jawbone;
	} else if (TYPE(ITEM(obj.off)) == SHAPE_SPELL_SCROLL) {
		scroll = new SpellScrollGump(obj);
		if (!scroll)
			return 0;
		dialog = scroll;
	} else {
		container = new ContainerGump(obj);
		if (!container)
			return 0;
		dialog = container;
	}
	OpenItemDialogsList.prepend(dialog);
	PlaySoundSimple(94);
	RedrawDialogs();
	return 1;
}

void ReturnDraggedItem(Panel *from, int16_t quantity)
{
	int16_t x, y;
	uint8_t mover;

	if (!quantity)
		return;
	x = from->mouseX();
	y = from->mouseY();
	if (from == WorldArea) {
		x += WorldArea->dragX();
		y += WorldArea->dragY();
	}
	mover = HackMoverEnabled;
	HackMoverEnabled = 1;
	if (!from->accepts(from->selected(), x, y)) {
		PlaceItem(&from->selected(), Item_getX(AvatarRef), Item_getY(AvatarRef), Item_getZ(&AvatarRef));
	}
	HackMoverEnabled = mover;
	if (quantity > -1) {
		Item_setQuantity(from->selected(), quantity, 0);
	}
}

void DropDraggedItem(objref obj, Panel *from, Panel *to, uint8_t quantity, uint8_t gump)
{
	uint8_t done = 0;
	int16_t mx, my, dx, dy;
	Loc x, y;

	from->dirty = 1;
	to->dirty = 1;
	mx = MouseState_getX(GetLastMouseState());
	my = GetLastMouseState()->y;
	dx = from->dragX();
	dy = from->dragY();
	to->setDragX(dx);
	to->setDragY(dy);
	x = Item_getX(obj);
	y = Item_getY(obj);
	if (quantity && (uint8_t)Item_getQuantity(&obj) > 1 && (from != to || !gump)) {
		objref dragged;
		int16_t count;
		int16_t total;

		total = (uint8_t)Item_getQuantity(&obj);
		CloneItem(&obj);
		GetItemBeingDragged(&dragged);
		count = RunSliderGump(0, total, 1, total, -1, -1);
		Item_setQuantity(dragged, count, 0);
		if (count) {
			Item_setQuantity(obj, total - count, 0);
			if (!to->accepts(dragged, mx, my)) {
				count = 0;
				ZapDetachedItem(&dragged);
			} else {
				if (to == WorldArea) {
					done = TriggerEggsUnderItem(dragged, Item_getX(dragged), Item_getY(dragged));
				}
				if (from == WorldArea && !done) {
					TriggerEggsUnderItem(dragged, x, y);
				}
				if (from == WorldArea) {
					SettleChunkItems(x, y);
					Item_setQualityFlags(&dragged, Item_getQualityFlags(&dragged) & 0xef);
				}
			}
		}
		ReturnDraggedItem(from, total - count);
	} else {
		objref owner = to->object();

		while (IsContained(&owner))
			owner = Item_getContainer(&owner);
		if (IS_CLASS(ITEM(owner.off), TYPE_CLASS_HUMAN) && TYPE(ITEM(obj.off)) == SHAPE_KEY) {
			if (AddKeyToKeyring(NPCRef(owner), obj))
				return;
		}
		if (to->accepts(obj, mx, my)) {
			if (to == WorldArea) {
				done = TriggerEggsUnderItem(obj, Item_getX(obj), Item_getY(obj));
			}
			if (from == WorldArea && !done) {
				done = TriggerEggsUnderItem(obj, x, y);
			}
			if (from == WorldArea) {
				SettleChunkItems(x, y);
			}
		} else
			ReturnDraggedItem(from, -1);
	}
	if (done)
		DialogState = DIALOG_DONE;
	PlaySoundSimple(134);
}

void DragItem(Panel *from, MouseState *state)
{
	uint8_t result = 0;
	uint8_t gump;
	Panel *to = 0;
	objref obj = from->selected();
	GumpNode *node;
	uint8_t quantity;

	Item_detach(&obj);
	{
		ItemDrag drag(obj, MouseState_getX(state), state->y, from->dragX(), from->dragY());

		drag.Control::hide();
		RedrawDialogs();
		drag.Control::show();
		drag.drag(state);
	}
	node = 0;
	quantity = IS_CLASS(ITEM(obj.off), TYPE_CLASS_QUANTITY);
	while (List_stepForward(&OpenItemDialogsList, (DoubleLink **) &node)) {
		result = node->dialog->handle(GetLastMouseState());
		if (result == GUMP_NO_DROP) {
			if (quantity) {
				ReturnDraggedItem(from, (uint8_t)Item_getQuantity(&obj));
				return;
			}
			ReturnDraggedItem(from, -1);
			return;
		}
		if (result == GUMP_DROP_HERE) {
			to = node->dialog;
			gump = 1;
			break;
		}
	}
	if (!to) {
		to = WorldArea;
		gump = 0;
	}
	DropDraggedItem(obj, from, to, quantity, gump);
}
