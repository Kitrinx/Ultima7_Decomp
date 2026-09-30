#ifndef ITABLE_H
#define ITABLE_H

struct View;
struct objref;

/* Draws text at (x, y) into a view; subclasses supply the font. */
class TextPrinter {
public:
	int x, y, spacing, leading;
	View *target;
	TextPrinter();
	virtual void drawChar(View *, int, int, char) = 0;
	virtual ~TextPrinter();
	virtual void reset() = 0;
	virtual int charHeight(char) = 0;
	virtual int charWidth(char) = 0;
	virtual int textWidth(char *);
	void printChar(char);
	void printString(char *);
	void move(int nx, int ny) { x = nx; y = ny; }
	void print(int px, int py, char *s) { x = px; y = py; printString(s); }
};

/* Draws each character as a frame of a font shape. */
class FontTextPrinter : public TextPrinter {
public:
	int font;
	FontTextPrinter() { font = -1; }
	~FontTextPrinter();
	void drawChar(View *, int, int, char);
	void reset();
	void setShape(int);
	int charHeight(char);
	int charWidth(char);
};

/* A font printer with per-font letter and line spacing. */
class ProportionalTextPrinter : public FontTextPrinter {
public:
	char unusedField1;
	~ProportionalTextPrinter();
	void drawChar(View *, int, int, char);
	void setFont(unsigned char);
	int charWidth(char);
	int charHeight(char);
	int textWidth(char *);
};

extern ProportionalTextPrinter YellowTextPrinter;

void far HidePointer(void);
void far ShowPointer(void);
void far SetPointerZ(int);
unsigned GetCursorLength();
extern "C" void far RunItemScript(unsigned char *, objref);
void far StepItem(objref, unsigned char, int, int);

void UpdateNPCStatus();
void ShiftPointerZ(char value);
void HideMarkerShapes();
unsigned char PostItemScript(unsigned char *script, objref ref);
unsigned char PostStepScript(objref ref, unsigned char dir, int dz, int count);

#endif
