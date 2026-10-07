#ifndef SPELLBK_H
#define SPELLBK_H

#include "gumps.h"
#include "npcref.h"
#include "spell.h"

/* The spellbook gump. */
struct Spellbook : Gump {
	int16_t clickX, clickY;
	ReagentCounter reagents;
	ImageButton *spells[8];
	ImageButton bookmark, previous, next;
	int8_t page, selectedSpell;
	int16_t flipShape;
	NPCRef owner;
	int16_t dragOffsetX, dragOffsetY;
	Spellbook(objref);
	~Spellbook();
	void initialize(objref);
	void getMarkerPosition(int16_t *, int16_t *);
	void updateSpells();
	void turnTo(int16_t);
	void paint(View *);
	uint8_t handle(MouseState *);
	void draw(View *);
	void moveTo(int16_t, int16_t);
	uint8_t accepts(objref, int16_t, int16_t);
	objref selected();
	int16_t mouseX();
	int16_t mouseY();
	int16_t dragX();
	int16_t dragY();
	void setDragX(int16_t);
	void setDragY(int16_t);
	void refresh(int8_t);
	uint8_t findPosition(objref, int16_t *, int16_t *);
};

struct ProportionalTextPrinter;

extern ProportionalTextPrinter SpellbookTextPrinter;

#endif
