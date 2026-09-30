#ifndef RGBSTEP_H
#define RGBSTEP_H

/* one palette color, 6 bits per channel as the VGA DAC takes it */
struct RgbColor {
	unsigned char red, green, blue;
	RgbColor() {}
	RgbColor(unsigned char r, unsigned char g, unsigned char b)
		: red(r), green(g), blue(b) {}
	void set(RgbColor color) { *this = color; }
};

void StepToColor(RgbColor *color, RgbColor target, int percent);
void StepColorToward(RgbColor *color, int red, int green, int blue, int percent);
void SetRgb(RgbColor *color, unsigned char red, unsigned char green, unsigned char blue);

#endif
