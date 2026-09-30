#ifndef SHARED_RGBPAL_H
#define SHARED_RGBPAL_H

namespace Shared {

#define PALETTE_COLORS  256

/* One palette color, 6 bits per channel as the VGA DAC takes it. */
struct RgbColor {
	uint8_t red, green, blue;
	void set(RgbColor *color);
};

/*
 * A palette fade in progress. Each component moves one unit toward its target whenever its
 * accumulator, fed rate per tick, passes its period, so every component arrives together.
 */
struct Fade {
	uint8_t *colors;
	uint8_t *target;
	int16_t size;
	uint16_t *accum;
	uint16_t *period;
	int16_t *step;              /* +1 or -1 */
	uint8_t *active;
	int16_t steps;              /* ticks left */
	int16_t rate;
	Fade() { accum = period = 0; step = 0; steps = rate = 0; active = 0; }
	~Fade() { release(); }
	int16_t prepare(uint8_t *from, uint8_t *to, int16_t count, int16_t ticks, int16_t unused);
	int16_t advance();
	void release();
};

/* The VGA palette, and a fade toward another. */
struct RgbPalette {
	int16_t fadeTicks;
	RgbColor colors[PALETTE_COLORS];
	Fade fade;
	void randomize();
	void capture();
	void apply();
	void loadFile(char *name);
	void load(char *flexName, int16_t i);
	void saveFile(char *name);
	void fill(RgbColor *color, int16_t first, int16_t last);
	RgbPalette &operator=(RgbPalette &palette);
	int16_t fadeStep(RgbPalette *unused);
	void fillCopy(RgbColor *color, RgbPalette *copy, int16_t first, int16_t last);
	void fillSaving(RgbColor *color, RgbPalette *saved, int16_t first, int16_t last);
	void fadeToColor(RgbColor *color, int16_t delay, int16_t first, int16_t last, int16_t ticks);
	void fadeFromColor(RgbColor *color, int16_t delay, int16_t first, int16_t last, int16_t ticks);
	void rotate(int16_t first, int16_t last, uint8_t show);
};

void CopyColors(RgbColor *to, RgbPalette *palette);
void SetVgaPalette(RgbColor *colors);
void SetDacColor(int16_t index, RgbColor *color);
void GetDacColor(int16_t index, RgbColor *color);
uint8_t DimColors(uint8_t *level);
void CopyColor(RgbColor *to, RgbColor *from);

}

#endif
