/* Serpent Isle SI.EXE, resident segment 39 (file offsets 0x01d2f6 to 0x01d38a, 148 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "objref.h"
#include "maps.h"
#include "coord.h"
#include "mapview.h"
#include "item.h"
#include "loadreg.h"

uint8_t ReloadingTerrain = 0;
uint8_t unused_global_4 = 0xff;
extern objref AvatarRef;

/* Reloads the terrain of the four loaded regions and recentres the view on the avatar. */
void ReloadRegionTerrain(void)
{
	uint8_t region[2];
	int16_t i;

	RegionsChanged = 1;
	for (i = 0; i < 4; i++) {
		if (LoadedRegions[i] != 255) {
			ReloadingTerrain = 1;
			region[0] = LoadedRegions[i];
			LoadRegionMap(region, MainWorldView.terrain + i);
		}
	}
	MainWorldView.setCenter(Item_getX(AvatarRef), Item_getY(AvatarRef));
	ReloadingTerrain = 0;
}

extern "C" void ResetU7reloadGlobals(void)
{
	ReloadingTerrain = 0;
	unused_global_4 = 0xff;
}
