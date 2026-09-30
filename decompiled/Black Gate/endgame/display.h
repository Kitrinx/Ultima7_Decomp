#ifndef DISPLAY_H
#define DISPLAY_H

#include "shutdown.h"
#include "vidmode.h"
#include "view.h"

#define TEXT_MODE       3
#define GRAPHICS_MODE   0x13

/* A BIOS video mode number. */
struct VideoMode {
	unsigned char mode;
	VideoMode() { mode = TEXT_MODE; }
	void set(unsigned char m) { mode = m; }
};

/* The mode found at start, put back however the program ends. */
struct SavedMode : VideoMode, ShutdownHook {
	SavedMode() { GetVideoMode(&mode); }
	void shutdown();
};

/* Mode 13h for the ending, and the text screen back afterwards. */
struct Display : ShutdownHook {
	ScreenMaker screen;
	SavedMode saved;
	Display() {}
	~Display() { close(); }
	void open();
	void close();
	void shutdown();
};

extern Display Screen;
extern unsigned char GraphicsOn;

#endif
