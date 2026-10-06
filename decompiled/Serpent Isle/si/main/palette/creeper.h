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
	int mode;
	long buffer, from, to;
	int steps;
	long saved;
	unsigned char active, paletteMode, playThunder;
	RgbColor original[256], current[256];
	Creeper();
	unsigned char transitioning() { return steps != 0 ? 1 : 0; }
};

/* a palette read from PALETTES.FLX into voodoo memory */
struct PaletteResource {
	long data;
	PaletteResource() { data = 0; }
};

extern PaletteResource RedPalette, DarkPalette, LightningPalette, CandlePalette;
extern PaletteResource SingleLightPalette, ManyLightsPalette, LightSpellPalette, OvercastPalette;
extern PaletteResource FogPalette, DayPalette, NightPalette, DuskPalette;
extern PaletteResource InvisiblePalette, SparePalette, RedRampPalette;
extern Palette LivePalette;
extern int EffectCountdown;

void far Creeper_loadPalettes(Creeper *creeper);
void far Creeper_clearSource(Creeper *creeper);
void far Creeper_clearTarget(Creeper *creeper);
void far Creeper_clearBuffer(Creeper *creeper);
void far Creeper_allocateSaved(Creeper *creeper);
void far Creeper_allocateSource(Creeper *creeper);
void far Creeper_allocateTarget(Creeper *creeper);
void far Creeper_allocateBuffer(Creeper *creeper);
void far Creeper_creepBetween(Creeper *creeper, int mode, long from, long to, long target, int speed);
void far Creeper_creepBetweenFar(Creeper *creeper, int mode, void far *from, void far *to, long target,
	int speed);
void far Creeper_creepTo(Creeper *creeper, int mode, long target, int speed);
void far Creeper_prepareSteps(Creeper *creeper, long target, int speed);
void far Creeper_step(Creeper *creeper);
unsigned char far Creeper_stepAndShow(Creeper *creeper);
void far Creeper_runEffect(Creeper *creeper, int effect, int duration, int continuing);
void far Creeper_savePalette(Creeper *creeper);

#endif
