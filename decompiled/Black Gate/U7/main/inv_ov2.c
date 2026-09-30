/* Black Gate U7.EXE, overlay segment 342 (file offsets 0x0a26c0 to 0x0a48f2, 8754 bytes).
 * Borland C++ 2.0 -mm -O -P -d -Y rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include <dos.h>
#include <stdio.h>
#include <string.h>
#include "lowlevel.h"
#include "u7manage.h"
#include "item.h"
#include "u7npc.h"
#include "u7event.h"
#include "colbuf.h"
#include "gumpmgr.h"
#include "bltshape.h"
#include "itable.h"
#include "bogus.h"
#include "combat.h"
#include "death.h"
#include "equip.h"
#include "makemojo.h"
#include "text.h"
#include "use.h"
#include "usehook.h"
#include "weight.h"
#include "wihh.h"
#include "type.h"
#include "npcref.h"
#include "voolook.h"
#include "ready.h"

#define TYPE(p) ((p)->typeFrame & 0x3ff)
#define FRAME(p) (((p)->typeFrame & 0x7c00) >> 10)
#define TYPE_CLASS(p) (gItemTypeInfo[TYPE(p)].typeClass)
#define CLASS_FLAGS(p) (ItemTypeClassFlags[TYPE_CLASS(p)])

ProportionalTextPrinter InventoryTextPrinter;

inline objref Gump::object() { return displayed; }

struct PointOffset { int x, y; };

/* where each equipment slot sits on the paperdoll */
PointOffset far PaperdollSlotOffsets[] = {
	{ 116, 24 },
	{ 116, 55 },
	{ 36, 55 },
	{ 116, 37 },
	{ 36, 24 },
	{ 36, 37 },
	{ 116, 70 },
	{ 36, 70 },
	{ 36, 10 },
	{ 116, 10 },
	{ 116, 85 },
	{ 77, 97 },
};

void StatsGump::initialize()
{
	int height, width;

	shape = 1406;
	gShapeManager.getShapeSize(&width, &height, shape);
	bounds.set(0, 0, width, height);
	add(&closeButton);
	closeButton.show();
	displayed = displayedItem;
	stats = 1;
	Control::show();
	StatsTextPrinter.setFont(2);
	add(&asleepIcon);
	asleepIcon.setFrame(0);
	add(&poisonedIcon);
	poisonedIcon.setFrame(1);
	add(&charmedIcon);
	charmedIcon.setFrame(2);
	add(&hungryIcon);
	hungryIcon.setFrame(3);
	add(&protectedIcon);
	protectedIcon.setFrame(4);
	add(&cursedIcon);
	cursedIcon.setFrame(5);
	add(&paralyzedIcon);
	paralyzedIcon.setFrame(6);
	update();
}

void StatsGump::update()
{
	asleepIcon.unlock();
	if (NPCRef(Gump::object()).flag(NPC_ASLEEP))
		asleepIcon.show();
	else
		asleepIcon.hide();
	asleepIcon.lock();
	poisonedIcon.unlock();
	if (NPCRef(Gump::object()).flag(NPC_POISONED))
		poisonedIcon.show();
	else
		poisonedIcon.hide();
	poisonedIcon.lock();
	charmedIcon.unlock();
	if (NPCRef(Gump::object()).flag(NPC_CHARMED))
		charmedIcon.show();
	else
		charmedIcon.hide();
	charmedIcon.lock();
	hungryIcon.unlock();
	if (IsFamished(&NPCRef(Gump::object())) || IsHungry(&NPCRef(Gump::object())) || IsStarving(&NPCRef(Gump::object())))
		hungryIcon.show();
	else
		hungryIcon.hide();
	hungryIcon.lock();
	protectedIcon.unlock();
	if (NPCRef(Gump::object()).flag(NPC_PROTECTED))
		protectedIcon.show();
	else
		protectedIcon.hide();
	protectedIcon.lock();
	cursedIcon.unlock();
	if (NPCRef(Gump::object()).flag(NPC_CURSED))
		cursedIcon.show();
	else
		cursedIcon.hide();
	cursedIcon.lock();
	paralyzedIcon.unlock();
	if (NPCRef(Gump::object()).flag(NPC_PARALYZED))
		paralyzedIcon.show();
	else
		paralyzedIcon.hide();
	paralyzedIcon.lock();
}

unsigned char StatsGump::handle(MouseState *event)
{
	unsigned char result = 0;

	if (!isVisible())
		return 0;
	int x = MouseState_getX(event);
	int y = event->y;
	if (!bounds.contains(x, y))
		return 0;
	int hit = gShapeManager.isCursorInBounds(shape, 0, bounds, Point(x, y));
	if (!hit)
		return 0;
	if ((result = closeButton.handle(event)) != 0) {
		switch (result) {
		case BUTTON_CLICKED: return GUMP_CLOSE;
		default: return result;
		}
	}
	if (event->action == MOUSE_CLICK)
		return GUMP_MOVE;
	if (event->action == MOUSE_RELEASE)
		return GUMP_NO_DROP;
	return 0;
}

void StatsGump::draw(View *target)
{
	if (isVisible()) {
		int x = bounds.x;
		int y = bounds.y;
		ShapeManager_draw(&gShapeManager, target, x, y, shape, 0, 0, 0);
		View *saved = StatsTextPrinter.target;
		StatsTextPrinter.target = target;
		char name[20], text[20];
		_fstrncpy(name, NPCRef(Gump::object()).buffer()->name, 19);
		name[19] = 0;
		int halfWidth = StatsTextPrinter.textWidth(name) >> 1;
		int center = 47;
		int nameX = center - halfWidth + 28;
		StatsTextPrinter.x = x + nameX;
		StatsTextPrinter.y = y + 12;
		StatsTextPrinter.printString(name);
		int i, textX, textY;
		long experience, level;
		experience = NPCRef(Gump::object()).buffer()->experience;
		level = Npc_getLevel(&NPCRef(Gump::object()));
		for (i = 0; i <= 9; i++) {
			switch (i) {
			case 0:
				textY = 23;
				sprintf(text, "%2d", NPCRef(Gump::object()).strength());
				break;
			case 1:
				textY = 32;
				sprintf(text, "%2d", NPCRef(Gump::object()).dexterity());
				break;
			case 2:
				textY = 41;
				sprintf(text, "%2d", NPCRef(Gump::object()).intelligence());
				break;
			case 7:
				textY = 52;
				sprintf(text, "%2d", NPCRef(Gump::object()).buffer()->combat);
				break;
			case 9:
				textY = 61;
				if (NPCRef(Gump::object()).isAvatar()) {
					sprintf(text, "%2d", NPCRef(Gump::object()).buffer()->magic);
				} else {
					sprintf(text, "n/a");
				}
				break;
			case 6:
				textY = 72;
				sprintf(text, "%2d", (signed char)Item_getHitPoints(&NPCRef(Gump::object())));
				break;
			case 8:
				textY = 81;
				if (NPCRef(Gump::object()).isAvatar()) {
					sprintf(text, "%2d", NPCRef(Gump::object()).buffer()->mana);
				} else {
					sprintf(text, "n/a");
				}
				break;
			case 4:
				textY = 92;
				sprintf(text, "%6ld", experience);
				break;
			case 5:
				textY = 101;
				sprintf(text, "%2d", level);
				break;
			case 3:
				textY = 110;
				sprintf(text, "%2d", NPCRef(Gump::object()).buffer()->training);
				break;
			}
			textX = 124 - StatsTextPrinter.textWidth(text);
			StatsTextPrinter.x = x + textX;
			StatsTextPrinter.y = y + textY;
			StatsTextPrinter.printString(text);
		}
		StatsTextPrinter.target = saved;
	}
}

void StatsGump::moveTo(int x, int y)
{
	bounds.moveTo(x, y);
	closeButton.moveTo(x + 23, y + 124);
	asleepIcon.moveTo(x + 42, y + 130);
	poisonedIcon.moveTo(x + 54, y + 130);
	charmedIcon.moveTo(x + 68, y + 130);
	hungryIcon.moveTo(x + 83, y + 130);
	protectedIcon.moveTo(x + 97, y + 130);
	cursedIcon.moveTo(x + 110, y + 130);
	paralyzedIcon.moveTo(x + 125, y + 130);
}

unsigned char StatsGump::accepts(objref, int, int) { return 0; }

objref StatsGump::selected()
{
	objref result = 0;

	return result;
}

int StatsGump::mouseX() { return clickX; }

int StatsGump::mouseY() { return clickY; }

int StatsGump::dragX() { return dragOffsetX; }

int StatsGump::dragY() { return dragOffsetY; }

void StatsGump::setDragX(int x) { dragOffsetX = x; }

void StatsGump::setDragY(int y) { dragOffsetY = y; }

void StatsGump::refresh(char force)
{
	if (dirty || force)
		update();
}

unsigned char StatsGump::findPosition(objref, int *, int *) { return 0; }

void ItemSlot::initialize(objref item, unsigned char isMale)
{
	carried = item;
	unusedMale = isMale;
}

unsigned char ItemSlot::handle(MouseState *event)
{
	unsigned char result = 0;
	int mouseX, mouseY;
	unsigned char hit;

	if (!visible)
		return 0;
	mouseX = MouseState_getX(event);
	mouseY = event->y;
	hit = 0;
	if (!carried.valid() && event->action != MOUSE_RELEASE)
		return 0;
	if (carried.valid()) {
		int height, width;
		gShapeManager.getFrameSize(&width, &height,
			TYPE(ITEM(carried.off)), FRAME(ITEM(carried.off)));
		hit = gShapeManager.isCursorInBounds(TYPE(ITEM(carried.off)), FRAME(ITEM(carried.off)),
			Point(x + (width >> 1), y + (height >> 1)), Point(mouseX, mouseY));
	}
	if (hit && event->action == MOUSE_CLICK && PickingItem)
		return GUMP_SELECT_ITEM;
	if (event->action == MOUSE_DOUBLE_CLICK) {
		if (hit) {
			if (Item_canBeOpened(carried))
				result = GUMP_OPEN_ITEM;
			else
				result = GUMP_USE_ITEM;
			return result;
		}
	} else if (event->action == MOUSE_CLICK) {
		if (hit) {
			if (WaitForClick(*event))
				return GUMP_SELECT_ITEM;
			Item_unequip(&carried);
			return GUMP_DRAG_ITEM;
		}
		return result;
	}
	return 0;
}

void ItemSlot::moveTo(int newX, int newY)
{
	x = newX;
	y = newY;
}

void ItemSlot::draw(View *target)
{
	int height, width;

	if (visible && carried.valid()) {
		gShapeManager.getFrameSize(&width, &height,
			TYPE(ITEM(carried.off)), FRAME(ITEM(carried.off)));
		ShapeManager_drawItem(&gShapeManager, x + (width >> 1), y + (height >> 1), carried, target);
	}
}

unsigned char ItemSlot::accept(objref item)
{
	carried = item;
	return 1;
}

void InventoryGump::initialize()
{
	unsigned char male = Npc_isMale(&NPCRef(Gump::object()));
	int number = NPCRef(Gump::object()).number();

	if (number == 0 || number > 10) {
		shape = male ? 1416 : 1417;
		if (number > 10)
			number = 0;
	} else {
		shape = number + 1417;
	}
	int width, height;
	gShapeManager.getShapeSize(&width, &height, shape);
	bounds.set(0, 0, width, height);
	Control::show();
	InventoryTextPrinter.setFont(2);
	add(&closeButton);
	closeButton.show();
	add(&statsButton);
	statsButton.show();
	add(&attackModeButton);
	attackModeButton.show();
	if (IsAvatarInCombat()) {
		attackModeButton.setFrame(Item_getAttackMode(&NPCRef(Gump::object())));
	} else {
		attackModeButton.setFrame(Item_getDefaultAttackMode(&NPCRef(Gump::object())));
	}
	add(&diskButton);
	add(&combatButton);
	if (NPCRef(object()).isAvatar()) {
		diskButton.show();
		combatButton.show();
		if (IsAvatarInCombat())
			combatButton.advance();
	} else {
		diskButton.hide();
		diskButton.lock();
		combatButton.hide();
		combatButton.lock();
	}
	add(&protectButton);
	protectButton.show();
	add(&twoHandedMark);
	add(&twoSlotMark);
	twoSlotMark.setFrame(1);
	update(NPCRef(Gump::object()), male, 1);
	moveTo(0, 0);
}

void InventoryGump::update(NPCRef npc, unsigned char male, unsigned char attach)
{
	objref item;
	int i;

	RefitInventory(npc);
	for (i = 0; i <= 11; i++) {
		item = objref(GetItemInSlot(npc, i));
		slots[i].initialize(item, male);
		if (attach)
			add(&slots[i]);
		slots[i].show();
	}
	if (slots[6].carried == slots[7].carried && slots[6].carried.valid()) {
		slots[7].unlock();
		slots[7].hide();
		slots[7].lock();
		twoSlotMark.unlock();
		twoSlotMark.show();
	} else {
		slots[7].unlock();
		slots[7].show();
		twoSlotMark.hide();
	}
	if (slots[1].carried == slots[2].carried && slots[1].carried.valid()) {
		slots[2].unlock();
		slots[2].hide();
		slots[2].lock();
		twoHandedMark.unlock();
		twoHandedMark.show();
	} else {
		slots[2].unlock();
		slots[2].show();
		twoHandedMark.hide();
	}
	twoSlotMark.lock();
	twoHandedMark.lock();
}

void InventoryGump::refresh(char force)
{
	unsigned char male = Npc_isMale(&NPCRef(Gump::object()));

	if (dirty || force) {
		update(Gump::object(), male, 0);
		dirty = 0;
	}
}

unsigned char InventoryGump::handle(MouseState *event)
{
	unsigned char result, hit;
	int x = MouseState_getX(event);
	int y = event->y;

	if (!bounds.contains(x, y))
		return 0;
	if (!(hit = gShapeManager.isCursorInBounds(shape, 0, bounds, Point(x, y))))
		return 0;
	if (event->action == MOUSE_RELEASE)
		return GUMP_DROP_HERE;
	for (int i = 0; i <= 11; i++) {
		if ((result = slots[i].handle(event)) != 0) {
			selectedItem = slots[i].carried;
			int height, width;
			gShapeManager.getFrameSize(&width, &height,
				TYPE(ITEM(selectedItem.off)), FRAME(ITEM(selectedItem.off)));
			dragOffsetX = x - (slots[i].x + (width >> 1));
			dragOffsetY = y - (slots[i].y + (height >> 1));
			clickX = x;
			clickY = y;
			if (result == GUMP_DRAG_ITEM && !PickingItem) {
				if (HasEquipUsecode(slots[i].carried)) {
					objref used = 0;
					used = slots[i].carried;
					if (used.off != 0) {
						unsigned char previous = DialogState;
						DialogState = DIALOG_USING;
						RunUsable(6, used.off, -1);
						DialogState = previous;
					}
				}
				objref empty = 0;
				if (i == 1 && slots[1].carried == slots[2].carried && slots[1].carried.valid()) {
					slots[2].carried = empty;
					slots[2].unlock();
					slots[2].show();
					twoHandedMark.unlock();
					twoHandedMark.hide();
					twoHandedMark.lock();
				} else if (i == 6 && slots[6].carried == slots[7].carried && slots[6].carried.valid()) {
					slots[7].carried = empty;
					slots[7].unlock();
					slots[7].show();
					twoSlotMark.unlock();
					twoSlotMark.hide();
					twoSlotMark.lock();
				}
				slots[i].carried = empty;
				return result;
			}
			return result;
		}
	}
	if ((result = closeButton.handle(event)) != 0) {
		switch (result) {
		case BUTTON_CLICKED: return GUMP_CLOSE;
		default: return result;
		}
	}
	if (PickingItem)
		goto done;
	if ((result = statsButton.handle(event)) != 0) {
		switch (result) {
		case BUTTON_CLICKED: return GUMP_STATS;
		default: return result;
		}
	} else if (diskButton.isVisible() && (result = diskButton.handle(event)) != 0) {
		switch (result) {
		case BUTTON_CLICKED: return GUMP_SAVE_DIALOG;
		default: return result;
		}
	} else if (combatButton.isVisible() && (result = combatButton.handle(event)) != 0) {
		if (result == GUMP_HANDLED)
			return result;
		unsigned char combat = IsAvatarInCombat();
		if (combat && combatButton.getFrame() == 0) {
			BreakOffCombat();
			return GUMP_HANDLED;
		} else if (!combat && combatButton.getFrame() == 1) {
			BeginCombat();
			return GUMP_REFRESH;
		}
		return GUMP_HANDLED;
	} else if (protectButton.isVisible() && (result = protectButton.handle(event)) != 0) {
		switch (result) {
		case BUTTON_ON:
			if (attackModeButton.getFrame() != 4)
				return GUMP_SET_LEADER;
			protectButton.setFrame(0);
			break;
		case BUTTON_OFF:
			return GUMP_CLEAR_LEADER;
		default:
			return GUMP_HANDLED;
		}
	} else if (attackModeButton.isVisible() && (result = attackModeButton.handle(event)) != 0) {
		if (result == GUMP_HANDLED)
			return result;
		int frame = attackModeButton.getFrame();
		if (frame == 4 && protectButton.getFrame() != 0) {
			frame++;
			attackModeButton.setFrame(frame);
		}
		if (!NPCRef(Gump::object()).isAvatar()) {
			if (frame == 9)
				attackModeButton.setFrame(0);
		} else {
			switch (frame) {
			case 3:
			case 4:
				attackModeButton.setFrame(5);
				break;
			case 6:
			case 7:
			case 8:
				attackModeButton.setFrame(9);
				break;
			}
		}
		NPCRef npc(NPCRef(object()));
		if (attackModeButton.getFrame() != Item_getDefaultAttackMode(&objref(npc.off))) {
			Item_setDefaultAttackMode(&objref(npc.off), attackModeButton.getFrame());
			if (IsAvatarInCombat())
				Item_setAttackMode(&objref(npc.off), attackModeButton.getFrame());
		}
	} else
		goto done;
	return GUMP_HANDLED;
done:
	return hit && event->action == MOUSE_CLICK ? GUMP_MOVE : 0;
}

void InventoryGump::draw(View *target)
{
	int halfWidth;

	if (visible) {
		ShapeManager_draw(&gShapeManager, target, bounds.x, bounds.y, shape, 0, 0, 0);
		View *saved = InventoryTextPrinter.target;
		InventoryTextPrinter.target = target;
		int weight = DetermineWeightOfContents(Gump::object());
		int load = weight / 10;
		if (weight % 10 != 0)
			load++;
		char text[20];
		sprintf(text, "%02d/%02d", load, NPCRef(Gump::object()).strength() << 1);
		halfWidth = InventoryTextPrinter.textWidth(text);
		halfWidth >>= 1;
		InventoryTextPrinter.x = bounds.x - halfWidth + 77;
		InventoryTextPrinter.y = bounds.y + 126;
		InventoryTextPrinter.printString(text);
		InventoryTextPrinter.target = saved;
	}
}

void InventoryGump::moveTo(int x, int y)
{
	bounds.moveTo(x, y);
	PointOffset far *positions;
	closeButton.moveTo(x + 23, y + 124);
	if (NPCRef(object()).isAvatar()) {
		diskButton.moveTo(x + 124, y + 114);
		combatButton.moveTo(x + 51, y + 100);
	}
	protectButton.moveTo(x + 47, y + 110);
	attackModeButton.moveTo(x + 48, y + 131);
	positions = PaperdollSlotOffsets;
	statsButton.moveTo(x + 123, y + 129);
	for (int i = 0; i <= 11; i++)
		slots[i].moveTo(x + positions[i].x, y + positions[i].y);
	twoSlotMark.moveTo(slots[7].x, slots[7].y);
	twoHandedMark.moveTo(slots[2].x, slots[2].y);
}

unsigned char InventoryGump::findSlot(unsigned char *slot, objref incoming, int x, int y)
{
	unsigned char i;
	int closest = 1000;
	unsigned char found = 0;
	objref occupied;
	unsigned char hit;
	int distances[13];

	for (i = 0; i <= 11; i++) {
		occupied = objref(GetItemInSlot(NPCRef(Gump::object()), i));
		if (occupied.valid() && (unsigned char)(CLASS_FLAGS(ITEM(occupied.off)) & CLASS_CONTENTS)) {
			int height, width;
			gShapeManager.getFrameSize(&width, &height,
				TYPE(ITEM(occupied.off)), FRAME(ITEM(occupied.off)));
			hit = gShapeManager.isCursorInBounds(TYPE(ITEM(occupied.off)), FRAME(ITEM(occupied.off)),
				Point(slots[i].x + (width >> 1), slots[i].y + (height >> 1)), Point(x, y));
			distances[i] = hit ? 0 : 1000;
		} else if (CanEquipInSlot(incoming, Gump::object(), i, 1)) {
			int dx = slots[i].x - x < 0 ? -(slots[i].x - x) : slots[i].x - x;
			int dy = slots[i].y - y < 0 ? -(slots[i].y - y) : slots[i].y - y;
			distances[i] = dx > dy ? dx : dy;
		} else {
			distances[i] = 1000;
		}
	}
	for (i = 0; i <= 11; i++) {
		if (distances[i] < closest) {
			closest = distances[i];
			*slot = i;
			found = 1;
		}
	}
	return found;
}

unsigned char InventoryGump::accepts(objref incoming, int x, int y)
{
	unsigned char slot;
	objref occupied;

	if (!findSlot(&slot, incoming, x, y)) {
		ReportNoCanDo(0);
		return 0;
	}
	occupied = objref(GetItemInSlot(NPCRef(Gump::object()), slot));
	if (occupied.valid() && (unsigned char)(CLASS_FLAGS(ITEM(occupied.off)) & CLASS_CONTENTS)) {
		int placed = TryToPlaceItem(occupied, 1, 0);
		switch (placed) {
		case 0:
			return 1;
		case 1:
			ReportNoCanDo(4);
			break;
		case 2:
		case 3:
			ReportNoCanDo(0);
			break;
		case 4:
			ReportNoCanDo(5);
			break;
		}
		return 0;
	}
	if (!CanCarryWeight(object(), incoming)) {
		ReportNoCanDo(4);
		return 0;
	}
	MarkItemOkayToTake(incoming);
	unsigned char preferred = ReadyRecords.get(ReadyLookup.get(TYPE(ITEM(incoming.off))))->slot;
	PlaceItemInContainer(&incoming, object());
	if (preferred == 20 && (slot == 1 || slot == 2)) {
		slots[1].accept(incoming);
		slots[2].accept(incoming);
		slots[2].hide();
		slots[2].lock();
		twoHandedMark.unlock();
		twoHandedMark.show();
		twoHandedMark.lock();
		slot = 20;
	} else if (preferred == 21 && (slot == 6 || slot == 7)) {
		slots[6].accept(incoming);
		slots[7].accept(incoming);
		slots[7].hide();
		slots[7].lock();
		twoSlotMark.unlock();
		twoSlotMark.show();
		twoSlotMark.lock();
		slot = 21;
	} else {
		slots[slot].accept(incoming);
	}
	EquipItem(incoming, object(), slot, 1);
	dirty = 1;
	return 1;
}

objref InventoryGump::selected() { return selectedItem; }

int InventoryGump::dragX() { return dragOffsetX; }

int InventoryGump::dragY() { return dragOffsetY; }

void InventoryGump::setDragX(int x) { dragOffsetX = x; }

void InventoryGump::setDragY(int y) { dragOffsetY = y; }

int InventoryGump::mouseX() { return clickX; }

int InventoryGump::mouseY() { return clickY; }

unsigned char InventoryGump::findPosition(objref item, int *x, int *y)
{
	for (int i = 0; i < 12; i++) {
		if (slots[i].carried == item) {
			*x = slots[i].x;
			*y = slots[i].y;
			return 1;
		}
	}
	return 0;
}
