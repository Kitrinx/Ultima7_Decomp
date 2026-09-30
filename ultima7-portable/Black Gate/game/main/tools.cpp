/* Black Gate U7.EXE, overlay segment 264 (file offsets 0x07c510 to 0x07ca74, 1380 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "main.h"
#include "plat.h"
#include "objref.h"
#include "view.h"
#include "u7manage.h"
#include "coord.h"
#include "camera.h"
#include "itable.h"
#include "init.h"
#include "bltshape.h"
#include "bogus.h"
#include "item.h"
#include "mainctrl.h"
#include "u7point.h"
#include "uccomm5.h"
#include "mouse.h"
#include "partymov.h"
#include "u7event.h"
#include "party.h"
#include "mapview.h"
#include "tools.h"

extern objref AvatarRef;

extern uint8_t VerifyItemBuffer(void);

int16_t unused_global_2 = 8192;
char *const WorldMapFileName = "WORLDMAP.VGA";

/* Whether a screen point lies on the map. */
uint8_t IsPointOnMap(int16_t x, int16_t y)
{
	return x >= 65 && x <= 254 && y >= 5 && y <= 194;
}

/* Shows the map with a cross at the avatar; with check set, only when the party has a sextant and
 * the avatar is outdoors. */
void DrawWorldMap(int16_t check, int16_t *shape)
{
	int16_t x, y;
	int16_t none;

	ShapeManager_drawInViewport(&gShapeManager, 160, 100, 1046, 0, 0, 0);
	{
		/* the avatar's place on the map */
		x = Item_getX(AvatarRef).value / 16 + 65;
		y = Item_getY(AvatarRef).value / 16 + 5;
	}
	none = 0;
	if (check == 0 || (CountHeldItems(1, none, 650, 255, 255) && CeilingZ == 15)) { /* sextant */
		DrawLine(&Viewport, x, y - 2, x, y + 2, 4);
		DrawLine(&Viewport, x - 2, y, x + 2, y, 4);
	}
	CopyFrameBuffer();
}

/* Shows the map and moves the avatar to the point clicked. */
void TeleportByMap(void)
{
	uint16_t key;
	int16_t mx, my;
	Coord x, y;
	uint8_t cursor;
	int16_t shape;

	cursor = CursorBase;
	SelectMouseCursor(0);
	shape = 0;
	DrawWorldMap(0, &shape);
	FlushKeyboard();
	for (;;) {
		plat_yield();
		PollKeyAndTranslateWithMouse(&key, &mx, &my);
		if (key == 0x1b)            /* Escape */
			break;
		if (key == 0x201) {         /* left click */
			if (!IsPointOnMap(mx, my))
				break;
			x = (mx - 65) * 16;
			y = (my - 5) * 16;
			TeleportParty(Coord(x.value + 3), Coord(y.value + 3), 0);
			break;
		}
	}
	if (shape)
		gShapeManager.releaseBlock(shape);
	SelectMouseCursor(cursor);
}

/* Shows the map until a key or a click. */
void ShowWorldMap(void)
{
	uint16_t key = 0;
	int16_t mx, my;
	int16_t shape;

	DrawWorldMap(1, &shape);
	FlushKeyboard();
	do {
		plat_yield();
		PollKeyAndTranslateWithMouse(&key, &mx, &my);
	} while (key != 0x201 && key != 0x1b);  /* left click, Escape */
}

/* Scrolls the camera by keys or the mouse for count turns, or until stopped. */
void RunWizardEye(int16_t count)
{
	uint16_t key;
	int16_t mx, my;
	int16_t dx, dy;
	uint8_t dir;

	RemoteViewActive = 1;
	/* Each pass is a whole world draw; a period PC managed about ten a second. */
	WaitForFrameTime();
	Camera_drawAt(&gCamera, gCamera.getCenterX(), gCamera.getCenterY(), 15);
	while (count != 0) {
		count--;
		PollKeyAndTranslateWithMouse(&key, &mx, &my);
		dir = 255;
		dx = dy = 0;
		switch (key) {
		case 0x202:     /* right button pressed or held */
		case 0x206:
			dir = (CursorFrame - CursorBase) & 7;
			break;
		case 0x1b:      /* Escape, left click or double-click */
		case 0x201:
		case 0x203:
			count = 0;
			break;
		default:
			if (key >= KEY_HOME && key <= KEY_PAGE_DOWN)   /* Home to PgDn on the keypad */
				dir = DirectionForDirectionKey[key - KEY_HOME];
			else if (key >= (uint16_t)('1') && key <= (uint16_t)('9'))
				dir = DirectionForNumberKey[key - '1'];
			break;
		}
		if (dir != 255) {
			dx = DirectionDX[dir] * 4;
			dy = DirectionDY[dir] * 4;
		}
		WaitForFrameTime();
		Camera_drawAt(&gCamera, Coord(gCamera.getCenterX().value + dx), Coord(gCamera.getCenterY().value + dy), 15);
	}
	RemoteViewActive = 0;
}

/* Reports a corrupt game and, after a key or a click, quits to DOS. */
void CheckItemBuffer(void)
{
	int16_t x, y;
	uint8_t done;
	MouseState *m;

	if (!VerifyItemBuffer()) {
		YellowTextPrinter.setFont(0);
		for (x = 2; x < 39; x++)
			for (y = 2; y < 24; y++) {
				if (x == 38 || y == 23) {
					if (x == 38 && y > 2 || x > 2 && y == 23)
						ShapeManager_drawAtCell(&gShapeManager, 48, 4, x, y, 0);
				} else
					ShapeManager_drawAtCell(&gShapeManager, 23, 0, x, y, 0);
			}
		YellowTextPrinter.target = &Viewport;
		YellowTextPrinter.x = 30;
		YellowTextPrinter.y = 60;
		YellowTextPrinter.printString("Ultima VII self-check reveals a corrupt");
		YellowTextPrinter.x = 30;
		YellowTextPrinter.y = 72;
		YellowTextPrinter.printString("game.  This occurs rarely, but may be");
		YellowTextPrinter.x = 30;
		YellowTextPrinter.y = 84;
		YellowTextPrinter.printString("corrected by reverting to an earlier");
		YellowTextPrinter.x = 30;
		YellowTextPrinter.y = 96;
		YellowTextPrinter.printString("saved game.");
		YellowTextPrinter.x = 70;
		YellowTextPrinter.y = 130;
		YellowTextPrinter.printString("Hit any key now to revert");
		YellowTextPrinter.x = 70;
		YellowTextPrinter.y = 142;
		YellowTextPrinter.printString("and correct the problem.");
		CopyFrameBuffer();
		done = 0;
		do {
			plat_yield();
			m = UpdateAndGetMouseState();
			if (m->clicked())
				done = 1;
			else if (KeyPressed()) {
				done = 1;
				while (KeyPressed())
					ReadKey();
			}
		} while (!done);
		QuitToDos();
	}
}

extern "C" void ResetToolsGlobals(void)
{
	unused_global_2 = 8192;
}
