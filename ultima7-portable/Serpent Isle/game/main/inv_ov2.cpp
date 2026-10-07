/* Serpent Isle SI.EXE, overlay segment 330 (file offsets 0x096000 to 0x099f8b, 16267 bytes).
 * Borland C++ 2.0 -mm -O -P -d -Y -y- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
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
#include "itemrec.h"

/* the two automaton shapes; neither eats */
#define SHAPE_AUTOMATON     747
#define SHAPE_AUTOMATON_NPC 658

/* the first paperdoll shape in gumps.vga */
#define PAPERDOLL_BASE 1523

#define TYPE(p) ((p)->typeFrame & 0x3ff)
#define FRAME(p) (((p)->typeFrame & 0x7c00) >> 10)
#define TYPE_CLASS(p) (gItemTypeInfo[TYPE(p)].typeClass)
#define CLASS_FLAGS(p) (ItemTypeClassFlags[TYPE_CLASS(p)])

ProportionalTextPrinter InventoryTextPrinter;

struct PointOffset { int16_t x, y; };

/* where each slot sits on the male and the female paperdoll */
const PointOffset MaleSlotOffsets[SLOT_COUNT] = {
	{ 94, 44 }, { 61, 66 }, { 72, 33 }, { 72, 44 }, { 72, 22 }, { 94, 44 },
	{ 94, 44 }, { 94, 44 }, { 61, 66 }, { 72, 22 }, { 54, 59 }, { 83, 55 },
	{ 72, 33 }, { 72, 99 }, { 83, 66 }, { 94, 22 }, { 83, 22 }, { 61, 22 },
};

const PointOffset FemaleSlotOffsets[SLOT_COUNT] = {
	{ 94, 44 }, { 61, 66 }, { 72, 33 }, { 72, 47 }, { 72, 22 }, { 94, 44 },
	{ 94, 44 }, { 94, 44 }, { 61, 66 }, { 72, 22 }, { 54, 59 }, { 84, 52 },
	{ 72, 33 }, { 72, 99 }, { 83, 66 }, { 94, 28 }, { 82, 25 }, { 61, 22 },
};

void StatsGump::initialize()
{
	int16_t height, width;

	shape = 1465;
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
	if (NPCRef(Gump::object()).type() == SHAPE_AUTOMATON_NPC || NPCRef(Gump::object()).type() == SHAPE_AUTOMATON)
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
	if (Npc_hasFreezeFlag(&AvatarRef)) {
		if ((displayed.type() != SHAPE_AUTOMATON_NPC
				|| displayed.type() == SHAPE_AUTOMATON_NPC && Npc_hasPetraFlag(&AvatarRef))
				&& displayed.type() != SHAPE_AUTOMATON
				&& (!NPCRef(Gump::object()).isAvatar()
					|| NPCRef(Gump::object()).isAvatar() && !Npc_hasPetraFlag(&AvatarRef))) {
			if (Npc_packedManaLowRange(&NPCRef(Gump::object())))
				frame = 2;
			else if (Npc_packedManaMiddleRange(&NPCRef(Gump::object())))
				frame = 3;
			else if (Npc_packedManaUpperRange(&NPCRef(Gump::object())))
				frame = 4;
			else if (Npc_packedManaHighRange(&NPCRef(Gump::object())))
				frame = 5;
			else
				frame = 1;
		} else
			frame = 0;
	} else
		frame = 0;
}

uint8_t StatsGump::handle(MouseState *event)
{
	uint8_t result = 0;

	if (!isVisible())
		return 0;
	int16_t x = MouseState_getX(event);
	int16_t y = event->y;
	if (!bounds.contains(x, y))
		return 0;
	int16_t hit = gShapeManager.isCursorInBounds(shape, 0, bounds, Point(x, y));
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
		int16_t x = bounds.x;
		int16_t y = bounds.y;
		ShapeManager_draw(&gShapeManager, target, x, y, shape, frame, 0, 0);
		View *saved = StatsTextPrinter.target;
		StatsTextPrinter.target = target;
		char name[20], text[20];
		int16_t type = NPCRef(Gump::object()).type();
		if (type == SHAPE_AUTOMATON)
			_fstrncpy(name, GetGameText(3, 241), 19);
		else
			_fstrncpy(name, NPCRef(Gump::object()).buffer()->name, 19);
		name[19] = 0;
		int16_t halfWidth = StatsTextPrinter.textWidth(name) >> 1;
		int16_t center = 47;
		int16_t nameX = center - halfWidth + 28;
		StatsTextPrinter.x = x + nameX;
		StatsTextPrinter.y = y + 12;
		StatsTextPrinter.printString(name);
		int16_t i, textX, textY;
		int32_t experience, level;
		experience = NPCRef(Gump::object()).buffer()->experience;
		level = Npc_getLevel(&NPCRef(Gump::object()));
		for (i = 0; i <= 9; i++) {
			switch (i) {
			case 0:
				textY = 23;
				sprintf(text, GetGameText(3, 239), NPCRef(Gump::object()).strength());
				break;
			case 1:
				textY = 32;
				sprintf(text, GetGameText(3, 239), NPCRef(Gump::object()).dexterity());
				break;
			case 2:
				textY = 41;
				sprintf(text, GetGameText(3, 239), NPCRef(Gump::object()).intelligence());
				break;
			case 7:
				textY = 52;
				sprintf(text, GetGameText(3, 239), (uint8_t)(NPCRef(Gump::object()).buffer()->combat & 0x1f));
				break;
			case 9:
				textY = 61;
				if (NPCRef(Gump::object()).isAvatar())
					sprintf(text, GetGameText(3, 239), (uint8_t)(NPCRef(Gump::object()).buffer()->magic & 0x1f));
				else
					sprintf(text, GetGameText(3, 242));
				break;
			case 6:
				textY = 72;
				sprintf(text, GetGameText(3, 239), (int8_t)Item_getHitPoints(&NPCRef(Gump::object())));
				break;
			case 8:
				textY = 81;
				if (NPCRef(Gump::object()).isAvatar())
					sprintf(text, GetGameText(3, 239), (uint8_t)(NPCRef(Gump::object()).buffer()->mana & 0x1f));
				else
					sprintf(text, GetGameText(3, 242));
				break;
			case 4:
				textY = 92;
				sprintf(text, GetGameText(3, 243), (long) experience);
				break;
			case 5:
				textY = 101;
				sprintf(text, GetGameText(3, 239), level);
				break;
			case 3:
				textY = 110;
				sprintf(text, GetGameText(3, 239), NPCRef(Gump::object()).buffer()->training);
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

void StatsGump::moveTo(int16_t x, int16_t y)
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

uint8_t StatsGump::accepts(objref, int16_t, int16_t) { return 0; }

objref StatsGump::selected()
{
	objref result = 0;

	return result;
}

int16_t StatsGump::mouseX() { return clickX; }

int16_t StatsGump::mouseY() { return clickY; }

int16_t StatsGump::dragX() { return dragOffsetX; }

int16_t StatsGump::dragY() { return dragOffsetY; }

void StatsGump::setDragX(int16_t x) { dragOffsetX = x; }

void StatsGump::setDragY(int16_t y) { dragOffsetY = y; }

void StatsGump::refresh(int8_t force)
{
	if (dirty || force)
		update();
}

uint8_t StatsGump::findPosition(objref, int16_t *, int16_t *) { return 0; }

void ItemSlot::initialize(objref item, uint8_t isMale, uint8_t slot, InventoryGump *gump)
{
	carried = item;
	male = isMale;
	showEmpty = 0;
	shape = 0;
	frame = 0;
	if (slot == 0)
		gump->weaponPose = 0;
	if (carried.valid())
		setItem(carried, slot, gump);
}

uint8_t ItemSlot::handle(MouseState *event)
{
	uint8_t result = 0;
	int16_t mouseX, mouseY;
	uint8_t hit;

	if (!visible)
		return 0;
	mouseX = MouseState_getX(event);
	mouseY = event->y;
	hit = 0;
	if (!carried.valid() && event->action != MOUSE_RELEASE)
		return 0;
	if (carried.valid()) {
		if (shape != 0) {
			hit = gShapeManager.isCursorInBounds(shape, frame, Point(x, y), Point(mouseX, mouseY));
		} else {
			int16_t height, width;
			gShapeManager.getFrameSize(&width, &height,
				TYPE(ITEM(carried.off)), FRAME(ITEM(carried.off)));
			hit = gShapeManager.isCursorInBounds(TYPE(ITEM(carried.off)), FRAME(ITEM(carried.off)),
				Point(x + (width >> 1), y + (height >> 1)), Point(mouseX, mouseY));
		}
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

void ItemSlot::moveTo(int16_t newX, int16_t newY)
{
	x = newX;
	y = newY;
}

void ItemSlot::draw(View *target)
{
	int16_t height, width;

	if (visible && (carried.valid() || showEmpty)) {
		if (shape != 0)
			ShapeManager_draw(&gShapeManager, target, x, y, shape, frame, 0, 0);
		else {
			gShapeManager.getFrameSize(&width, &height,
				TYPE(ITEM(carried.off)), FRAME(ITEM(carried.off)));
			ShapeManager_drawItem(&gShapeManager, x + (width >> 1), y + (height >> 1), carried, target);
		}
	}
}

/* Chooses the paperdoll picture for an item put in a slot. */
uint8_t ItemSlot::setItem(objref item, uint8_t slot, InventoryGump *gump)
{
	if (!item.valid())
		return 0;
	carried = item;
	switch (item.type()) {
	case 583:
		shape = 9;
		break;
	case 403:
		shape = 28;
		break;
	case 227:
		if (item.frame() == 0)
			shape = 28;
		else if (item.frame() == 1)
			shape = 8;
		else if (item.frame() == 2)
			shape = 61;
		else if (item.frame() == 3)
			shape = 116;
		else if (item.frame() == 4)
			shape = 164;
		break;
	case 383:
		if (item.frame() == 0)
			shape = 74;
		else if (item.frame() == 1)
			shape = 162;
		break;
	case 539:
		shape = 24;
		break;
	case 541:
		shape = 30;
		break;
	case 542:
		if (item.frame() == 0)
			shape = 31;
		else if (item.frame() == 1)
			shape = 88;
		else if (item.frame() == 2)
			shape = 114;
		break;
	case 543:
		shape = 20;
		break;
	case 545:
		shape = 34;
		break;
	case 486:
		shape = 18;
		break;
	case 569:
		shape = 57;
		break;
	case 570:
		shape = 92;
		break;
	case 571:
		shape = 22;
		break;
	case 572:
		shape = 117;
		break;
	case 638:
		shape = 94;
		break;
	case 573:
		shape = 85;
		break;
	case 419:
		shape = 140;
		break;
	case 490:
		shape = 141;
		break;
	case 729:
		shape = 142;
		break;
	case 836:
		shape = 1;
		break;
	case 574:
		shape = 60;
		break;
	case 575:
		shape = 26;
		break;
	case 576:
		shape = 87;
		break;
	case 578:
		shape = 104;
		break;
	case 579:
		shape = 47;
		break;
	case 580:
		shape = 25;
		break;
	case 835:
		shape = 70;
		break;
	case 584:
		shape = 54;
		break;
	case 587:
		switch (item.frame()) {
		case 0: shape = 58; break;
		case 1: shape = 73; break;
		case 2: shape = 86; break;
		case 3: shape = 46; break;
		case 4: shape = 150; break;
		case 5: shape = 151; break;
		case 6: shape = 106; break;
		}
		break;
	case 609:
		shape = 37;
		break;
	case 663:
		shape = 75;
		break;
	case 666:
		shape = 2;
		break;
	case 677:
		if (item.frame() == 0)
			shape = 152;
		else if (item.frame() == 1)
			shape = 153;
		break;
	case 686:
		shape = 72;
		break;
	case 1004:
		if (item.frame() == 0)
			shape = 59;
		else if (item.frame() == 1)
			shape = 129;
		else if (item.frame() == 2)
			shape = 33;
		else if (item.frame() == 3)
			shape = 165;
		else if (item.frame() == 4)
			shape = 129;
		break;
	case 586:
		shape = 19;
		break;
	case 585:
		shape = 38;
		break;
	case 640:
		shape = 96;
		break;
	case 241:
		shape = 105;
		break;
	case 698:
		shape = 98;
		break;
	case 701:
		shape = 63;
		break;
	case 595:
		shape = 113;
		break;
	case 722:
		shape = 3;
		break;
	case 723:
		shape = 132;
		break;
	case 474:
		shape = 102;
		break;
	case 547:
		shape = 76;
		break;
	case 520:
		shape = 161;
		break;
	case 231:
		shape = 67;
		break;
	case 806:
		shape = 139;
		break;
	case 942:
		shape = 138;
		break;
	case 535:
		shape = 143;
		break;
	case 926:
		shape = 144;
		break;
	case 549:
		shape = 62;
		break;
	case 551:
		shape = 41;
		break;
	case 552:
		shape = 77;
		break;
	case 553:
		shape = 43;
		break;
	case 554:
		shape = 21;
		break;
	case 556:
		shape = 68;
		break;
	case 557:
		shape = 53;
		break;
	case 558:
		shape = 65;
		break;
	case 591:
		shape = 64;
		break;
	case 594:
		shape = 35;
		break;
	case 563:
		shape = 12;
		break;
	case 564:
		shape = 36;
		break;
	case 567:
		shape = 110;
		break;
	case 568:
		shape = 101;
		break;
	case 589:
		shape = 84;
		break;
	case 590:
		shape = 29;
		break;
	case 592:
		shape = 103;
		break;
	case 593:
		shape = 111;
		break;
	case 597:
		shape = 17;
		break;
	case 711:
		shape = 160;
		break;
	case 598:
		shape = 32;
		break;
	case 599:
		shape = 107;
		break;
	case 600:
		shape = 51;
		break;
	case 601:
		shape = 5;
		break;
	case 602:
		shape = 108;
		break;
	case 603:
		shape = 49;
		break;
	case 604:
		shape = 48;
		break;
	case 605:
		shape = 16;
		break;
	case 606:
		shape = 69;
		break;
	case 608:
		shape = 109;
		break;
	case 618:
		shape = 93;
		break;
	case 620:
		shape = 91;
		break;
	case 622:
		shape = 115;
		break;
	case 623:
		shape = 50;
		break;
	case 508:
		shape = 163;
		break;
	case 624:
		shape = 83;
		break;
	case 625:
		shape = 100;
		break;
	case 626:
		shape = 52;
		break;
	case 629:
		shape = 118;
		break;
	case 417:
		shape = 133;
		break;
	case 856:
		shape = 45;
		break;
	case 630:
		shape = 42;
		break;
	case 662:
		shape = 44;
		break;
	case 659:
		shape = 66;
		break;
	case 596:
		shape = 81;
		break;
	case 792:
		shape = 78;
		break;
	case 771:
		shape = 134;
		break;
	case 636:
		shape = 95;
		break;
	case 637:
	case 710:
		shape = 97;
		break;
	case 994:
		shape = 112;
		break;
	case 296:
		switch (item.frame()) {
		case 0: shape = 120; break;
		case 1: shape = 122; break;
		case 2: shape = 148; break;
		case 3: shape = 148; break;
		}
		break;
	case 887:
		switch (item.frame()) {
		case 0:
			shape = 147;
			break;
		case 1:
			shape = 149;
			break;
		}
		break;
	case 802:
		shape = 89;
		break;
	case 801:
		shape = 6;
		break;
	case 955:
		shape = 0;
		break;
	case 635:
		shape = 39;
		break;
	case 761:
		shape = 15;
		break;
	case 400:
	case 402:
	case 414:
	case 762:
		shape = 135;
		break;
	case 996:
		shape = 166;
		break;
	case 1001:
		shape = 167;
		break;
	case 990:
		shape = 168;
		break;
	case 1013:
		shape = 169;
		break;
	default:
		shape = 0;
		break;
	}
	if (shape != 0 || item.type() == 955)
		shape = shape + PAPERDOLL_BASE;
	gump->poseArms();
	pickFrame(slot, gump);
	if (slot == 1) {
		if (shape == 0)
			moveTo(gump->bounds.x + 36, gump->bounds.y + 53);
		else
			moveTo(gump->bounds.x + MaleSlotOffsets[1].x, gump->bounds.y + MaleSlotOffsets[1].y);
	}
	if (slot == 0) {
		if (shape == 0)
			moveTo(gump->bounds.x + 115, gump->bounds.y + 53);
		else
			moveTo(gump->bounds.x + MaleSlotOffsets[0].x, gump->bounds.y + MaleSlotOffsets[0].y);
	}
	return 1;
}

/* Chooses the frame of a slot's paperdoll picture. */
void ItemSlot::pickFrame(uint8_t slot, InventoryGump *gump)
{
	frame = 0;
	if (slot == 16) {
		shape = 1622;
		switch (carried.type()) {
		case 543:
		case 572:
		case 578:
			frame = 2;
			break;
		}
	} else if (slot == 17)
		frame = 1;
	else if (slot == 15)
		;
	else if (slot == 11) {
		switch (carried.type()) {
		case 584:
			frame = male;
			break;
		case 595:
			frame = 2;
			break;
		case 802:
			frame = 0;
			break;
		default:
			frame = 1;
			break;
		}
	} else if (slot == 8)
		frame = gump->weaponPose * 2 + 1;
	else if (slot == 7)
		frame = gump->weaponPose * 2;
	else if (slot == 12 || slot == 13 || slot == 14 || slot == 9 || slot == 4)
		frame = male;
	else if (slot == 5 || slot == 6)
		frame = gump->weaponPose;
	else if (slot == 10) {
		uint8_t count = Item_getQuantity(&carried);
		if (gump->ammoLoaded())
			count--;
		if (count == 0)
			frame = 1;
		else if (count < 3)
			frame = 2;
		else if (count < 5)
			frame = 3;
		else
			frame = 4;
	} else if (slot == 3)
		frame = carried.frame();
	else if (slot == 2)
		frame = male + 1;
	else {
		if (shape == 1658) {
			if (slot == 0)
				frame = 1;
			else if (slot != 1) {
				shape = 0;
				return;
			}
			switch (carried.type()) {
			case 400:
				if (carried.frame() != 0)
					return;
				break;
			case 414:
				if (carried.frame() != 0)
					return;
				break;
			case 762:
				switch (carried.frame()) {
				case 4:
				case 6:
				case 8:
				case 10:
				case 18:
				case 21:
				case 27:
				case 28:
					return;
				default:
					shape = 0;
					return;
				}
			case 402:
				if (carried.frame() != 3)
					return;
				shape = 0;
				return;
			}
		}
		if (slot == 0) {
			switch (carried.type()) {
			case 490:
			case 543:
			case 545:
			case 572:
			case 578:
			case 585:
			case 586:
			case 609:
			case 663:
			case 729:
				break;
			case 595:
			case 701:
				frame = 1;
				break;
			default:
				shape = 0;
				break;
			}
		} else if (slot == 1) {
			if ((uint8_t)ReadyRecords.get(ReadyLookup.get(carried.type()))->slot == 20
					|| (uint8_t)ReadyRecords.get(ReadyLookup.get(carried.type()))->slot == 1) {
				switch (carried.type()) {
				case 417:
				case 490:
				case 543:
				case 545:
				case 554:
				case 556:
				case 558:
				case 568:
				case 572:
				case 578:
				case 585:
				case 586:
				case 591:
				case 609:
				case 663:
				case 722:
				case 723:
				case 729:
				case 856:
					shape = 0;
					break;
				}
			} else
				shape = 0;
		}
	}
}

void InventoryGump::initialize()
{
	uint8_t male = Npc_isMale(&NPCRef(Gump::object())) ? 1 : 0;

	shape = 1646;
	int16_t width, height;
	gShapeManager.getShapeSize(&width, &height, shape);
	bounds.set(0, 0, width, height);
	Control::show();
	InventoryTextPrinter.setFont(2);
	add(&closeButton);
	closeButton.show();
	add(&statsButton);
	statsButton.show();
	add(&diskButton);
	add(&combatButton);
	add(&combatStatsButton);
	combatStatsButton.show();
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
	update(NPCRef(Gump::object()), male, 1);
	moveTo(0, 0);
}

/* Whether the bow or crossbow in hand has its ammunition in the quiver. */
uint8_t InventoryGump::ammoLoaded()
{
	if (slots[0].carried.valid()) {
		if (slots[0].carried.type() == 597 || slots[0].carried.type() == 606) {
			switch (slots[10].carried.type()) {
			case 554:
			case 556:
			case 558:
			case 568:
			case 591:
			case 722:
				return 1;
			}
		}
		if (slots[0].carried.type() == 598) {
			switch (slots[10].carried.type()) {
			case 417:
			case 723:
			case 856:
				return 1;
			}
		}
	}
	return 0;
}

/* Poses the arms for the weapon in hand. */
void InventoryGump::poseArms()
{
	int16_t type = slots[0].carried.type();

	if ((uint8_t)ReadyRecords.get(ReadyLookup.get(type))->slot == 20) {
		switch (slots[0].carried.type()) {
		case 241:
		case 553:
		case 589:
		case 603:
		case 618:
		case 620:
		case 625:
		case 626:
		case 640:
		case 662:
			weaponPose = 2;
			break;
		case 597:
		case 598:
		case 606:
		case 711:
			weaponPose = 0;
			break;
		default:
			weaponPose = 1;
			break;
		}
	} else
		weaponPose = 0;
	if (slots[12].male) {
		torso.setFrame(weaponPose * 2 + 1);
		arms.setFrame(weaponPose * 2 + 3);
	} else {
		torso.setFrame(weaponPose * 2);
		arms.setFrame(weaponPose * 2 + 2);
	}
	if (slots[6].carried.valid())
		slots[6].pickFrame(6, this);
	if (slots[5].carried.valid())
		slots[5].pickFrame(5, this);
	if (slots[8].carried.valid())
		slots[8].pickFrame(8, this);
	if (slots[7].carried.valid())
		slots[7].pickFrame(7, this);
}

void InventoryGump::update(NPCRef npc, uint8_t male, uint8_t attach)
{
	objref item;
	int16_t i;

	RefitInventory(npc);
	for (i = 0; i <= 17; i++) {
		item = objref(GetItemInSlot(npc, i));
		slots[i].initialize(item, male, i, this);
		if (attach) {
			add(&slots[i]);
			if (i == 1) {
				add(&leftHand);
				leftHand.hide();
				leftHand.lock();
				add(&rightHand);
				rightHand.hide();
				rightHand.lock();
				add(&quiverAmmo);
			}
		}
		slots[i].show();
		if (attach) {
			switch (i) {
			case 8:
				add(&arms);
				add(&torso);
				break;
			case 11:
				add(&feet);
				break;
			case 14:
				add(&head);
				add(&legs);
				add(&neck);
				break;
			}
		}
	}
	if (slots[2].carried.valid()) {
		neck.setShape(slots[2].shape);
		neck.setFrame(0);
		neck.unlock();
		neck.show();
	} else {
		neck.hide();
		neck.lock();
	}
	if (slots[1].carried.valid()) {
		if (slots[1].carried.type() == 597 || slots[1].carried.type() == 606) {
			quiverAmmo.unlock();
			if (slots[10].carried.valid()) {
				quiverAmmo.show();
				switch (slots[10].carried.type()) {
				case 722:
					quiverAmmo.setShape(1526);
					quiverAmmo.setFrame(0);
					break;
				case 568:
					quiverAmmo.setShape(1624);
					quiverAmmo.setFrame(0);
					break;
				case 591:
					quiverAmmo.setShape(1587);
					quiverAmmo.setFrame(0);
					break;
				case 554:
					quiverAmmo.setShape(1544);
					quiverAmmo.setFrame(0);
					break;
				case 556:
					quiverAmmo.setShape(1591);
					quiverAmmo.setFrame(0);
					break;
				case 558:
					quiverAmmo.setShape(1588);
					quiverAmmo.setFrame(0);
					break;
				default:
					quiverAmmo.hide();
				}
			} else
				quiverAmmo.hide();
			quiverAmmo.lock();
		} else if (slots[1].carried.type() == 598) {
			quiverAmmo.unlock();
			if (slots[10].carried.valid()) {
				quiverAmmo.show();
				switch (slots[10].carried.type()) {
				case 417:
					quiverAmmo.setShape(1656);
					quiverAmmo.setFrame(0);
					break;
				case 723:
					quiverAmmo.setShape(1655);
					quiverAmmo.setFrame(0);
					break;
				case 856:
					quiverAmmo.setShape(1568);
					quiverAmmo.setFrame(0);
					break;
				default:
					quiverAmmo.hide();
				}
			} else
				quiverAmmo.hide();
			quiverAmmo.lock();
		} else {
			quiverAmmo.unlock();
			quiverAmmo.hide();
			quiverAmmo.lock();
		}
	} else {
		quiverAmmo.unlock();
		quiverAmmo.hide();
		quiverAmmo.lock();
	}
	switch (npc.type()) {
	case SHAPE_AUTOMATON_NPC:
		legs.setShape(Npc_isMale(&npc) ? 1537 : 1536);
		if (Npc_hasPetraFlag(&AvatarRef))
			legs.setFrame(Npc_getSkinColor(&AvatarRef));
		else
			legs.setFrame(3);
		break;
	case 721:
		legs.setShape(Npc_isMale(&npc) ? 1537 : 1536);
		if (Npc_hasPetraFlag(&AvatarRef))
			legs.setFrame(3);
		else
			legs.setFrame(Npc_getSkinColor(&AvatarRef));
		break;
	case SHAPE_AUTOMATON:
		legs.setShape(1537);
		legs.setFrame(3);
		break;
	default:
		legs.setShape(slots[12].male ? 1537 : 1536);
		legs.setFrame(0);
		break;
	}
	legs.show();
	uint8_t part = 132;
	switch (npc.type()) {
	case 721:
	case 989:
		if (slots[4].male)
			part = 128;
		else if (Npc_hasPetraFlag(&AvatarRef))
			part = 137;
		else
			part = 4;
		break;
	case 488:
		part = 124;
		break;
	case 487:
		part = 126;
		break;
	case 465:
		part = 125;
		break;
	case SHAPE_AUTOMATON_NPC:
		if (Npc_hasPetraFlag(&AvatarRef)) {
			if (Npc_isMale(&npc))
				part = 128;
			else
				part = 4;
		} else
			part = 137;
		break;
	case SHAPE_AUTOMATON:
		part = 145;
		break;
	default:
		switch (Item_getNpcNumber(&npc)) {
		case 45:
			part = 154;
			break;
		case 44:
			part = 155;
			break;
		case 149:
			part = 127;
			break;
		case 26:
			part = 156;
			break;
		case 168:
			part = 157;
			break;
		case 34:
			part = 158;
			break;
		case 152:
			part = 159;
			break;
		}
		break;
	}
	head.setShape(part + PAPERDOLL_BASE);
	head.setFrame(npc.isAvatar() && !Npc_hasPetraFlag(&npc)
		|| npc.type() == SHAPE_AUTOMATON_NPC && Npc_hasPetraFlag(&AvatarRef)
		? Npc_getSkinColor(&AvatarRef) * 2 : 0);
	if (slots[4].carried.valid() && slots[4].shape != 1556)
		head.setFrame(head.getFrame() + 1);
	head.show();
	feet.setShape(1533);
	feet.setFrame(slots[12].male ? 1 : 0);
	feet.show();
	part = 7;
	switch (npc.type()) {
	case 721:
		if (Npc_hasPetraFlag(&AvatarRef))
			part = 136;
		else {
			switch (Npc_getSkinColor(&AvatarRef)) {
			case 0:
				part = 7;
				break;
			case 1:
				part = 130;
				break;
			case 2:
				part = 131;
				break;
			}
		}
		break;
	case SHAPE_AUTOMATON_NPC:
		if (Npc_hasPetraFlag(&AvatarRef)) {
			switch (Npc_getSkinColor(&AvatarRef)) {
			case 0:
				part = 7;
				break;
			case 1:
				part = 130;
				break;
			case 2:
				part = 131;
				break;
			}
		} else
			part = 136;
		break;
	case SHAPE_AUTOMATON:
		part = 146;
		break;
	}
	torso.setShape(part + PAPERDOLL_BASE);
	torso.show();
	if (slots[12].carried.valid()) {
		arms.setShape(slots[12].shape);
		arms.unlock();
		arms.show();
	} else {
		arms.hide();
		arms.lock();
	}
	poseArms();
	if (slots[5].carried.valid()) {
		slots[8].unlock();
		slots[8].hide();
		slots[8].lock();
		slots[7].unlock();
		slots[7].hide();
		slots[7].lock();
	} else {
		slots[8].unlock();
		slots[8].show();
		slots[7].unlock();
		slots[7].show();
	}
	if (slots[1].carried == slots[0].carried && slots[1].carried.valid()) {
		slots[0].unlock();
		slots[0].hide();
		slots[0].lock();
	} else {
		slots[0].unlock();
		slots[0].show();
	}
	leftHand.unlock();
	if (slots[1].carried.valid() && slots[1].shape == INT32_C(0) && slots[1].carried != slots[0].carried)
		leftHand.show();
	else
		leftHand.hide();
	rightHand.unlock();
	if (slots[0].carried.valid() && slots[0].shape == INT32_C(0) && slots[0].carried != slots[1].carried)
		rightHand.show();
	else
		rightHand.hide();
	leftHand.lock();
	rightHand.lock();
}

void InventoryGump::refresh(int8_t force)
{
	uint8_t male = Npc_isMale(&NPCRef(Gump::object())) ? 1 : 0;

	if (dirty || force) {
		update(Gump::object(), male, 0);
		dirty = 0;
	}
}

uint8_t InventoryGump::handle(MouseState *event)
{
	uint8_t result, hit, saved;
	int16_t x = MouseState_getX(event);
	int16_t y = event->y;

	if (!bounds.contains(x, y))
		return 0;
	if (!(hit = gShapeManager.isCursorInBounds(shape, 0, bounds, Point(x, y))))
		return 0;
	if (event->action == MOUSE_RELEASE)
		return GUMP_DROP_HERE;
	for (int16_t i = 0; i <= 17; i++) {
		result = slots[i].handle(event);
		if (result == 0) {
			switch (i) {
			case 14:
				result = head.handle(event);
				if (result != 0)
					i = 4;
				else {
					result = legs.handle(event);
					if (result != 0)
						i = 12;
					else if (slots[2].carried.valid()) {
						slots[2].frame = 0;
						result = slots[2].handle(event);
						slots[2].frame = slots[2].male + 1;
						if (result != 0)
							i = 2;
					}
				}
				break;
			case 0:
				if (ammoLoaded()) {
					saved = slots[10].frame;
					slots[10].frame = 0;
					result = slots[10].handle(event);
					slots[10].frame = saved;
					if (result != 0)
						i = 10;
				}
				break;
			case 11:
				result = feet.handle(event);
				if (result != 0)
					i = 11;
				break;
			case 8:
				if (slots[12].carried.valid()) {
					slots[12].frame = weaponPose * 2 + slots[12].male + 2;
					result = slots[12].handle(event);
					slots[12].frame = slots[12].male;
					if (result != 0)
						i = 12;
				}
				if (result == 0) {
					result = torso.handle(event);
					if (result != 0)
						i = 12;
				}
				break;
			}
		}
		if (result != 0) {
			selectedItem = slots[i].carried;
			int16_t height, width;
			gShapeManager.getFrameSize(&width, &height,
				TYPE(ITEM(selectedItem.off)), FRAME(ITEM(selectedItem.off)));
			if (slots[i].shape == INT32_C(0)) {
				dragOffsetX = x - (slots[i].x + (width >> 1));
				dragOffsetY = y - (slots[i].y + (height >> 1));
			} else {
				dragOffsetX = -width >> 1;
				dragOffsetY = -height >> 1;
			}
			clickX = x;
			clickY = y;
			if (result == GUMP_DRAG_ITEM && !PickingItem) {
				if (HasEquipUsecode(slots[i].carried)) {
					objref used = 0;
					used = slots[i].carried;
					if (used.off != 0) {
						if (slots[i].carried.type() == 806)
							QueueUnequipScript((ItemId &)slots[i].carried);
						else {
							uint8_t previous = DialogState;
							DialogState = DIALOG_USING;
							RunUsable(6, used.off, -1);
							DialogState = previous;
						}
					}
				}
				objref empty = 0;
				if (i == 1 && slots[1].carried == slots[0].carried && slots[1].carried.valid()) {
					slots[0].carried = empty;
					slots[0].unlock();
					slots[0].show();
					poseArms();
					quiverAmmo.unlock();
					quiverAmmo.hide();
					quiverAmmo.lock();
					slots[10].pickFrame(10, this);
				} else if (i == 5 && slots[5].carried.valid()) {
					slots[8].unlock();
					slots[7].show();
					slots[7].unlock();
					slots[7].show();
				} else if (i == 12 && slots[12].carried.valid()) {
					arms.hide();
					arms.lock();
				} else if (i == 4 && slots[4].carried.valid() && slots[4].shape != 1556)
					head.setFrame(head.getFrame() - 1);
				else if (i == 1 && slots[1].carried.valid()) {
					leftHand.unlock();
					leftHand.hide();
					leftHand.lock();
				} else if (i == 0 && slots[0].carried.valid()) {
					rightHand.unlock();
					rightHand.hide();
					rightHand.lock();
				} else if (i == 2 && slots[2].carried.valid()) {
					neck.hide();
					neck.lock();
				} else if (i == 10 && slots[10].carried.valid()) {
					quiverAmmo.unlock();
					quiverAmmo.hide();
					quiverAmmo.lock();
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
	} else if ((result = combatStatsButton.handle(event)) != 0) {
		switch (result) {
		case BUTTON_CLICKED: return GUMP_COMBAT_STATS;
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
		uint8_t combat = IsAvatarInCombat();
		if (combat && combatButton.getFrame() == 0)
			BreakOffCombat();
		else if (!combat && combatButton.getFrame() == 1) {
			BeginCombat();
			return GUMP_REFRESH;
		}
		return GUMP_HANDLED;
	}
done:
	return hit && event->action == MOUSE_CLICK ? GUMP_MOVE : 0;
}

void InventoryGump::draw(View *target)
{
	int16_t halfWidth;

	if (visible) {
		ShapeManager_draw(&gShapeManager, target, bounds.x, bounds.y, shape, 0, 0, 0);
		View *saved = InventoryTextPrinter.target;
		InventoryTextPrinter.target = target;
		int16_t weight = DetermineWeightOfContents(Gump::object());
		int16_t load = weight / 10;
		if (weight % 10 != 0)
			load++;
		char text[20];
		sprintf(text, GetGameText(3, 244), load, NPCRef(Gump::object()).strength() << 1);
		halfWidth = InventoryTextPrinter.textWidth(text);
		halfWidth >>= 1;
		InventoryTextPrinter.x = bounds.x - halfWidth + 83;
		InventoryTextPrinter.y = bounds.y + 120;
		InventoryTextPrinter.printString(text);
		InventoryTextPrinter.target = saved;
	}
}

void InventoryGump::moveTo(int16_t x, int16_t y)
{
	bounds.moveTo(x, y);
	const PointOffset *positions;
	closeButton.moveTo(x + 23, y + 133);
	if (NPCRef(object()).isAvatar()) {
		diskButton.moveTo(x + 123, y + 137);
		combatButton.moveTo(x + 51, y + 142);
	}
	combatStatsButton.moveTo(x + 72, y + 137);
	positions = slots[12].male ? MaleSlotOffsets : FemaleSlotOffsets;
	statsButton.moveTo(x + 97, y + 137);
	for (int16_t i = 0; i <= 17; i++) {
		if (i == 0 && slots[i].shape == INT32_C(0) && slots[i].carried.valid())
			slots[i].moveTo(x + 115, y + 53);
		else if (i == 1 && slots[i].shape == INT32_C(0) && slots[i].carried.valid())
			slots[i].moveTo(x + 36, y + 53);
		else
			slots[i].moveTo(x + positions[i].x, y + positions[i].y);
	}
	legs.moveTo(x + positions[12].x, y + positions[12].y);
	head.moveTo(x + positions[4].x, y + positions[4].y);
	feet.moveTo(x + positions[11].x, y + positions[11].y);
	quiverAmmo.moveTo(x + positions[10].x, y + positions[10].y);
	torso.moveTo(x + positions[12].x, y + positions[12].y);
	neck.moveTo(x + positions[2].x, y + positions[2].y);
	arms.moveTo(x + positions[12].x, y + positions[12].y);
	leftHand.moveTo(x + positions[1].x - 5, y + positions[1].y - 8);
	rightHand.moveTo(x + positions[0].x, y + positions[0].y + 6);
}

uint8_t InventoryGump::findSlot(uint8_t *slot, objref incoming, int16_t x, int16_t y)
{
	uint8_t i;
	int16_t closest = 1000;
	uint8_t found = 0;
	objref occupied;
	uint8_t hit;
	int16_t distances[SLOT_COUNT];

	for (i = 0; i <= 17; i++) {
		occupied = objref(GetItemInSlot(NPCRef(Gump::object()), i));
		if (occupied.valid() && (uint8_t)(CLASS_FLAGS(ITEM(occupied.off)) & CLASS_CONTENTS) && i != 6) {
			if (slots[i].shape != 0) {
				hit = gShapeManager.isCursorInBounds(slots[i].shape, slots[i].frame,
					Point(slots[i].x, slots[i].y), Point(x, y));
			} else {
				int16_t height, width;
				gShapeManager.getFrameSize(&width, &height,
					TYPE(ITEM(occupied.off)), FRAME(ITEM(occupied.off)));
				hit = gShapeManager.isCursorInBounds(TYPE(ITEM(occupied.off)), FRAME(ITEM(occupied.off)),
					Point(slots[i].x + (width >> 1), slots[i].y + (height >> 1)), Point(x, y));
			}
			distances[i] = hit ? 0 : 1000;
		} else if (CanEquipInSlot(incoming, Gump::object(), i, 1)) {
			int16_t dx = slots[i].x - x < 0 ? -(slots[i].x - x) : slots[i].x - x;
			int16_t dy = slots[i].y - y < 0 ? -(slots[i].y - y) : slots[i].y - y;
			distances[i] = dx > dy ? dx : dy;
		} else {
			distances[i] = 1001;
		}
	}
	for (i = 0; i <= 17; i++) {
		if (distances[i] <= closest) {
			closest = distances[i];
			*slot = i;
			found = 1;
		}
	}
	return found;
}

uint8_t InventoryGump::accepts(objref incoming, int16_t x, int16_t y)
{
	uint8_t slot;
	objref occupied;

	if (!findSlot(&slot, incoming, x, y)) {
		ReportNoCanDo(0);
		return 0;
	}
	occupied = objref(GetItemInSlot(NPCRef(Gump::object()), slot));
	if (occupied.valid() && (uint8_t)(CLASS_FLAGS(ITEM(occupied.off)) & CLASS_CONTENTS)) {
		int16_t placed = TryToPlaceItem(occupied, 1, 0);
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
	Item_clearTemporary(&incoming);
	uint8_t preferred = ReadyRecords.get(ReadyLookup.get(TYPE(ITEM(incoming.off))))->slot;
	PlaceItemInContainer(&incoming, object());
	if (preferred == 20 && (slot == 1 || slot == 0)) {
		slots[1].setItem(incoming, 20, this);
		slots[0].setItem(incoming, 0, this);
		slots[0].hide();
		slots[0].lock();
		poseArms();
		slot = 20;
	} else if (preferred == 5 && slot == 5) {
		slots[5].setItem(incoming, 5, this);
		slots[8].hide();
		slots[8].lock();
		slots[7].hide();
		slots[7].lock();
		slot = 5;
	} else {
		slots[slot].setItem(incoming, slot, this);
	}
	EquipItem(incoming, object(), slot, 1);
	dirty = 1;
	return 1;
}

objref InventoryGump::selected() { return selectedItem; }

int16_t InventoryGump::dragX() { return dragOffsetX; }

int16_t InventoryGump::dragY() { return dragOffsetY; }

void InventoryGump::setDragX(int16_t x) { dragOffsetX = x; }

void InventoryGump::setDragY(int16_t y) { dragOffsetY = y; }

int16_t InventoryGump::mouseX() { return clickX; }

int16_t InventoryGump::mouseY() { return clickY; }

uint8_t InventoryGump::findPosition(objref item, int16_t *x, int16_t *y)
{
	for (int16_t i = 0; i < SLOT_COUNT; i++) {
		if (slots[i].carried == item) {
			*x = slots[i].x;
			*y = slots[i].y;
			return 1;
		}
	}
	return 0;
}

extern "C" void ResetInv_ov2Globals(void)
{
	memset((void *)&InventoryTextPrinter, 0, sizeof(InventoryTextPrinter));
}

extern "C" void ConstructInv_ov2Globals(void)
{
	new (&InventoryTextPrinter) ProportionalTextPrinter();
}
