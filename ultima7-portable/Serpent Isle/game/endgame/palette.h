#ifndef ENDGAME_PALETTE_H
#define ENDGAME_PALETTE_H

namespace Endgame {

/* One DAC entry, six bits a component. */
struct Rgb {
	uint8_t red, green, blue;
	Rgb() {}
	Rgb(uint8_t r, uint8_t g, uint8_t b) { red = r; green = g; blue = b; }
};

/* A run of count DAC entries starting at start. */
struct Palette {
	uint16_t count;
	uint8_t start;
	Rgb *colors;
	Palette() { count = 0; colors = 0; }
	Palette(uint8_t first, int16_t n) { init(first, n); }
	Palette(uint8_t first, int16_t n, Rgb *from) { load(first, n, from); }
	~Palette();
	Palette &operator=(Palette &p) { merge(p); return *this; }
	void merge(Palette &p) { copy(&p, p.start, p.count); }
	void init(uint8_t first, int16_t n);
	void set(Rgb *from);
	void load(uint8_t first, int16_t n, Rgb *from);
	void write(uint8_t first, uint16_t n, uint16_t batch);
	void write(uint16_t batch) { write(start, count, batch); }
	void fill(Rgb *color, uint8_t first, uint16_t n);
	void fill(Rgb *color) { fill(color, start, count); }
	void copy(Palette *from, uint8_t first, uint16_t n);
	void writeOne(Rgb *color, uint8_t index);
	uint8_t step(Palette *target);
	void fadeToColor(Rgb *color, uint16_t batch, uint8_t first, uint16_t n);
	void fadeFromColor(Rgb *color, uint16_t batch, uint8_t first, uint16_t n);
};

/* A working copy of a palette, faded to and from a color while the original stays as it was. */
struct FadePalette : Palette {
	FadePalette() : Palette(0, 256) {}
	FadePalette(Palette &p) : Palette(0, 256) { merge(p); }
	FadePalette &operator=(Palette &p) { merge(p); return *this; }
};

void WriteDac(uint8_t first, int16_t count, Rgb *from);

}

#endif
