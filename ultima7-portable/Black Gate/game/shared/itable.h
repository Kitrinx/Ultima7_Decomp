#ifndef SHARED_ITABLE_H
#define SHARED_ITABLE_H

/* The TextPrinter base is U7's. */
#include "../main/itable.h"

namespace Shared {

/* Draws each character as a frame of a font shape, held at a linear address; flags 0 means the
 * printer owns the shape. */
class FontTextPrinter : public TextPrinter {
public:
	int16_t drawFlags;
	int32_t shape;
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
	char hasShape() { return shape != 0; }
	char ownsShape() { return drawFlags == 0; }
};

/* A font printer that loads its shape into Voodoo memory from a file or a Flex entry. */
class FlexTextPrinter : public FontTextPrinter {
public:
	void load(char *name);
	void load(char *flexName, int16_t entry);
	~FlexTextPrinter();
	void reset();
};

}

#endif
