/* Serpent Isle SI.EXE, resident segment 87 (file offsets 0x0337c4 to 0x03386f, 171 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "worldpal.h"
#include "palctrl.h"

/* C entry points into the palette object */
extern "C" {

void FlashDamagePalette(void)
{
	GameScreen.startDamageFlash();
}

void StartPaletteFadeIn(void)
{
	GameScreen.startQuickFadeIn();
}

void StartPaletteFadeOut(void)
{
	GameScreen.fadeOut();
}

void SetLightLevel(int level)
{
	GameScreen.lightLevel = level;
}

void FlashLightning(void)
{
	GameScreen.flash();
}

void MarkPaletteFadedOut(void)
{
	GameScreen.enabled = 0;
	GameScreen.fadedOut = 12;
}

void MarkPaletteFadedIn(void)
{
	GameScreen.fadedOut = 0;
	GameScreen.enabled = 1;
}

void SetLightSpellTime(int duration)
{
	GameScreen.lightSpellTime = duration;
}

void SetTimePalette(void)
{
	GameScreen.update();
}

void SetFixedPalette(char daylightOnly)
{
	GameScreen.finishFade(daylightOnly);
}

void RestoreGamePalette(void)
{
	GameScreen.reset();
}

}
