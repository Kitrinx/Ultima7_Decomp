/* Black Gate U7.EXE, resident segment 8 (file offsets 0x00f312 to 0x00fbe4, 2258 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include <new>
#include "plat.h"
#include "arena.h"
#include "objref.h"
#include "lowlevel.h"
#include "view.h"
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
#include "u7event.h"
#include "palfade.h"
#include "uccomm5.h"
#include "camera.h"
#include "worldpal.h"

Camera gCamera;

WorldView MainWorldView;
int8_t FirstFramePending = 1;
int8_t CopyingFrame;

void DrawCeilingMask(void)
{
	int16_t top, right, bottom, left;
	int16_t half = CeilingZ / 2;
	WorldToCollisionCell(MainWorldView.centerX - 20 + half, MainWorldView.centerY - 12 + half, &left, &top, 0);
	WorldToCollisionCell(MainWorldView.centerX - 20 + half + 40, MainWorldView.centerY - 12 + half + 25,
		&right, &bottom, 0);
	int16_t py;
	uint32_t mask = INT32_C(1) << (++CeilingZ * 2);
	CeilingZ--;
	int16_t origin = (CeilingZ & 1) * 4 - 4;
	int16_t start;
	int16_t x, px;
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
				FillRectangle(&Viewport, start, py, px + 7, py + 7, 0);
			}
		}
	}
}

void CopyFrameBuffer(void)
{
	if (RemoteViewActive != 0)
		ShapeManager_draw(&gShapeManager, &Viewport, 159, 99, 1034, 0, 0, 0);
	CursorTracking = 0;
	DrawCursorInto(&Viewport);
	plat_video_wait_retrace();
	CopyingFrame = 1;
	CopyScreen(GetRowAddress(0, Viewport.rowTable), GetRowAddress(0, ScreenView.rowTable));
	if (!PlayerActionSuspended && GameScreen.fading == 0) {
		if (GameScreen.pal->changed() != 0)
			GameScreen.pal->apply(GameScreen.paletteMode);
		else
			SetPaletteRange(GameScreen.pal->colors, GameScreen.pal->order, 224, 31);
	}
	plat_video_present(ScreenPixels());
	CopyingFrame = 0;
	ClearSteppedNPCs();
	EraseCursorFrom(&Viewport);
	ShowCursor();
	CursorTracking = 1;
}

void FinishFrame(int8_t advance)
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
		LogMemoryUsage("mouse3");
		SetCursorTarget(&ScreenView);
		LogMemoryUsage("mouse4");
		CursorTracking = 0;
		SelectMouseCursor(!IsAvatarInCombat() ? 8 : 32);
		CursorTracking = 1;
	}
	CopyFrameBuffer();
}

void Camera::setOverlay(uint8_t enabled) { drawOverlay = enabled; }
Coord Camera::getCenterX() { return MainWorldView.centerX; }
Coord Camera::getCenterY() { return MainWorldView.centerY; }

int16_t IsWorldPosOnScreen(Coord a, Coord b)
{
	Coord x, y;
	int16_t dx, dy;
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

int16_t Camera::isOnScreen(Coord x, Coord y) { return IsWorldPosOnScreen(x, y); }

void Camera::toWorldCoords(int16_t x, int16_t y, Coord *outX, Coord *outY)
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
	int16_t screenX, screenY;
	objref item;
	Coord x, y;
	int16_t npcNum;

	beginFrame();
	FinishFrame(GameTime.hold == 0);
	if (ShowNpcNumbers) {
		for (npcNum = 0; npcNum < NPC_COUNT; npcNum++) {
			GetNpcIbo(&item, npcNum);
			x = MainWorldView.centerX + 19 - Item_getX(item);
			y = MainWorldView.centerY + 12 - Item_getY(item);
			screenX = x.value;
			screenY = y.value;
			if ((int8_t) (item.off != 0) && screenX >= 0 && screenX <= 39
				&& screenY >= 0 && screenY <= 24)
				ConsolePrintAt(40 - screenX, 25 - screenY, "%d", Item_getNpcNumber(&item));
		}
	}
	if (QueueToggle)
		DumpActionQueue();
	if (waitForKey)
		while (!KeyPressed())
			plat_yield();
}

void Camera_drawAt(Camera *self, CellCoord x, CellCoord y, uint8_t z)
{
	uint8_t saved;
	saved = CeilingZ;
	CeilingZ = z;
	self->moveTo(x, y);
	self->render();
	CeilingZ = saved;
}

void Camera_drawAt(Camera *self, Coord x, Coord y, ItemId item)
{
	self->moveToItem(x, y, item);
	self->render();
}

void DrawWorld(Camera *self)
{
	Camera_drawAt(self, Item_getX(self->target), Item_getY(self->target), (ItemId) self->target);
}

void Camera::setTarget(objref item) { target = item; }

void InitCamera(int16_t) {}

void InitChunkCache(void)
{
	int16_t i;
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

void CenterOnAvatar(void)
{
	MainWorldView.setCenter(Item_getX(AvatarRef), Item_getY(AvatarRef));
	UpdateCeiling((ItemId) AvatarRef);
	MainWorldView.setCenter(Item_getX(AvatarRef), Item_getY(AvatarRef));
}

extern "C" void ResetCameraGlobals(void)
{
	memset((void *)&gCamera, 0, sizeof(gCamera));
	memset((void *)&MainWorldView, 0, sizeof(MainWorldView));
	FirstFramePending = 1;
	CopyingFrame = 0;
}

extern "C" void ConstructCameraGlobals(void)
{
	new (&gCamera) Camera();
	new (&MainWorldView) WorldView();
}
