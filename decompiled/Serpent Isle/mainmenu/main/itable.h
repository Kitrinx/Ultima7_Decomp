#ifndef ITABLE_H
#define ITABLE_H

struct View;

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
	View *getTarget() { return target; }
	void setTarget(View *view) { target = view; }
	void setSpacing(int value) { spacing = value; }
	void setLeading(int value) { leading = value; }
	void print(int px, int py, char *s) { move(px, py); printString(s); }
	void print(int px, int py, char c) { move(px, py); printChar(c); }
};

/* Draws each character as a frame of a font shape; flags 0 means the printer owns the shape. */
class FontTextPrinter : public TextPrinter {
public:
	int drawFlags;
	long shape;
	long getShape() { return shape; }
	FontTextPrinter() { shape = 0; }
	~FontTextPrinter() {}
	void reset() {}
	void setShape(long data, int flags);
	void drawChar(View *, int, int, char);
	int charHeight(char);
	int charWidth(char);
	int textWidth(char *);
	int measure(char *text, int count);
	int textHeight(char *text);
	int frameCount();
	void printFormatted(int px, int py, char *format, ...);
	char hasShape() { return shape != 0; }
	char ownsShape() { return drawFlags == 0; }
};

/* A font printer that loads its shape from a file or a Flex entry. */
class FlexTextPrinter : public FontTextPrinter {
public:
	FlexTextPrinter() {}
	void load(char *name);
	void load(char *flexName, int entry);
	~FlexTextPrinter();
	void reset();
};

#endif
