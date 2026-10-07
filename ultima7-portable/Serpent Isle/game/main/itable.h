#ifndef ITABLE_H
#define ITABLE_H

struct View;
struct objref;

/* Draws text at (x, y) into a view; subclasses supply the font. */
class TextPrinter {
public:
	int16_t x, y, spacing, leading;
	View *target;
	TextPrinter();
	virtual void drawChar(View *, int16_t, int16_t, int8_t) = 0;
	virtual ~TextPrinter();
	virtual void reset() = 0;
	virtual int16_t charHeight(int8_t) = 0;
	virtual int16_t charWidth(int8_t) = 0;
	virtual int16_t textWidth(char *);
	void printChar(int8_t);
	void printString(char *);
	void move(int16_t nx, int16_t ny) { x = nx; y = ny; }
	void print(int16_t px, int16_t py, char *s) { x = px; y = py; printString(s); }
};

/* Draws each character as a frame of a font shape. */
class FontTextPrinter : public TextPrinter {
public:
	int16_t font;
	FontTextPrinter() { font = -1; }
	~FontTextPrinter();
	void drawChar(View *, int16_t, int16_t, int8_t);
	void reset();
	void setShape(int16_t);
	int16_t charHeight(int8_t);
	int16_t charWidth(int8_t);
};

/* A font printer with per-font letter and line spacing. */
class ProportionalTextPrinter : public FontTextPrinter {
public:
	int8_t unusedField1;
	~ProportionalTextPrinter();
	void drawChar(View *, int16_t, int16_t, int8_t);
	void setFont(uint8_t);
	int16_t charWidth(int8_t);
	int16_t charHeight(int8_t);
	int16_t textWidth(char *);
};

extern ProportionalTextPrinter YellowTextPrinter;

void HidePointer(void);
void ShowPointer(void);
void SetPointerZ(int16_t);
uint16_t GetCursorLength();
extern "C" void RunItemScript(uint8_t *, objref);
void StepItem(objref, uint8_t, int16_t, int16_t);

void UpdateNPCStatus();
int16_t GetClothingWarmth(objref npc);
void ShiftPointerZ(int8_t value);
void HideMarkerShapes();
uint8_t PostItemScript(uint8_t *script, objref ref);
uint8_t PostStepScript(objref ref, uint8_t dir, int16_t dz, int16_t count);

#endif
