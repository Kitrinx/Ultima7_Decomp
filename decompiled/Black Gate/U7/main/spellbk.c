/* Black Gate U7.EXE, overlay segment 348 (file offsets 0x0a7710 to 0x0a83e0, 3280 bytes).
 * Borland C++ 2.0 -mm -O -P -d rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include <dos.h>
#include <string.h>
#include <stdio.h>
#include "u7manage.h"
#include "item.h"
#include "npcref.h"
#include "colbuf.h"
#include "gumpmgr.h"
#include "itable.h"
#include "bltshape.h"
#include "oops.h"
#include "spell.h"
#include "u7event.h"
#include "cast.h"
#include "text.h"
#include "type.h"
#include "camera.h"
#include "spellbk.h"

struct View;

extern View Viewport;

extern unsigned char far SpellCastCounts[];

struct SpellRef : objref {
	SpellRef(objref ref) { off = ref.off; }
	char selection() { return GetSpellbookBookmark(this); }
	unsigned char hasSpell(int spell) { return DoesSpellbookHaveSpell(this, spell); }
};

void Spellbook::initialize(objref item)
{
	shape = 1402;
	int width, height;
	gShapeManager.getShapeSize(&width, &height, shape);
	bounds.set(0, 0, width, height);
	add(&closeButton);
	add(&bookmark);
	bookmark.show();
	page = 0;
	for (int i = 0; i < 8; ++i) {
		spells[i] = new ImageButton(i + 33);
		if (spells[i] == 0)
			ReportOutOfNearMemory();
		add(spells[i]);
		spells[i]->setFrame(0);
	}
	add(&previous);
	add(&next);
	displayed = item;
	selectedSpell = SpellRef(displayed).selection();
	if (selectedSpell < 0 || (int)selectedSpell > 72)
		selectedSpell = 0;
	flipShape = 1400;
	dragging = 0;
	int x = MouseState_getX(GetLastMouseState());
	int y = GetLastMouseState()->y;
	if (x + 160 > 319)
		x = 159;
	if (y + 106 > 199)
		y = 93;
	moveTo(x, y);
	SpellbookTextPrinter.setFont(5);
	refresh(1);
	Control::show();
	updateSpells();
	paint(&Viewport);
	CopyFrameBuffer();
	turnTo(selectedSpell / 8);
	paint(&Viewport);
	CopyFrameBuffer();
}

void Spellbook::draw(View *target)
{
	if (visible) {
		if (page == 0)
			previous.hide();
		else
			previous.show();
		if ((int)page == 8)
			next.hide();
		else
			next.show();
		ShapeManager_draw(&gShapeManager, target, bounds.x, bounds.y, shape, 0, 0, 0);
	}
}

void Spellbook::paint(View *target)
{
	Control::paint(target);
	if (!dragging) {
		View *saved = SpellbookTextPrinter.target;
		SpellbookTextPrinter.target = target;
		int column = 0;
		int row = 0;
		int x, y, spell;
		unsigned count;
		int circle = spells[0]->getFrame();
		char text[20];
		if (circle != 0) {
			int width;
			y = bounds.y + 24;
			strncpy(text, GetGameText(2, circle + 69), 20);
			if (strlen(text) > 20)
				text[20] = 0;
			width = SpellbookTextPrinter.textWidth(text);
			x = bounds.x - (width >> 1) + 60;
			SpellbookTextPrinter.x = x;
			SpellbookTextPrinter.y = y;
			SpellbookTextPrinter.printString(text);
			strncpy(text, GetGameText(2, 69), 20);
			if (strlen(text) > 20)
				text[20] = 0;
			width = SpellbookTextPrinter.textWidth(text);
			x = bounds.x - (width >> 1) + 115;
			SpellbookTextPrinter.x = x;
			SpellbookTextPrinter.y = y;
			SpellbookTextPrinter.printString(text);
		}
		for (int i = 0; i < 8; ++i) {
			x = bounds.x + column * 54 + 71;
			y = bounds.y + row * 17 + 40;
			spell = circle * 8 + i;
			if (spell < 8)
				break;
			if (SpellRef(displayed).hasSpell(spell)) {
				count = SpellCastCounts[spell];
				if (count != 0) {
					sprintf(text, "%3d", count);
					SpellbookTextPrinter.x = x;
					SpellbookTextPrinter.y = y;
					SpellbookTextPrinter.printString(text);
				}
			}
			if (i == 3) {
				column = 1;
				row = 0;
			} else {
				row++;
			}
		}
		SpellbookTextPrinter.target = saved;
	}
}

void Spellbook::getMarkerPosition(int *x, int *y)
{
	int right;
	if (selectedSpell / 8 < page)
		right = 0;
	else if (selectedSpell / 8 > page)
		right = 1;
	else if (selectedSpell % 8 < 4)
		right = 0;
	else
		right = 1;
	if (!right)
		*x = 71;
	else
		*x = 119;
	*y = 92;
	if (selectedSpell / 8 == page)
		bookmark.setFrame(selectedSpell % 4 + 1);
	else
		bookmark.setFrame(0);
}

void Spellbook::updateSpells()
{
	for (int i = 0; i < 8; ++i) {
		int circle = spells[i]->getFrame();
		spells[i]->unlock();
		if (SpellRef(displayed).hasSpell(circle * 8 + i))
			spells[i]->show();
		else
			spells[i]->hide();
		spells[i]->lock();
	}
}

/* Flips page by page to the wanted circle, drawing the page-turn frames. */
void Spellbook::turnTo(int wanted)
{
	if (page != wanted) {
		int step, begin, end;
		if (page < wanted)
			step = 1;
		else
			step = -1;
		do {
			page += step;
			if (step == 1) {
				begin = 4;
				end = 8;
			} else {
				begin = 0;
				end = 4;
			}
			for (int i = begin; i < end; ++i)
				spells[i]->setFrame(page);
			updateSpells();
			if (step == -1) {
				begin = 0;
				end = 4;
			} else {
				begin = 3;
				end = -1;
			}
			dragging = 1;
			paint(&Viewport);
			dragging = 0;
			CopyFrameBuffer();
			hotX = 0;
			hotY = 0;
			allocate((bounds.width() + 1) * (bounds.height() + 1));
			show(bounds.x, bounds.y);
			for (i = begin; i != end; i -= step) {
				ShapeManager_draw(&gShapeManager, &Viewport, bounds.x + 87, bounds.y + 15, flipShape, i, 0, 0);
				CopyFrameBuffer();
				hide();
			}
			release();
			if (step == 1) {
				begin = 0;
				end = 4;
			} else {
				begin = 4;
				end = 8;
			}
			for (i = begin; i < end; ++i)
				spells[i]->setFrame(page);
			updateSpells();
			int markerX, markerY;
			getMarkerPosition(&markerX, &markerY);
			bookmark.moveTo(bounds.x + markerX, bounds.y + markerY);
			paint(&Viewport);
		} while (page != wanted);
	}
}

unsigned char Spellbook::handle(MouseState *event)
{
	int x = MouseState_getX(event);
	int y = event->y;
	unsigned char result = 0;
	unsigned char hit = gShapeManager.isCursorInBounds(shape, 0, Point(bounds.x, bounds.y), Point(x, y));
	if (result = closeButton.handle(event)) {
		switch (result) {
		case BUTTON_CLICKED:
			return GUMP_CLOSE;
		default:
			return result;
		}
	}
	if (PickingItem == 0) {
		if (bookmark.handle(event) && selectedSpell / 8 != page) {
			turnTo(selectedSpell / 8);
			return GUMP_HANDLED;
		} else if (next.isVisible() && next.handle(event) == BUTTON_CLICKED) {
			if (page < 8) {
				turnTo(page + 1);
				return GUMP_HANDLED;
			}
		} else if (previous.isVisible() && previous.handle(event) == BUTTON_CLICKED) {
			if (page > 0) {
				turnTo(page - 1);
				return GUMP_HANDLED;
			}
		}
		for (int i = 0; i < 8; ++i) {
			if (spells[i]->isVisible()) {
				if (result = spells[i]->handle(event)) {
					if (page * 8 + i != selectedSpell) {
						selectedSpell = page * 8 + i;
						int markerX, markerY;
						getMarkerPosition(&markerX, &markerY);
						bookmark.moveTo(bounds.x + markerX, bounds.y + markerY);
					}
					switch (result) {
					case BUTTON_CLICKED:
						return GUMP_HANDLED;
					case BUTTON_DOUBLE_CLICK:
						if (CanCastSpell(owner, selectedSpell, 1, 1))
							return GUMP_CAST_SPELL;
						ReportNoCanDo(0);
						return GUMP_HANDLED;
					}
				}
			}
		}
	}
	if (hit) {
		if (event->action == MOUSE_RELEASE)
			return GUMP_NO_DROP;
		if (event->action == MOUSE_CLICK) {
			hotX = x - bounds.x;
			hotY = y - bounds.y;
			return GUMP_MOVE;
		}
		return GUMP_HANDLED;
	}
	return 0;
}

void Spellbook::moveTo(int x, int y)
{
	bounds.moveTo(x, y);
	closeButton.moveTo(x + 23, y + 45);
	previous.moveTo(x + 43, y + 25);
	next.moveTo(x + 137, y + 25);
	int column = 0;
	int row = 0;
	for (int i = 0; i < 8; ++i) {
		spells[i]->moveTo(bounds.x + column * 54 + 82, bounds.y + row * 17 + 40);
		if (i == 3) {
			column = 1;
			row = 0;
		} else {
			row++;
		}
	}
	int markerX, markerY;
	getMarkerPosition(&markerX, &markerY);
	bookmark.moveTo(bounds.x + markerX, bounds.y + markerY);
}

unsigned char Spellbook::accepts(objref item, int x, int y) { return 0; }

objref Spellbook::selected()
{
	objref selected(0);
	return selected;
}

int Spellbook::mouseX() { return clickX; }

int Spellbook::mouseY() { return clickY; }

int Spellbook::dragX() { return dragOffsetX; }

int Spellbook::dragY() { return dragOffsetY; }

void Spellbook::setDragX(int x) { dragOffsetX = x; }

void Spellbook::setDragY(int y) { dragOffsetY = y; }

void Spellbook::refresh(char)
{
	objref item = displayed;
	/* climb to the person holding the book (type class 13) */
	do {
		item = Item_getContainer(&item);
	} while (!(unsigned char)(gItemTypeInfo[item.type()].typeClass == TYPE_CLASS_HUMAN) && item.valid());
	owner = NPCRef(item);
	reagents.countReagentsInPossession(owner);
}
