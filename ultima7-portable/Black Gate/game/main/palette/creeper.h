#ifndef CREEPER_H
#define CREEPER_H

#include "rgbstep.h"

struct Palette;

/* the effects Creeper_runEffect plays */
#define EFFECT_DAMAGE    1  /* flash red, then fade back */
#define EFFECT_UNFADE    2  /* go black, then fade back quickly */
#define EFFECT_LIGHTNING 4
#define EFFECT_FADE_OUT  8
#define EFFECT_FADE_IN   16

/* steps the screen palette toward another set of colors, and plays flashes and fades */
struct Creeper {
	Palette *pal;
	int16_t mode;
	int32_t buffer, from, to;
	int16_t steps;
	int32_t saved;
	uint8_t active, paletteMode, playThunder;
	RgbColor original[256], current[256];
	Creeper();
	uint8_t transitioning() { return steps != 0 ? 1 : 0; }
};

/* a palette read from PALETTES.FLX into voodoo memory */
struct PaletteResource {
	int32_t data;
	PaletteResource() { data = 0; }
};

extern PaletteResource RedPalette, DarkPalette, LightningPalette, CandlePalette;
extern PaletteResource SingleLightPalette, ManyLightsPalette, LightSpellPalette, OvercastPalette;
extern PaletteResource FogPalette, DayPalette, NightPalette, DuskPalette;
extern PaletteResource InvisiblePalette, SparePalette, RedRampPalette;
extern Palette LivePalette;
extern int16_t EffectCountdown;

void Creeper_loadPalettes(Creeper *creeper);
void Creeper_clearSource(Creeper *creeper);
void Creeper_clearTarget(Creeper *creeper);
void Creeper_clearBuffer(Creeper *creeper);
void Creeper_allocateSaved(Creeper *creeper);
void Creeper_allocateSource(Creeper *creeper);
void Creeper_allocateTarget(Creeper *creeper);
void Creeper_allocateBuffer(Creeper *creeper);
void Creeper_creepBetween(Creeper *creeper, int16_t mode, int32_t from, int32_t to, int32_t target, int16_t speed);
void Creeper_creepBetweenFar(Creeper *creeper, int16_t mode, void *from, void *to, int32_t target,
	int16_t speed);
void Creeper_creepTo(Creeper *creeper, int16_t mode, int32_t target, int16_t speed);
void Creeper_prepareSteps(Creeper *creeper, int32_t target, int16_t speed);
void Creeper_step(Creeper *creeper);
uint8_t Creeper_stepAndShow(Creeper *creeper);
void Creeper_runEffect(Creeper *creeper, int16_t effect, int16_t duration, int16_t continuing);
void Creeper_savePalette(Creeper *creeper);

#endif
