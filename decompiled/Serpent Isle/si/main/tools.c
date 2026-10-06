/* Serpent Isle SI.EXE, overlay segment 245 (file offsets 0x06a050 to 0x06a65a, 1546 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include <conio.h>
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

/* bounds the debug screen shows as <fn and >fn */
int LowFn = 8192;
int HighFn = 0;
char *WorldMapFileName = "WORLDMAP.VGA";

/* Whether a screen point lies on the map. */
unsigned char IsPointOnMap(int x, int y)
{
	return x >= 78 && x <= 241 && y >= 10 && y <= 189;
}

/* Shows map number map with a cross at the avatar. Map 0 always marks the avatar; map 1 only when
 * the party has a sextant and the avatar is outdoors; the others never do. */
void DrawWorldMap(int map, int *shape)
{
	int x, y;
	int none;

	switch (map) {
	case 0:
		ShapeManager_drawInViewport(&gShapeManager, 160, 100, 1046, 0, 0, 0);
		x = Item_getX(AvatarRef).value / 16 + 78;
		y = Item_getY(AvatarRef).value / 16 + 10;
		break;
	case 1:
		ShapeManager_drawInViewport(&gShapeManager, 160, 100, 1046, 0, 0, 0);
		x = Item_getX(AvatarRef).value / 16 + 78;
		y = Item_getY(AvatarRef).value / 16 + 10;
		break;
	case 2:
		ShapeManager_drawInViewport(&gShapeManager, 160, 100, 1081, 0, 0, 0);
		x = Item_getX(AvatarRef).value / 16 + 78;
		y = Item_getY(AvatarRef).value / 16 + 10;
		break;
	case 3:
		ShapeManager_drawInViewport(&gShapeManager, 160, 100, 1082, 0, 0, 0);
		x = Item_getX(AvatarRef).value / 16 + 78;
		y = Item_getY(AvatarRef).value / 16 + 10;
		break;
	case 4:
		ShapeManager_drawInViewport(&gShapeManager, 160, 100, 1083, 0, 0, 0);
		x = Item_getX(AvatarRef).value / 16 + 78;
		y = Item_getY(AvatarRef).value / 16 + 10;
		break;
	case 5:
		ShapeManager_drawInViewport(&gShapeManager, 160, 100, 1084, 0, 0, 0);
		x = Item_getX(AvatarRef).value / 16 + 78;
		y = Item_getY(AvatarRef).value / 16 + 10;
		break;
	case 6:
		ShapeManager_drawInViewport(&gShapeManager, 160, 100, 1076, 0, 0, 0);
		x = Item_getX(AvatarRef).value / 16 + 78;
		y = Item_getY(AvatarRef).value / 16 + 10;
		break;
	}
	none = 0;
	if (map == 0 || (CountHeldItems(1, none, 650, 255, 255) && CeilingZ == 15 && map == 1)) { /* sextant */
		DrawLine(&Viewport, x, y - 2, x, y + 2, 4);
		DrawLine(&Viewport, x - 2, y, x + 2, y, 4);
	}
	CopyFrameBuffer();
}

/* Shows the map and moves the avatar to the point clicked. */
void TeleportByMap(void)
{
	unsigned key;
	int mx, my;
	Coord x, y;
	unsigned char cursor;
	int shape;

	cursor = CursorBase;
	SelectMouseCursor(0);
	shape = 0;
	DrawWorldMap(0, &shape);
	FlushKeyboard();
	for (;;) {
		PollKeyAndTranslateWithMouse(&key, &mx, &my);
		if (key == 0x1b)            /* Escape */
			break;
		if (key == 0x201) {         /* left click */
			if (!IsPointOnMap(mx, my))
				break;
			x = (mx - 78) * 16;
			y = (my - 10) * 16;
			TeleportParty(Coord(x.value + 3), Coord(y.value + 3), 0);
			break;
		}
	}
	if (shape)
		gShapeManager.releaseBlock(shape);
	SelectMouseCursor(cursor);
}

/* Shows map number map + 1 until a key or a click. */
void ShowWorldMap(unsigned char map)
{
	unsigned key = 0;
	int mx, my;
	int shape;

	DrawWorldMap(map + 1, &shape);
	FlushKeyboard();
	do {
		PollKeyAndTranslateWithMouse(&key, &mx, &my);
	} while (key != 0x201 && key != 0x1b);  /* left click, Escape */
}

/* Scrolls the camera by keys or the mouse for count turns, or until stopped. */
void RunWizardEye(int count)
{
	unsigned key;
	int mx, my;
	int dx, dy;
	unsigned char dir;

	RemoteViewActive = 1;
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
			else if (key >= '1' && key <= '9')
				dir = DirectionForNumberKey[key - '1'];
			break;
		}
		if (dir != 255) {
			dx = DirectionDX[dir] * 4;
			dy = DirectionDY[dir] * 4;
		}
		Camera_drawAt(&gCamera, Coord(gCamera.getCenterX().value + dx), Coord(gCamera.getCenterY().value + dy), 15);
	}
	RemoteViewActive = 0;
}
