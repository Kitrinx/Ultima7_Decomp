/* Serpent Isle SI.EXE, resident segment 90 (file offsets 0x033cf6 to 0x0349a5, 3247 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
#include "lowlevel.h"
#include "dosio.h"
#include "vooalloc.h"
#include "crawpal.h"
#include "oops.h"
#include "crgbpal.h"
#include "rgbstep.h"
#include "vidmode.h"
#include "debug.h"
#include "creeper.h"
#include "memapi.h"

extern "C" void PlaySfx(uint8_t number, uint16_t volume, int16_t pan);

PaletteResource RedPalette;
PaletteResource DarkPalette;
PaletteResource LightningPalette;
PaletteResource CandlePalette;
PaletteResource SingleLightPalette;
PaletteResource ManyLightsPalette;
PaletteResource LightSpellPalette;
PaletteResource OvercastPalette;
PaletteResource FogPalette;
PaletteResource DayPalette;
PaletteResource NightPalette;
PaletteResource DuskPalette;
PaletteResource InvisiblePalette;
PaletteResource SparePalette;
PaletteResource RedRampPalette;
Palette LivePalette;
int16_t EffectCountdown = 0;

Creeper::Creeper()
{
	playThunder = 0;
	active = 0;
	paletteMode = 0;
	mode = 0;
	buffer = 0;
	from = 0;
	to = 0;
	steps = 0;
}

void Creeper_loadPalettes(Creeper *creeper)
{
	LoadPalette(&CandlePalette.data, 7);
	LoadPalette(&SingleLightPalette.data, 11);
	LoadPalette(&ManyLightsPalette.data, 12);
	LoadPalette(&RedPalette.data, 8);
	LoadPalette(&DarkPalette.data, 9);
	LoadPalette(&LightningPalette.data, 10);
	LoadPalette(&LightSpellPalette.data, 6);
	LoadPalette(&OvercastPalette.data, 4);
	LoadPalette(&FogPalette.data, 5);
	LoadPalette(&DayPalette.data, 0);
	LoadPalette(&DuskPalette.data, 1);
	LoadPalette(&NightPalette.data, 2);
	LoadPalette(&InvisiblePalette.data, 3);
	LoadPalette(&RedRampPalette.data, 9);
	creeper->steps = 0;
	if (DebugOutputEnabled == 0)
		LivePalette.setColors(RedRampPalette.data);
	else
		LivePalette.setColors(DayPalette.data);
	creeper->pal = &LivePalette;
	creeper->pal->modified = 1;
	creeper->pal->apply(creeper->paletteMode);
}

void Creeper_clearSource(Creeper *creeper)
{
	FillLinear(creeper->from, 0, INT32_C(768), 0x111);
}

void Creeper_clearTarget(Creeper *creeper)
{
	FillLinear(creeper->to, 0, PALETTE_SIZE, 0x111);
}

void Creeper_clearBuffer(Creeper *creeper)
{
	FillLinear(creeper->buffer, 0, PALETTE_SIZE, 0x111);
}

void Creeper_allocateSaved(Creeper *creeper)
{
	if ((creeper->saved = AllocateVoodooMemory(&VoodooXmsBlock, PALETTE_SIZE)) == 0)
		ReportOutOfVoodooMemory();
}

void Creeper_allocateSource(Creeper *creeper)
{
	if ((creeper->from = AllocateVoodooMemory(&VoodooXmsBlock, PALETTE_SIZE)) == 0)
		ReportOutOfVoodooMemory();
	FillLinear(creeper->from, 0, PALETTE_SIZE, 0x111);
}

void Creeper_allocateTarget(Creeper *creeper)
{
	if ((creeper->to = AllocateVoodooMemory(&VoodooXmsBlock, PALETTE_SIZE)) == 0)
		ReportOutOfVoodooMemory();
	FillLinear(creeper->to, 0, PALETTE_SIZE, 0x111);
}

void Creeper_allocateBuffer(Creeper *creeper)
{
	if ((creeper->buffer = AllocateVoodooMemory(&VoodooXmsBlock, PALETTE_SIZE)) == 0)
		ReportOutOfVoodooMemory();
	FillLinear(creeper->buffer, 0, PALETTE_SIZE, 0x111);
}

void Creeper_creepBetween(Creeper *creeper, int16_t mode, int32_t from, int32_t to, int32_t target, int16_t speed)
{
	creeper->mode = mode;
	creeper->from = from;
	creeper->to = to;
	if (creeper->buffer == 0)
		Creeper_allocateBuffer(creeper);
	else
		FillLinear(creeper->buffer, 0, PALETTE_SIZE, 0x111);
	MoveLinear(creeper->buffer, creeper->pal->colors, PALETTE_SIZE, 0x111);
	Creeper_prepareSteps(creeper, target, speed);
}

void Creeper_creepBetweenFar(Creeper *creeper, int16_t mode, void *from, void *to, int32_t target, int16_t speed)
{
	creeper->mode = mode;
	creeper->from = PointerToLinear(from);
	creeper->to = PointerToLinear(to);
	if (creeper->buffer == 0)
		Creeper_allocateBuffer(creeper);
	else
		FillLinear(creeper->buffer, 0, PALETTE_SIZE, 0x111);
	MoveLinear(creeper->buffer, creeper->pal->colors, PALETTE_SIZE, 0x111);
	Creeper_prepareSteps(creeper, target, speed);
}

void Creeper_creepTo(Creeper *creeper, int16_t mode, int32_t target, int16_t speed)
{
	creeper->mode = mode;
	if (creeper->from == 0)
		Creeper_allocateSource(creeper);
	else
		FillLinear(creeper->from, 0, PALETTE_SIZE, 0x111);
	if (creeper->to == 0)
		Creeper_allocateTarget(creeper);
	else
		FillLinear(creeper->to, 0, PALETTE_SIZE, 0x111);
	if (creeper->buffer == 0)
		Creeper_allocateBuffer(creeper);
	else
		FillLinear(creeper->buffer, 0, PALETTE_SIZE, 0x111);
	MoveLinear(creeper->buffer, creeper->pal->colors, PALETTE_SIZE, 0x111);
	Creeper_prepareSteps(creeper, target, speed);
}

void Creeper_prepareSteps(Creeper *creeper, int32_t target, int16_t speed)
{
	creeper->steps = PreparePaletteFade(creeper->pal->colors, target, creeper->from, creeper->to, speed);
}

void Creeper_step(Creeper *creeper)
{
	StepPaletteFade(creeper->pal->colors, creeper->from, creeper->to);
	creeper->pal->modified = 1;
}

uint8_t Creeper_stepAndShow(Creeper *creeper)
{
	Creeper_step(creeper);
	creeper->pal->apply(creeper->paletteMode);
	if (--creeper->steps)
		return 1;
	return 0;
}

/* The effect colors live in the creeper, outside linear memory, so they are converted
 * through a far heap buffer. */
static void *scratch = 0;

static void WordsToColors(RgbColor *dest, int32_t src)
{
	if (scratch == 0 && (scratch = AllocateFarHeap(256 * sizeof(RgbColor), 0)) == 0)
		ReportOutOfFarMemory();
	PaletteWordsToBytes(PointerToLinear(scratch), src);
	memcpy(dest, scratch, 256 * sizeof(RgbColor));
}

/* play one step of an effect; continuing is 0 on its first step */
void Creeper_runEffect(Creeper *creeper, int16_t effect, int16_t duration, int16_t continuing)
{
	int16_t unused = 6;
	RgbColor red(63, 0, 0);
	int16_t i;

	if (continuing == 0) {
		creeper->active = 0;
		EffectCountdown = duration - 2;
	}
	switch (effect) {
	case EFFECT_DAMAGE:
		if (creeper->active) {
			if (--EffectCountdown > 0) {
				for (i = 0; i < 256; ++i)
					StepToColor(&creeper->current[creeper->pal->order[i]],
						creeper->original[creeper->pal->order[i]], 12);
				WaitForRetrace();
				SetPaletteRangeBytes((uint8_t *) creeper->current, creeper->pal->order, 0, 256);
			} else {
				creeper->active = 0;
				WaitForRetrace();
				SetPaletteRangeBytes((uint8_t *) creeper->original, creeper->pal->order, 0, 256);
			}
		} else {
			WordsToColors(creeper->original, creeper->pal->colors);
			EffectCountdown = 8;
			for (i = 0; i < 256; ++i)
				creeper->current[creeper->pal->order[i]].set(red);
			WaitForRetrace();
			SetPaletteRangeBytes((uint8_t *) creeper->current, creeper->pal->order, 0, 256);
			creeper->active = 1;
		}
		break;
	case EFFECT_UNFADE:
		if (creeper->active) {
			if (--EffectCountdown > 0) {
				for (i = 0; i < 256; ++i)
					StepToColor(&creeper->current[creeper->pal->order[i]],
						creeper->original[creeper->pal->order[i]], 9);
				WaitForRetrace();
				SetPaletteRangeBytes((uint8_t *) creeper->current, creeper->pal->order, 0, 256);
			} else {
				creeper->active = 0;
				WaitForRetrace();
				SetPaletteRangeBytes((uint8_t *) creeper->original, creeper->pal->order, 0, 256);
			}
		} else {
			WordsToColors(creeper->original, creeper->pal->colors);
			EffectCountdown = 8;
			for (i = 0; i < 256; ++i)
				SetRgb(&creeper->current[creeper->pal->order[i]], 0, 0, 0);
			WaitForRetrace();
			SetPaletteRangeBytes((uint8_t *) creeper->current, creeper->pal->order, 0, 256);
			creeper->active = 1;
		}
		break;
	case EFFECT_FADE_OUT:
		if (creeper->active) {
			if (--EffectCountdown > 0) {
				for (i = 0; i < 256; ++i)
					StepToColor(&creeper->current[creeper->pal->order[i]],
						creeper->original[creeper->pal->order[i]], 9);
				WaitForRetrace();
				SetPaletteRangeBytes((uint8_t *) creeper->current, creeper->pal->order, 0, 256);
			} else {
				creeper->active = 0;
				WaitForRetrace();
				SetPaletteRangeBytes((uint8_t *) creeper->original, creeper->pal->order, 0, 256);
			}
		} else {
			WordsToColors(creeper->current, creeper->pal->colors);
			for (i = 0; i < 256; ++i)
				SetRgb(&creeper->original[creeper->pal->order[i]], 0, 0, 0);
			creeper->active = 1;
		}
		break;
	case EFFECT_FADE_IN:
		if (creeper->active) {
			if (--EffectCountdown > 0) {
				for (i = 0; i < 256; ++i)
					StepToColor(&creeper->current[creeper->pal->order[i]],
						creeper->original[creeper->pal->order[i]], 9);
				WaitForRetrace();
				SetPaletteRangeBytes((uint8_t *) creeper->current, creeper->pal->order, 0, 256);
			} else {
				creeper->active = 0;
				WaitForRetrace();
				SetPaletteRangeBytes((uint8_t *) creeper->original, creeper->pal->order, 0, 256);
			}
		} else {
			WordsToColors(creeper->original, creeper->pal->colors);
			for (i = 0; i < 256; ++i)
				SetRgb(&creeper->current[creeper->pal->order[i]], 0, 0, 0);
			creeper->active = 1;
		}
		break;
	case EFFECT_LIGHTNING:
		if (creeper->playThunder) {
			PlaySfx(116, 255, 64);  /* thunder */
			creeper->playThunder = 0;
		}
		Creeper_savePalette(creeper);
		creeper->pal->setColors(LightningPalette.data);
		WaitForRetrace();
		creeper->pal->apply(creeper->paletteMode);
		MoveLinear(creeper->pal->colors, creeper->saved, PALETTE_SIZE, 0x111);
		creeper->pal->modified = 1;
		WaitForRetrace();
		creeper->pal->apply(creeper->paletteMode);
		/* On VGA the restored colours showed at once; end the flash now. */
		WaitForRetrace();
		break;
	}
}

void Creeper_savePalette(Creeper *creeper)
{
	if (creeper->saved == 0)
		Creeper_allocateSaved(creeper);
	MoveLinear(creeper->saved, creeper->pal->colors, PALETTE_SIZE, 0x111);
	creeper->pal->modified = 1;
}

extern "C" void ResetCreeperGlobals(void)
{
	memset((void *)&RedPalette, 0, sizeof(RedPalette));
	memset((void *)&DarkPalette, 0, sizeof(DarkPalette));
	memset((void *)&LightningPalette, 0, sizeof(LightningPalette));
	memset((void *)&CandlePalette, 0, sizeof(CandlePalette));
	memset((void *)&SingleLightPalette, 0, sizeof(SingleLightPalette));
	memset((void *)&ManyLightsPalette, 0, sizeof(ManyLightsPalette));
	memset((void *)&LightSpellPalette, 0, sizeof(LightSpellPalette));
	memset((void *)&OvercastPalette, 0, sizeof(OvercastPalette));
	memset((void *)&FogPalette, 0, sizeof(FogPalette));
	memset((void *)&DayPalette, 0, sizeof(DayPalette));
	memset((void *)&NightPalette, 0, sizeof(NightPalette));
	memset((void *)&DuskPalette, 0, sizeof(DuskPalette));
	memset((void *)&InvisiblePalette, 0, sizeof(InvisiblePalette));
	memset((void *)&SparePalette, 0, sizeof(SparePalette));
	memset((void *)&RedRampPalette, 0, sizeof(RedRampPalette));
	memset((void *)&LivePalette, 0, sizeof(LivePalette));
	EffectCountdown = 0;
	scratch = 0;
}

extern "C" void ConstructCreeperGlobals(void)
{
	new (&RedPalette) PaletteResource();
	new (&DarkPalette) PaletteResource();
	new (&LightningPalette) PaletteResource();
	new (&CandlePalette) PaletteResource();
	new (&SingleLightPalette) PaletteResource();
	new (&ManyLightsPalette) PaletteResource();
	new (&LightSpellPalette) PaletteResource();
	new (&OvercastPalette) PaletteResource();
	new (&FogPalette) PaletteResource();
	new (&DayPalette) PaletteResource();
	new (&NightPalette) PaletteResource();
	new (&DuskPalette) PaletteResource();
	new (&InvisiblePalette) PaletteResource();
	new (&SparePalette) PaletteResource();
	new (&RedRampPalette) PaletteResource();
	new (&LivePalette) Palette();
}
