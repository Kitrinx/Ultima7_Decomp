/* Black Gate U7.EXE, overlay segment 234 (file offsets 0x06c740 to 0x06c8f3, 435 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "plat.h"
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

#define IS_VALID(r) ((int8_t) ((r) != 0))
#define IS_EVENT(s, e) ((uint8_t) ((s).action == (e)))
#define CLICKED(s) ((int8_t) (((s).action == MOUSE_CLICK || (s).action == MOUSE_DOUBLE_CLICK) && (s).button == 1))

/* Let the player pick an object, or a spot, with the mouse. */
int8_t HavePlayerSelect(objref *picked, Coord *px, Coord *py, int16_t *pz)
{
	uint8_t done = 0;
	int16_t obj;
	int16_t x, y;
	MouseState state;
	uint8_t cursor = CursorBase;

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
					plat_yield();
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
