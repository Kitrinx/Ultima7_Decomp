#ifndef RGBPAL_H
#define RGBPAL_H

#define PALETTE_COLORS  256

/* One palette color, 6 bits per channel as the VGA DAC takes it. */
struct RgbColor {
	unsigned char red, green, blue;
	void set(RgbColor *color);
};

/*
 * A palette fade in progress. Each component moves one unit toward its target whenever its
 * accumulator, fed rate per tick, passes its period, so every component arrives together.
 */
struct Fade {
	unsigned char *colors;
	unsigned char *target;
	int size;
	int *accum;
	int *period;
	int *step;                  /* +1 or -1 */
	unsigned char *active;
	int steps;                  /* ticks left */
	int rate;
	Fade() { accum = period = step = 0; steps = rate = 0; active = 0; }
	~Fade() { release(); }
	int prepare(unsigned char *from, unsigned char *to, int count, int ticks, int unused);
	int advance();
	void release();
};

/* The VGA palette, and a fade toward another. */
struct RgbPalette {
	int fadeTicks;
	RgbColor colors[PALETTE_COLORS];
	Fade fade;
	void randomize();
	void capture();
	void apply();
	void loadFile(char *name);
	void load(char *flexName, int i);
	void saveFile(char *name);
	void fill(RgbColor *color, int first, int last);
	RgbPalette &operator=(RgbPalette &palette);
	int fadeStep(RgbPalette *unused);
	void fillCopy(RgbColor *color, RgbPalette *copy, int first, int last);
	void fillSaving(RgbColor *color, RgbPalette *saved, int first, int last);
	void fadeToColor(RgbColor *color, int delay, int first, int last, int ticks);
	void fadeFromColor(RgbColor *color, int delay, int first, int last, int ticks);
	void rotate(int first, int last, unsigned char show);
};

void CopyColors(RgbColor *to, RgbPalette *palette);
void SetVgaPalette(RgbColor *colors);
void SetDacColor(int index, RgbColor *color);
void GetDacColor(int index, RgbColor *color);
unsigned char DimColors(unsigned char *level);
void CopyColor(RgbColor *to, RgbColor *from);

#endif
