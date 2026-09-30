#ifndef REDSCRN_H
#define REDSCRN_H

/* A picture read from a FLX file and drawn at 0,0, with a cycling palette. */
struct RedScreen {
	int8_t visible;   /* on screen */
	int16_t shapeBlock; /* segment holding the shape, -1 until it is read */
	RedScreen();
	~RedScreen();
	void show();
	void hide();
	void load();
	void stop();
	void paint();
};

extern RedScreen RedScreenPicture;

extern int32_t RedScreenCycleRate;
extern int16_t RedScreenShapeNumber;
extern char *EndshapeFileName;
void CycleRedScreenPalette(void);

#endif
