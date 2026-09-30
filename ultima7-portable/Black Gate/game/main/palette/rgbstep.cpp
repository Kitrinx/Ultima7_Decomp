/* Black Gate U7.EXE, resident segment 105 (file offsets 0x036bc7 to 0x036cbc, 245 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "rgbstep.h"

inline int16_t Sign(int16_t d)
{
	if (d < 0)
		return -1;
	if (d == 0)
		return 0;
	return 1;
}

void StepToColor(RgbColor *color, RgbColor target, int16_t percent)
{
	StepColorToward(color, target.red, target.green, target.blue, percent);
}

/* move each channel percent of the way to the target, and at least one step */
void StepColorToward(RgbColor *color, int16_t red, int16_t green, int16_t blue, int16_t percent)
{
	int16_t dr, dg, db;

	dr = red - color->red;
	dg = green - color->green;
	db = blue - color->blue;
	color->red += dr * percent / 100 + Sign(dr);
	color->green += dg * percent / 100 + Sign(dg);
	color->blue += db * percent / 100 + Sign(db);
}

void SetRgb(RgbColor *color, uint8_t red, uint8_t green, uint8_t blue)
{
	color->red = red;
	color->green = green;
	color->blue = blue;
}
