/* Black Gate U7.EXE, resident segment 63 (file offsets 0x025c98 to 0x025d2c, 148 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "objref.h"
#include "maps.h"
#include "coord.h"
#include "mapview.h"
#include "item.h"
#include "loadreg.h"

unsigned char ReloadingTerrain = 0;
unsigned char unused_global_4 = 0xff;
extern objref AvatarRef;

/* Reloads the terrain of the four loaded regions and recentres the view on the avatar. */
void ReloadRegionTerrain(void)
{
	unsigned char region[2];
	int i;

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
