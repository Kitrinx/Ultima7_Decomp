#ifndef SPELLBK_H
#define SPELLBK_H

#include "gumps.h"
#include "npcref.h"
#include "spell.h"

/* The spellbook gump. */
struct Spellbook : Gump {
	int clickX, clickY;
	ReagentCounter reagents;
	ImageButton *spells[8];
	ImageButton bookmark, previous, next;
	char page, selectedSpell;
	int flipShape;
	NPCRef owner;
	int dragOffsetX, dragOffsetY;
	Spellbook(objref);
	~Spellbook();
	void initialize(objref);
	void getMarkerPosition(int *, int *);
	void updateSpells();
	void turnTo(int);
	void paint(View *);
	unsigned char handle(MouseState *);
	void draw(View *);
	void moveTo(int, int);
	unsigned char accepts(objref, int, int);
	objref selected();
	int mouseX();
	int mouseY();
	int dragX();
	int dragY();
	void setDragX(int);
	void setDragY(int);
	void refresh(char);
	unsigned char findPosition(objref, int *, int *);
};

struct ProportionalTextPrinter;

extern ProportionalTextPrinter SpellbookTextPrinter;

#endif
