/* Serpent Isle SI.EXE, overlay segment 223 (file offsets 0x05e2c0 to 0x05e48d, 461 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "objref.h"
#include "iteminfo.h"
#include "coord.h"
#include "u7manage.h"
#include "u7point.h"
#include "mouse.h"
#include "u7event.h"
#include "bogus.h"
#include "item.h"
#include "target.h"
#include "camera.h"
#include "gumpmgr.h"
#include "mevent.h"
#include <conio.h>

#define IS_VALID(r) ((char) ((r) != 0))
#define IS_EVENT(s, e) ((unsigned char) ((s).action == (e)))
#define CLICKED(s) ((char) (((s).action == MOUSE_CLICK || (s).action == MOUSE_DOUBLE_CLICK) && (s).button == 1))

/* Let the player pick an object, or a spot, with the mouse. */
char far HavePlayerSelect(objref *picked, Coord *px, Coord *py, int *pz)
{
	unsigned char done = 0;
	int obj;
	int x, y;
	MouseState state;
	unsigned char cursor = CursorBase;

	SelectMouseCursor(2);
	*picked = 0;
	GameInput.enableKeyboardMouse();
	while (!done) {
		UpdateAndCopyMouseState(&state);
		if (IS_VALID(state.action) && !IS_EVENT(state, 4)) {
			x = MouseState_getX(&state);
			y = state.y;
			if (GameInput.isGumpMode())
				done = ProcessDialogInput(&state, 1, (objref *)&obj);
			else if (CLICKED(state)) {
				FindItemAtScreenPoint((objref *)&obj, x, y, &MainWorldView, &gShapeManager, 0);
				while (!GameInput.isButtonReleased(1))
					;
				done = 1;
			}
		}
	}
	SelectMouseCursor(cursor);
	if (IS_VALID(obj)) {
		*px = Item_getX((objref &)obj);
		*py = Item_getY((objref &)obj);
		*pz = Item_getZ((objref *)&obj);
		*picked = obj;
	} else
		PickWorldCoords(x, y, px, py, pz, 1);
	if (picked->valid())
		return 1;
	return 0;
}

/* Drop a held mouse button and any keys typed ahead. */
void far FlushPlayerInput(void)
{
	SkipToMouseRelease();
	while (kbhit())
		getch();
}
