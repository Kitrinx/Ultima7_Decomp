#ifndef WORLDPAL_H
#define WORLDPAL_H

#include "creeper.h"

/* which palette the screen shows */
#define PAL_DAY          1
#define PAL_DUSK         2
#define PAL_NIGHT        4
#define PAL_LIGHT_SPELL  8
#define PAL_LIGHT_SOURCE 16
#define PAL_INVISIBLE    32
#define PAL_FADED_OUT    64
#define PAL_OVERCAST     128
#define PAL_RESET        255
#define PAL_HELD         256

/* the fade or flash in progress */
#define FADE_DAMAGE    1
#define FADE_QUICK_IN  2
#define FADE_LIGHTNING 3
#define FADE_OUT       4
#define FADE_IN        5

/* how often a palette change takes its next step */
#define STEP_EVERY_TICK     0
#define STEP_EVERY_2_TICKS  1
#define STEP_EVERY_5_TICKS  2
#define STEP_EVERY_10_TICKS 3
#define STEP_EVERY_12_TICKS 4
#define STEP_EVERY_MINUTE   5

/* the screen palette: follows the time of day, weather, dungeons and light, and runs the fades */
struct ScreenPalette : Creeper {
	uint8_t enabled;
	int16_t fadedOut, state;
	uint8_t weather;
	int16_t unusedField1;
	char unusedField2[2];
	uint8_t lightSpellActive;
	int16_t lightSpellTime, lightLevel;
	uint8_t unusedField3, overridePalette, cycleOnly, stepRate;
	int16_t lastMinute;
	uint32_t lastTime;
	uint8_t currentWeather;
	int16_t pendingUpdate;
	uint8_t fadeType, fading;
	int16_t fadeSteps;
	uint8_t suspended, currentDungeon, previousDungeon;
	int16_t lightBand;
	ScreenPalette();
	void forceUpdate(int16_t ticks);
	void update();
	void updateWeather();
	void startDamageFlash();
	void showDay();
	void showDusk();
	uint8_t isDark();
	uint8_t checkDungeon();
	void updateDungeon();
	void showNight();
	void restoreWeather();
	void restore();
	void advanceToClock();
	void advanceTo(uint16_t hour, uint16_t minute);
	void selectCurrent();
	void updateTime();
	void endLight();
	void updateLight();
	void clearLight();
	void updateLightLevel();
	void endLightLevel();
	void setLightLevel(int16_t level);
	void showInvisiblePalette();
	void fadeOut();
	void startQuickFadeIn();
	void startFadeInOver(int16_t count);
	void flash();
	void fadeIn();
	void suspend();
	void resume();
	void hold();
	void finishFadeOut();
	void finishFade(int8_t daylightOnly);
	void reset();
};

extern ScreenPalette GameScreen;

#endif
