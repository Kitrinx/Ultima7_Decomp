#ifndef REDSCRN_H
#define REDSCRN_H

/* A picture read from a FLX file and drawn at 0,0, with a cycling palette. */
struct RedScreen {
	char visible;   /* on screen */
	int shapeBlock; /* segment holding the shape, -1 until it is read */
	RedScreen();
	~RedScreen();
	void show();
	void hide();
	void load();
	void stop();
	void paint();
};

extern RedScreen RedScreenPicture;

extern long RedScreenCycleRate;
extern int RedScreenShapeNumber;
extern char *EndshapeFileName;
void far CycleRedScreenPalette(void);

#endif
