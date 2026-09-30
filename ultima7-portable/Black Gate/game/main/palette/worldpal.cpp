/* Black Gate U7.EXE, resident segment 114 (file offsets 0x0382ec to 0x03969e, 5042 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "crawpal.h"
#include "gtimer.h"
#include "weather.h"
#include "random.h"
#include "item.h"
#include "collide.h"
#include "worldpal.h"

extern objref AvatarRef;

inline int8_t IsNight()
{
	return GameTime.getHour() < 5 || GameTime.getHour() > 20;
}

ScreenPalette::ScreenPalette()
{
	lightBand = 0;
	previousDungeon = 0;
	currentDungeon = 0;
	fadedOut = 0;
	enabled = 1;
	fadeType = 0;
	fading = 0;
	fadeSteps = 0;
	lastTime = 0;
	stepRate = STEP_EVERY_MINUTE;
	lastMinute = 0;
	cycleOnly = 0;
	state = 0;
	lightLevel = 0;
	currentWeather = 0;
	weather = 0;
	unusedField1 = 0;
	overridePalette = 0;
	unusedField3 = 0;
}

void ScreenPalette::forceUpdate(int16_t ticks)
{
	if (ticks > 0) {
		cycleOnly = 1;
		update();
		cycleOnly = 0;
	}
}

void ScreenPalette::update()
{
	uint32_t time = GameTime.getElapsed();
	uint16_t hour = GameTime.getHour();
	uint16_t minute = GameTime.getMinute();

	currentWeather = CurrentWeather.type;
	if (enabled == 0)
		return;
	pal->cycle();
	if (cycleOnly)
		return;
	if (suspended)
		return;
	if (fading) {
		switch (fadeType) {
		case FADE_OUT:
			Creeper_runEffect(this, EFFECT_FADE_OUT, fadeSteps, 12 - fadeSteps);
			break;
		case FADE_LIGHTNING:
			playThunder = 1;
			Creeper_runEffect(this, EFFECT_LIGHTNING, fadeSteps, 1 - fadeSteps);
			break;
		case FADE_DAMAGE:
			Creeper_runEffect(this, EFFECT_DAMAGE, fadeSteps, 6 - fadeSteps);
			break;
		case FADE_QUICK_IN:
			Creeper_runEffect(this, EFFECT_UNFADE, fadeSteps, 8 - fadeSteps);
			break;
		case FADE_IN:
			Creeper_runEffect(this, EFFECT_FADE_IN, fadeSteps, 12 - fadeSteps);
			break;
		}
		if (--fadeSteps == 0) {
			fading = 0;
			switch (fadeType) {
			case FADE_IN:
				updateTime();
				pal->modified = 1;
				stepRate = STEP_EVERY_TICK;
				break;
			case FADE_OUT:
				if (fadedOut > 0)
					fadedOut = 1;
				break;
			case FADE_QUICK_IN:
				updateTime();
				pal->modified = 1;
				stepRate = STEP_EVERY_TICK;
				break;
			case FADE_DAMAGE:
				updateTime();
				pal->modified = 1;
				stepRate = STEP_EVERY_TICK;
				break;
			}
			fadeType = 0;
		}
	} else {
		/* an invisible avatar gets its own palette */
		if ((uint8_t)(Item_getQualityFlags(&AvatarRef) & QUALITY_INVISIBLE)) {
			showInvisiblePalette();
		} else if (state == PAL_INVISIBLE) {
			state = PAL_RESET;
			overridePalette = 0;
			restore();
		}
		if (!overridePalette) {
			if (lightSpellTime != 0)
				updateLight();
			else if (state == PAL_LIGHT_SPELL)
				endLight();
			if (lightLevel != 0)
				updateLightLevel();
			else if (state == PAL_LIGHT_SOURCE)
				endLightLevel();
			if ((currentDungeon = InDungeon) || previousDungeon) {
				updateDungeon();
			} else {
				updateWeather();
				if (hour > 20 || hour < 5)
					showNight();
				else if (hour == 5)
					showDusk();
				else if (hour > 5 && hour < 20)
					showDay();
				else if (hour == 20)
					showDusk();
			}
		}
	}
	if (transitioning() && !fading) {
		switch (stepRate) {
		case STEP_EVERY_TICK:
			if (lastTime != time) {
				lastTime = time;
				if (Creeper_stepAndShow(this) == 0) {
					stepRate = STEP_EVERY_MINUTE;
					if (overridePalette)
						overridePalette = 0;
				}
			}
			break;
		case STEP_EVERY_2_TICKS:
			if (lastTime + 2 <= time) {
				lastTime = time;
				if (Creeper_stepAndShow(this) == 0) {
					stepRate = STEP_EVERY_MINUTE;
					if (overridePalette)
						overridePalette = 0;
				}
			}
			break;
		case STEP_EVERY_5_TICKS:
			if (lastTime + 5 <= time) {
				lastTime = time;
				if (Creeper_stepAndShow(this) == 0) {
					stepRate = STEP_EVERY_MINUTE;
					if (overridePalette)
						overridePalette = 0;
				}
			}
			break;
		case STEP_EVERY_10_TICKS:
			if (lastTime + 10 <= time) {
				lastTime = time;
				if (Creeper_stepAndShow(this) == 0) {
					stepRate = STEP_EVERY_MINUTE;
					if (overridePalette)
						overridePalette = 0;
				}
			}
			break;
		case STEP_EVERY_12_TICKS:
			if (lastTime + 12 <= time) {
				lastTime = time;
				if (Creeper_stepAndShow(this) == 0) {
					stepRate = STEP_EVERY_MINUTE;
					if (overridePalette)
						overridePalette = 0;
				}
			}
			break;
		case STEP_EVERY_MINUTE:
			if ((uint16_t)lastMinute != minute) {
				lastMinute = minute;
				if (Creeper_stepAndShow(this) == 0) {
					stepRate = STEP_EVERY_MINUTE;
					if (overridePalette)
						overridePalette = 0;
				}
			}
			break;
		}
	}
}

void ScreenPalette::updateWeather()
{
	uint8_t type;
	int8_t day;

	day = GameTime.getHour() > 4 && GameTime.getHour() < 21;
	if (day == 0)
		return;
	type = CurrentWeather.type;
	if (weather != type) {
		steps = 0;
		lightBand = 0;
		weather = type;
		/* weathers 1, 2 and 5 dim the day and 4 is fog; only 2 brings lightning */
		switch (weather) {
		case 0:
		case 3:
			if (state != PAL_DAY) {
				state = PAL_DAY;
				stepRate = STEP_EVERY_2_TICKS;
				Creeper_creepTo(this, 1, DayPalette.data, 10);
			}
			break;
		case 1:
		case 2:
		case 5:
			if (state != PAL_OVERCAST) {
				state = PAL_OVERCAST;
				stepRate = STEP_EVERY_2_TICKS;
				Creeper_creepTo(this, 1, OvercastPalette.data, 10);
			}
			break;
		case 4:
			if (state != PAL_OVERCAST) {
				state = PAL_OVERCAST;
				stepRate = STEP_EVERY_2_TICKS;
				Creeper_creepTo(this, 1, FogPalette.data, 10);
			}
			break;
		case 6:
			if (state != PAL_DAY) {
				state = PAL_DAY;
				stepRate = STEP_EVERY_2_TICKS;
				Creeper_creepTo(this, 1, DayPalette.data, 10);
			}
			break;
		default:
			weather = 0;
			if (state != PAL_DAY) {
				state = PAL_DAY;
				stepRate = STEP_EVERY_2_TICKS;
				Creeper_creepTo(this, 1, DayPalette.data, 10);
			}
			break;
		}
	} else if (weather == 2 && RollRandom(99) < 3) {
		flash();
	}
}

void ScreenPalette::startDamageFlash()
{
	if (!fading) {
		fadeType = FADE_DAMAGE;
		fading = 1;
		fadeSteps = 6;
	}
}

void ScreenPalette::showDay()
{
	if (state != PAL_DAY) {
		if (state != PAL_INVISIBLE) {
			if (state != PAL_OVERCAST) {
				lightBand = 0;
				stepRate = STEP_EVERY_MINUTE;
				state = PAL_DAY;
				Creeper_creepTo(this, 1, DayPalette.data, 60);
			}
		}
	}
}

void ScreenPalette::showDusk()
{
	if (state != PAL_OVERCAST && state != PAL_DUSK && state != PAL_INVISIBLE
		&& lightLevel == 0 && !lightSpellActive) {
		lightBand = 0;
		stepRate = STEP_EVERY_MINUTE;
		state = PAL_DUSK;
		Creeper_creepTo(this, 1, DuskPalette.data, 60);
	}
}

uint8_t ScreenPalette::isDark()
{
	if (state == PAL_NIGHT || state == PAL_OVERCAST || InDungeon == 1)
		return 1;
	return 0;
}

uint8_t ScreenPalette::checkDungeon()
{
	currentDungeon = InDungeon;
	return currentDungeon;
}

void ScreenPalette::updateDungeon()
{
	if (InDungeon != 0) {
		previousDungeon = 1;
		if (state != PAL_NIGHT && state != PAL_INVISIBLE && lightLevel == 0 && !lightSpellActive) {
			lightBand = 0;
			state = PAL_NIGHT;
			steps = 0;
			pal->setColors(NightPalette.data);
		} else if (lightSpellActive) {
			updateLight();
		} else if (lightLevel != 0) {
			setLightLevel(lightLevel);
		}
	} else {
		previousDungeon = 0;
		if (state != PAL_INVISIBLE)
			restore();
	}
}

void ScreenPalette::showNight()
{
	if (state != PAL_OVERCAST && state != PAL_NIGHT && state != PAL_INVISIBLE
		&& lightLevel == 0 && !lightSpellActive) {
		lightBand = 0;
		stepRate = STEP_EVERY_MINUTE;
		state = PAL_NIGHT;
		Creeper_creepTo(this, 1, NightPalette.data, 60);
	}
}

void ScreenPalette::restoreWeather()
{
	lightBand = 0;
	if (weather != CurrentWeather.type) {
		switch (weather) {
		case 0:
		case 3:
			state = PAL_DAY;
			stepRate = STEP_EVERY_2_TICKS;
			pal->setColors(DayPalette.data);
			break;
		case 1:
		case 2:
		case 5:
			state = PAL_OVERCAST;
			stepRate = STEP_EVERY_2_TICKS;
			pal->setColors(OvercastPalette.data);
			break;
		case 4:
			state = PAL_OVERCAST;
			stepRate = STEP_EVERY_2_TICKS;
			pal->setColors(FogPalette.data);
			break;
		case 6:
			if (state != PAL_DAY) {
				state = PAL_DAY;
				stepRate = STEP_EVERY_2_TICKS;
				Creeper_creepTo(this, 1, DayPalette.data, 10);
			}
			break;
		default:
			weather = 0;
			state = PAL_DAY;
			stepRate = STEP_EVERY_2_TICKS;
			pal->setColors(DayPalette.data);
		}
	} else {
		state = PAL_DAY;
		pal->setColors(DayPalette.data);
	}
	steps = 0;
	pal->modified = 1;
	pal->apply(paletteMode);
}

void ScreenPalette::restore()
{
	uint16_t hour = GameTime.getHour();

	if (state == PAL_HELD)
		return;
	if (!overridePalette) {
		stepRate = STEP_EVERY_MINUTE;
		if (InDungeon != 0) {
			if (lightLevel == 0 && !lightSpellActive) {
				lightBand = 0;
				if (state != PAL_NIGHT) {
					state = PAL_NIGHT;
					pal->setColors(NightPalette.data);
				}
			} else if (lightSpellActive) {
				lightBand = 0;
				updateLight();
			} else {
				setLightLevel(lightLevel);
			}
		} else {
			if (hour > 20 || hour < 5) {
				if (lightLevel == 0 && !lightSpellActive) {
					lightBand = 0;
					state = PAL_NIGHT;
					pal->setColors(NightPalette.data);
				} else if (lightSpellActive) {
					lightBand = 0;
					updateLight();
				} else {
					setLightLevel(lightLevel);
				}
			} else if (hour == 5) {
				lightBand = 0;
				state = PAL_DUSK;
				pal->setColors(DuskPalette.data);
			} else if (hour > 5 && hour < 20) {
				lightBand = 0;
				weather = CurrentWeather.type;
				switch (weather) {
				case 0:
				case 3:
					state = PAL_DAY;
					stepRate = STEP_EVERY_2_TICKS;
					pal->setColors(DayPalette.data);
					break;
				case 1:
				case 2:
				case 5:
					state = PAL_OVERCAST;
					stepRate = STEP_EVERY_2_TICKS;
					pal->setColors(OvercastPalette.data);
					break;
				case 4:
					state = PAL_OVERCAST;
					stepRate = STEP_EVERY_2_TICKS;
					pal->setColors(FogPalette.data);
					break;
				case 6:
					if (state == PAL_RESET) {
						state = PAL_DAY;
						stepRate = STEP_EVERY_2_TICKS;
						pal->setColors(DayPalette.data);
					} else if (state != PAL_DAY) {
						state = PAL_DAY;
						stepRate = STEP_EVERY_2_TICKS;
						pal->setColors(DayPalette.data);
					}
					break;
				default:
					weather = 0;
					state = PAL_DAY;
					stepRate = STEP_EVERY_2_TICKS;
					pal->setColors(DayPalette.data);
				}
			} else if (hour == 20) {
				lightBand = 0;
				state = PAL_DUSK;
				pal->setColors(DuskPalette.data);
			}
		}
	}
	steps = 0;
	pal->modified = 1;
	pal->apply(paletteMode);
}

void ScreenPalette::advanceToClock()
{
	uint16_t hour = GameTime.getHour();
	uint16_t minute = GameTime.getMinute();
	uint16_t elapsed;

	if (!overridePalette) {
		pendingUpdate = 0;
		if (hour > 20 || hour < 5) {
			if (lightLevel == 0 && !lightSpellActive) {
				lightBand = 0;
				state = PAL_NIGHT;
				showNight();
			} else if (lightSpellActive) {
				lightBand = 0;
				updateLight();
			} else {
				setLightLevel(lightLevel);
			}
		} else if (hour == 5) {
			showDusk();
		} else if (hour > 5 && hour < 20) {
			showDay();
		} else if (hour == 20) {
			showDusk();
		}
		updateWeather();
		for (elapsed = 0, --minute; elapsed < minute; ++elapsed)
			Creeper_step(this);
		if (Creeper_stepAndShow(this) == 0) {
			stepRate = STEP_EVERY_MINUTE;
			if (overridePalette)
				overridePalette = 0;
		}
		pal->modified = 1;
		pal->apply(paletteMode);
	} else {
		pendingUpdate = 1;
	}
}

void ScreenPalette::advanceTo(uint16_t hour, uint16_t minute)
{
	uint16_t elapsed;

	if (!overridePalette) {
		pendingUpdate = 0;
		if (hour > 20 || hour < 5) {
			if (lightLevel == 0 && !lightSpellActive)
				showNight();
		} else if (hour == 5) {
			showDusk();
		} else if (hour > 5 && hour < 20) {
			showDay();
		} else if (hour == 20) {
			showDusk();
		}
		updateWeather();
		for (elapsed = 0, --minute; elapsed < minute; ++elapsed)
			Creeper_step(this);
		if (Creeper_stepAndShow(this) == 0) {
			stepRate = STEP_EVERY_MINUTE;
			if (overridePalette)
				overridePalette = 0;
		}
	} else {
		pendingUpdate = 1;
	}
}

void ScreenPalette::selectCurrent()
{
	uint16_t hour = GameTime.getHour();

	if ((uint8_t)(Item_getQualityFlags(&AvatarRef) & QUALITY_INVISIBLE)) {
		showInvisiblePalette();
	} else {
		stepRate = STEP_EVERY_MINUTE;
		if (InDungeon != 0) {
			if (lightLevel == 0 && !lightSpellActive) {
				lightBand = 0;
				if (state != PAL_NIGHT) {
					state = PAL_NIGHT;
					pal->setColors(NightPalette.data);
				}
			} else if (lightSpellActive) {
				lightBand = 0;
				updateLight();
			} else {
				setLightLevel(lightLevel);
			}
		} else {
			if (hour > 20 || hour < 5) {
				if (lightLevel == 0 && !lightSpellActive) {
					lightBand = 0;
					state = PAL_NIGHT;
					pal->setColors(NightPalette.data);
				} else if (lightSpellActive) {
					lightBand = 0;
					updateLight();
				} else {
					setLightLevel(lightLevel);
				}
			} else if (hour == 5) {
				lightBand = 0;
				state = PAL_DUSK;
				pal->setColors(DuskPalette.data);
			} else if (hour > 5 && hour < 20) {
				weather = CurrentWeather.type;
				lightBand = 0;
				switch (weather) {
				case 0:
				case 3:
					state = PAL_DAY;
					stepRate = STEP_EVERY_2_TICKS;
					pal->setColors(DayPalette.data);
					break;
				case 1:
				case 2:
				case 5:
					state = PAL_OVERCAST;
					stepRate = STEP_EVERY_2_TICKS;
					pal->setColors(OvercastPalette.data);
					break;
				case 4:
					state = PAL_OVERCAST;
					stepRate = STEP_EVERY_2_TICKS;
					pal->setColors(FogPalette.data);
					break;
				case 6:
					if (state != PAL_DAY) {
						state = PAL_DAY;
						stepRate = STEP_EVERY_2_TICKS;
						Creeper_creepTo(this, 1, DayPalette.data, 10);
					}
					break;
				default:
					weather = 0;
					state = PAL_DAY;
					stepRate = STEP_EVERY_2_TICKS;
					pal->setColors(DayPalette.data);
				}
			} else if (hour == 20) {
				lightBand = 0;
				state = PAL_DUSK;
				pal->setColors(DuskPalette.data);
			}
		}
	}
	steps = 0;
}

void ScreenPalette::updateTime()
{
	uint16_t hour = GameTime.getHour();

	if (!overridePalette) {
		if ((hour > 20) | (hour < 5)) {
			if (lightLevel == 0 && !lightSpellActive) {
				lightBand = 0;
				showNight();
			} else if (lightSpellActive != 0) {
				lightBand = 0;
				updateLight();
			} else {
				setLightLevel(lightLevel);
			}
		} else if (hour == 5) {
			lightBand = 0;
			showDusk();
		} else if (hour > 5 && hour < 20) {
			lightBand = 0;
			showDay();
		} else if (hour == 20) {
			lightBand = 0;
			showDusk();
		}
		stepRate = STEP_EVERY_MINUTE;
		updateWeather();
	}
}

void ScreenPalette::endLight()
{
	if (state == PAL_LIGHT_SPELL)
		state = 0;
	lightSpellActive = 0;
	restore();
}

void ScreenPalette::updateLight()
{
	lightSpellActive = 1;
	if (state != PAL_LIGHT_SPELL) {
		if (state != PAL_INVISIBLE) {
			if (IsNight() || InDungeon == 1) {
				lightBand = 0;
				state = PAL_LIGHT_SPELL;
				stepRate = STEP_EVERY_2_TICKS;
				steps = 0;
				pal->setColors(LightSpellPalette.data);
			}
		}
	} else if (--lightSpellTime == 0) {
		clearLight();
	}
}

void ScreenPalette::clearLight()
{
	lightSpellTime = 0;
}

void ScreenPalette::updateLightLevel()
{
	if (lightLevel != 0) {
		setLightLevel(lightLevel);
	}
}

void ScreenPalette::endLightLevel()
{
	if (state == PAL_LIGHT_SOURCE)
		state = 0;
	lightBand = 0;
	restore();
}

void ScreenPalette::setLightLevel(int16_t level)
{
	if (state == PAL_LIGHT_SPELL)
		return;
	if (state == PAL_INVISIBLE)
		return;
	if (IsNight() || InDungeon == 1) {
		steps = 0;
		stepRate = STEP_EVERY_2_TICKS;
		state = PAL_LIGHT_SOURCE;
		if (level < 7) {
			if (lightBand == 1)
				return;
			lightBand = 1;
			pal->setColors(CandlePalette.data);
		} else if (level < 20) {
			if (lightBand == 2)
				return;
			lightBand = 2;
			pal->setColors(SingleLightPalette.data);
		} else {
			if (lightBand == 3)
				return;
			lightBand = 3;
			pal->setColors(ManyLightsPalette.data);
		}
	}
}

void ScreenPalette::showInvisiblePalette()
{
	if (state != PAL_INVISIBLE) {
		lightBand = 0;
		overridePalette = 1;
		state = PAL_INVISIBLE;
		stepRate = STEP_EVERY_2_TICKS;
		pal->setColors(InvisiblePalette.data);
		steps = 0;
		pal->modified = 1;
		pal->apply(paletteMode);
	}
}

void ScreenPalette::fadeOut()
{
	state = PAL_FADED_OUT;
	fading = 1;
	fadeType = FADE_OUT;
	fadeSteps = 12;
}

void ScreenPalette::startQuickFadeIn()
{
	if (!fading) {
		fading = 1;
		fadeType = FADE_QUICK_IN;
		fadeSteps = 8;
	}
}

void ScreenPalette::startFadeInOver(int16_t count)
{
	if (!fading) {
		fading = 1;
		fadeType = FADE_QUICK_IN;
		fadeSteps = count;
	}
}

void ScreenPalette::flash()
{
	if (!fading) {
		fading = 1;
		fadeType = FADE_LIGHTNING;
		fadeSteps = RollRandom(3) + 1;
	}
}

void ScreenPalette::fadeIn()
{
	fading = 1;
	fadeType = FADE_IN;
	fadeSteps = 12;
}

void ScreenPalette::suspend()
{
	suspended = 1;
	lightBand = 0;
	overridePalette = 0;
	if (fading) {
		fading = 0;
		fadeType = 0;
		fadeSteps = 0;
	}
	weather = 0;
	restoreWeather();
}

void ScreenPalette::resume()
{
	suspended = 0;
	restore();
}

void ScreenPalette::hold()
{
	lightBand = 0;
	state = PAL_HELD;
}

void ScreenPalette::finishFadeOut()
{
	lightBand = 0;
	fadeOut();
	while (fadeSteps != 0) {
		Creeper_runEffect(this, EFFECT_FADE_OUT, fadeSteps, 12 - fadeSteps);
		if (--fadeSteps == 0) {
			fading = 0;
			fadeType = 0;
		}
	}
	overridePalette = 0;
}

void ScreenPalette::finishFade(int8_t daylightOnly)
{
	lightBand = 0;
	if (fading) {
		while (fading) {
			Creeper_runEffect(this, EFFECT_FADE_OUT, fadeSteps, 12 - fadeSteps);
			if (--fadeSteps == 0) {
				fading = 0;
				fadeType = 0;
			}
		}
	}
	if (daylightOnly) {
		overridePalette = 0;
		state = PAL_DAY;
		stepRate = STEP_EVERY_2_TICKS;
		pal->setColors(DayPalette.data);
	} else {
		restore();
	}
}

void ScreenPalette::reset()
{
	lightBand = 0;
	restore();
}

