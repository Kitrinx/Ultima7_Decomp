/* Black Gate U7.EXE, resident segment 105 (file offsets 0x036bc7 to 0x036cbc, 245 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "rgbstep.h"

inline int Sign(int d)
{
	if (d < 0)
		return -1;
	if (d == 0)
		return 0;
	return 1;
}

void StepToColor(RgbColor *color, RgbColor target, int percent)
{
	StepColorToward(color, target.red, target.green, target.blue, percent);
}

/* move each channel percent of the way to the target, and at least one step */
void StepColorToward(RgbColor *color, int red, int green, int blue, int percent)
{
	int dr, dg, db;

	dr = red - color->red;
	dg = green - color->green;
	db = blue - color->blue;
	color->red += dr * percent / 100 + Sign(dr);
	color->green += dg * percent / 100 + Sign(dg);
	color->blue += db * percent / 100 + Sign(db);
}

void SetRgb(RgbColor *color, unsigned char red, unsigned char green, unsigned char blue)
{
	color->red = red;
	color->green = green;
	color->blue = blue;
}
