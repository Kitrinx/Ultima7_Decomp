#ifndef RGBSTEP_H
#define RGBSTEP_H

/* one palette color, 6 bits per channel as the VGA DAC takes it */
struct RgbColor {
	uint8_t red, green, blue;
	RgbColor() {}
	RgbColor(uint8_t r, uint8_t g, uint8_t b)
		: red(r), green(g), blue(b) {}
	void set(RgbColor color) { *this = color; }
};

void StepToColor(RgbColor *color, RgbColor target, int16_t percent);
void StepColorToward(RgbColor *color, int16_t red, int16_t green, int16_t blue, int16_t percent);
void SetRgb(RgbColor *color, uint8_t red, uint8_t green, uint8_t blue);

#endif
