/* Black Gate U7.EXE, resident segment 106 (file offsets 0x036cbc to 0x03796e, 3250 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

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

extern "C" void far PlaySfx(unsigned char number, int volume, int pan, int flags);

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
int EffectCountdown = 0;

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

void far Creeper_loadPalettes(Creeper *creeper)
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
	LoadPalette(&RedRampPalette.data, RED_RAMP_PALETTE);
	creeper->steps = 0;
	if (DebugOutputEnabled == 0)
		LivePalette.setColors(RedRampPalette.data);
	else
		LivePalette.setColors(DayPalette.data);
	creeper->pal = &LivePalette;
	creeper->pal->modified = 1;
	creeper->pal->apply(creeper->paletteMode);
}

void far Creeper_clearSource(Creeper *creeper)
{
	FillLinear(creeper->from, 0, 768L, 0x111);
}

void far Creeper_clearTarget(Creeper *creeper)
{
	FillLinear(creeper->to, 0, PALETTE_SIZE, 0x111);
}

void far Creeper_clearBuffer(Creeper *creeper)
{
	FillLinear(creeper->buffer, 0, PALETTE_SIZE, 0x111);
}

void far Creeper_allocateSaved(Creeper *creeper)
{
	if ((creeper->saved = AllocateVoodooMemory(&VoodooXmsBlock, PALETTE_SIZE)) == 0)
		ReportOutOfVoodooMemory();
}

void far Creeper_allocateSource(Creeper *creeper)
{
	if ((creeper->from = AllocateVoodooMemory(&VoodooXmsBlock, PALETTE_SIZE)) == 0)
		ReportOutOfVoodooMemory();
	FillLinear(creeper->from, 0, PALETTE_SIZE, 0x111);
}

void far Creeper_allocateTarget(Creeper *creeper)
{
	if ((creeper->to = AllocateVoodooMemory(&VoodooXmsBlock, PALETTE_SIZE)) == 0)
		ReportOutOfVoodooMemory();
	FillLinear(creeper->to, 0, PALETTE_SIZE, 0x111);
}

void far Creeper_allocateBuffer(Creeper *creeper)
{
	if ((creeper->buffer = AllocateVoodooMemory(&VoodooXmsBlock, PALETTE_SIZE)) == 0)
		ReportOutOfVoodooMemory();
	FillLinear(creeper->buffer, 0, PALETTE_SIZE, 0x111);
}

void far Creeper_creepBetween(Creeper *creeper, int mode, long from, long to, long target, int speed)
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

void far Creeper_creepBetweenFar(Creeper *creeper, int mode, void far *from, void far *to, long target, int speed)
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

void far Creeper_creepTo(Creeper *creeper, int mode, long target, int speed)
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

void far Creeper_prepareSteps(Creeper *creeper, long target, int speed)
{
	creeper->steps = PreparePaletteFade(creeper->pal->colors, target, creeper->from, creeper->to, speed);
}

void far Creeper_step(Creeper *creeper)
{
	StepPaletteFade(creeper->pal->colors, creeper->from, creeper->to);
	creeper->pal->modified = 1;
}

unsigned char far Creeper_stepAndShow(Creeper *creeper)
{
	Creeper_step(creeper);
	creeper->pal->apply(creeper->paletteMode);
	if (--creeper->steps)
		return 1;
	return 0;
}

/* play one step of an effect; continuing is 0 on its first step */
void far Creeper_runEffect(Creeper *creeper, int effect, int duration, int continuing)
{
	int unused = 6;
	RgbColor red(63, 0, 0);
	int i;

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
				SetPaletteRangeBytes(PointerToLinear(creeper->current), creeper->pal->order, 0, 256);
			} else {
				creeper->active = 0;
				WaitForRetrace();
				SetPaletteRangeBytes(PointerToLinear(creeper->original), creeper->pal->order, 0, 256);
			}
		} else {
			PaletteWordsToBytes(PointerToLinear(creeper->original), creeper->pal->colors);
			EffectCountdown = 8;
			for (i = 0; i < 256; ++i)
				creeper->current[creeper->pal->order[i]].set(red);
			WaitForRetrace();
			SetPaletteRangeBytes(PointerToLinear(creeper->current), creeper->pal->order, 0, 256);
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
				SetPaletteRangeBytes(PointerToLinear(creeper->current), creeper->pal->order, 0, 256);
			} else {
				creeper->active = 0;
				WaitForRetrace();
				SetPaletteRangeBytes(PointerToLinear(creeper->original), creeper->pal->order, 0, 256);
			}
		} else {
			PaletteWordsToBytes(PointerToLinear(creeper->original), creeper->pal->colors);
			EffectCountdown = 8;
			for (i = 0; i < 256; ++i)
				SetRgb(&creeper->current[creeper->pal->order[i]], 0, 0, 0);
			WaitForRetrace();
			SetPaletteRangeBytes(PointerToLinear(creeper->current), creeper->pal->order, 0, 256);
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
				SetPaletteRangeBytes(PointerToLinear(creeper->current), creeper->pal->order, 0, 256);
			} else {
				creeper->active = 0;
				WaitForRetrace();
				SetPaletteRangeBytes(PointerToLinear(creeper->original), creeper->pal->order, 0, 256);
			}
		} else {
			PaletteWordsToBytes(PointerToLinear(creeper->current), creeper->pal->colors);
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
				SetPaletteRangeBytes(PointerToLinear(creeper->current), creeper->pal->order, 0, 256);
			} else {
				creeper->active = 0;
				WaitForRetrace();
				SetPaletteRangeBytes(PointerToLinear(creeper->original), creeper->pal->order, 0, 256);
			}
		} else {
			PaletteWordsToBytes(PointerToLinear(creeper->original), creeper->pal->colors);
			for (i = 0; i < 256; ++i)
				SetRgb(&creeper->current[creeper->pal->order[i]], 0, 0, 0);
			creeper->active = 1;
		}
		break;
	case EFFECT_LIGHTNING:
		if (creeper->playThunder) {
			PlaySfx(62, 255, 64, 0);  /* thunder */
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
		break;
	}
}

void far Creeper_savePalette(Creeper *creeper)
{
	if (creeper->saved == 0)
		Creeper_allocateSaved(creeper);
	MoveLinear(creeper->saved, creeper->pal->colors, PALETTE_SIZE, 0x111);
	creeper->pal->modified = 1;
}
