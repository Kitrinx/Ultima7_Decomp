/* Serpent Isle ENDGAME.EXE, resident segment 22 (file offsets 0x00bf9f to 0x00bff1, 82 bytes).
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
