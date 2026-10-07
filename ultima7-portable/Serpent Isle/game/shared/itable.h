#ifndef SHARED_ITABLE_H
#define SHARED_ITABLE_H

struct View;

namespace Shared {

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
	View *getTarget() { return target; }
	void setTarget(View *view) { target = view; }
	void setSpacing(int16_t value) { spacing = value; }
	void setLeading(int16_t value) { leading = value; }
	void print(int16_t px, int16_t py, char *s) { move(px, py); printString(s); }
	void print(int16_t px, int16_t py, int8_t c) { move(px, py); printChar(c); }
};

/* Draws each character as a frame of a font shape; flags 0 means the printer owns the shape. */
class FontTextPrinter : public TextPrinter {
public:
	int16_t drawFlags;
	int32_t shape;
	int32_t getShape() { return shape; }
	FontTextPrinter() { shape = 0; }
	~FontTextPrinter() {}
	void reset() {}
	void setShape(int32_t data, int16_t flags);
	void drawChar(View *, int16_t, int16_t, int8_t);
	int16_t charHeight(int8_t);
	int16_t charWidth(int8_t);
	int16_t textWidth(char *);
	int16_t measure(char *text, int16_t count);
	int16_t textHeight(char *text);
	int16_t frameCount();
	void printFormatted(int16_t px, int16_t py, char *format, ...);
	int8_t hasShape() { return shape != 0; }
	int8_t ownsShape() { return drawFlags == 0; }
};

/* A font printer that loads its shape from a file or a Flex entry. */
class FlexTextPrinter : public FontTextPrinter {
public:
	FlexTextPrinter() {}
	void load(char *name);
	void load(char *flexName, int16_t entry);
	~FlexTextPrinter();
	void reset();
};

}

#endif
