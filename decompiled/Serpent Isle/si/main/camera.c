/* Serpent Isle SI.EXE, resident segment 7 (file offsets 0x00e33c to 0x00ec3d, 2305 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include <conio.h>
#include "objref.h"
#include "lowlevel.h"
#include "coord.h"
#include "collide.h"
#include "crawpal.h"
#include "u7manage.h"
#include "redscrn.h"
#include "mapview.h"
#include "gtimer.h"
#include "sprite.h"
#include "bltshape.h"
#include "bogus.h"
#include "collgrid.h"
#include "combat.h"
#include "itemcmd.h"
#include "sysusage.h"
#include "vidmode.h"
#include "mouse.h"
#include "actqueue.h"
#include "cheat.h"
#include "item.h"
#include "npcref.h"
#include "u7npc.h"
#include "itemovr2.h"
#include "preload.h"
#include "u7point.h"
#include "palfade.h"
#include "uccomm5.h"
#include "camera.h"
#include "worldpal.h"

/* A flat copy of the screen view: RemapView takes it by value, pushed word by word. */
struct View {
	int id;
	long pixels;
	int left, top, right, bottom;
	View();
};

Camera gCamera;
extern View Viewport;
extern View ScreenView;
extern "C" void far FillRectangle(View *view, int x0, int y0, int x1, int y1, char color);

WorldView MainWorldView;
char FirstFramePending = 1;
char CopyingFrame;

void far DrawCeilingMask(void)
{
	int top, right, bottom, left;
	int half = CeilingZ / 2;
	WorldToCollisionCell(MainWorldView.centerX - 20 + half, MainWorldView.centerY - 12 + half, &left, &top, 0);
	WorldToCollisionCell(MainWorldView.centerX - 20 + half + 40, MainWorldView.centerY - 12 + half + 25,
		&right, &bottom, 0);
	int py;
	unsigned long mask = 1L << (++CeilingZ * 2);
	CeilingZ--;
	int origin = (CeilingZ & 1) * 4 - 4;
	int start;
	int x, px;
	right++;
	bottom++;
	for (py = origin; top < bottom; top++, py += 8) {
		px = origin;
		x = left;
		while (x < right) {
			while (x < right && (GetCollisionCell(top, x) & mask)) {
				x++;
				px += 8;
			}
			if (x < right) {
				start = px;
				while (x < right && !(GetCollisionCell(top, x) & mask)) {
					x++;
					px += 8;
				}
				switch (InDungeon) {
				case 2:
					FillRectangle(&Viewport, start, py, px + 7, py + 7, 73);
					break;
				case 3:
					break;
				default:
					FillRectangle(&Viewport, start, py, px + 7, py + 7, 0);
					break;
				}
			}
		}
	}
}

void far CopyFrameBuffer(void)
{
	if (RemoteViewActive != 0)
		ShapeManager_draw(&gShapeManager, &Viewport, 159, 99, 1034, 0, 0, 0);
	CursorTracking = 0;
	DrawCursorInto(&Viewport);
	WaitForRetrace();
	CopyingFrame = 1;
	CopyScreen(GetRowAddress(0, Viewport.pixels), GetRowAddress(0, ScreenView.pixels));
	if (!PlayerActionSuspended && GameScreen.fading == 0) {
		if (GameScreen.pal->changed() != 0)
			GameScreen.pal->apply(GameScreen.paletteMode);
		else
			SetPaletteRange(GameScreen.pal->colors, GameScreen.pal->order, 224, 31);
	}
	CopyingFrame = 0;
	ClearSteppedNPCs();
	EraseCursorFrom(&Viewport);
	ShowCursor();
	CursorTracking = 1;
}

void far FinishFrame(char advance)
{
	AdvanceWeather(advance);
	UpdateSprites(advance, gSpriteManager.sprites, gSpriteManager.capacity);
	if (advance != 0) {
		if (EarthquakeCount != 0)
			ShakeScreenForQuake();
		if (!PlayerActionSuspended && FirstFramePending == 0)
			GameScreen.update();
	}
	if (FirstFramePending == 1) {
		FirstFramePending = 0;
		RedScreenPicture.hide();
		LogMemoryUsage(GetGameText(3, 215));
		SetCursorTarget(&ScreenView);
		LogMemoryUsage(GetGameText(3, 216));
		CursorTracking = 0;
		SelectMouseCursor(!IsAvatarInCombat() ? 8 : 32);
		CursorTracking = 1;
	}
	CopyFrameBuffer();
}

void Camera::setOverlay(unsigned char enabled) { drawOverlay = enabled; }
Coord Camera::getCenterX() { return MainWorldView.centerX; }
Coord Camera::getCenterY() { return MainWorldView.centerY; }

int far IsWorldPosOnScreen(Coord a, Coord b)
{
	Coord x, y;
	int dx, dy;
	x.value = a.value;
	dx = CompareWorldCoords(&MainWorldView.centerX, &x);
	if ((dx < 0 ? -dx : dx) <= 20) {
		y.value = b.value;
		dy = CompareWorldCoords(&MainWorldView.centerY, &y);
		if ((dy < 0 ? -dy : dy) <= 12)
			return 1;
	}
	return 0;
}

int Camera::isOnScreen(Coord x, Coord y) { return IsWorldPosOnScreen(x, y); }

void Camera::toWorldCoords(int x, int y, Coord *outX, Coord *outY)
{
	*outX = x;
	*outY = y;
}

void Camera::moveTo(Coord x, Coord y)
{
	if (!locked)
		MainWorldView.setCenter(x, y);
}

void Camera::moveToItem(Coord x, Coord y, ItemId item)
{
	UpdateCeiling(item);
	moveTo(x, y);
}

void Camera::beginFrame()
{
	MainWorldView.paint();
	if (InDungeon && RemoteViewActive == 0)
		DrawCeilingMask();
	if (drawOverlay)
		RemapView(Viewport, gShapeManager.translations + 512);
}

void Camera::render()
{
	int screenX, screenY;
	objref item;
	Coord x, y;
	int npcNum;

	beginFrame();
	FinishFrame(GameTime.hold == 0);
	if (ShowNpcNumbers) {
		for (npcNum = 0; npcNum < NPC_COUNT; npcNum++) {
			GetNpcIbo(&item, npcNum);
			x = MainWorldView.centerX + 19 - Item_getX(item);
			y = MainWorldView.centerY + 12 - Item_getY(item);
			screenX = x.value;
			screenY = y.value;
			if ((char) (item.off != 0) && screenX >= 0 && screenX <= 39
				&& screenY >= 0 && screenY <= 24)
				ConsolePrintAt(40 - screenX, 25 - screenY, "%d", Item_getNpcNumber(&item));
		}
	}
	if (QueueToggle)
		DumpActionQueue();
	if (waitForKey)
		while (!kbhit())
			;
}

void far Camera_drawAt(Camera *self, CellCoord x, CellCoord y, unsigned char z)
{
	unsigned char saved;
	saved = CeilingZ;
	CeilingZ = z;
	self->moveTo(x, y);
	self->render();
	CeilingZ = saved;
}

void far Camera_drawAt(Camera *self, Coord x, Coord y, ItemId item)
{
	self->moveToItem(x, y, item);
	self->render();
}

void far DrawWorld(Camera *self)
{
	Camera_drawAt(self, Item_getX(self->target), Item_getY(self->target), (ItemId) self->target);
}

void Camera::setTarget(objref item) { target = item; }

void far InitCamera(int) {}

void far InitChunkCache(void)
{
	int i;
	for (i = 0; i < 128; i++) {
		MainWorldView.cache.id[i] = -1;
		MainWorldView.cache.age[i] = i;
	}
}

void InitMap(void)
{
	MainWorldView.renderer.shapes = &gShapeManager;
	MainWorldView.renderer.savedState[0] |= 4;
	InitItemManager(&MainWorldView);
}

void far CenterOnAvatar(void)
{
	MainWorldView.setCenter(Item_getX(AvatarRef), Item_getY(AvatarRef));
	UpdateCeiling((ItemId) AvatarRef);
	MainWorldView.setCenter(Item_getX(AvatarRef), Item_getY(AvatarRef));
}
