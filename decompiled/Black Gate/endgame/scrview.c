/* Black Gate ENDGAME.EXE, resident segment 24 (file offsets 0x00c69b to 0x00c6ed, 82 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include "view.h"

View *CurrentView = 0;
View *ScreenView = 0;

/* Makes the screen view, once, and draws on it from then on. */
void ScreenMaker::open()
{
	if (ScreenView == 0) {
		ScreenView = new View;
		ScreenView->initScreen(0);
		CurrentView = ScreenView;
	}
}
