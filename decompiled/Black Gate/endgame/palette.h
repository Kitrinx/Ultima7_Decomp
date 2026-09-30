#ifndef PALETTE_H
#define PALETTE_H

struct MemHandle;

/* The smaller of a and b. */
inline int Smaller(int a, int b) { return a < b ? a : b; }

/* One DAC entry, six bits a component. */
struct Rgb {
	unsigned char red, green, blue;
	Rgb() {}
	Rgb(unsigned char r, unsigned char g, unsigned char b) { red = r; green = g; blue = b; }
};

/* A run of count DAC entries starting at start. */
struct Palette {
	unsigned count;
	unsigned char start;
	Rgb *colors;
	Palette() { count = 0; colors = 0; }
	Palette(unsigned char first, int n) { init(first, n); }
	Palette(unsigned char first, int n, Rgb *from) { load(first, n, from); }
	Palette(Palette &p) { init(0, 256); merge(p); }
	~Palette();
	Palette &operator=(Palette &p) { merge(p); return *this; }
	void merge(Palette &p) { copy(&p, p.start, p.count); }
	void init(unsigned char first, int n);
	void set(Rgb far *from);
	void load(MemHandle *from);
	void load(unsigned char first, int n, Rgb far *from);
	void read();
	void write(unsigned char first, unsigned n, unsigned batch);
	void write(unsigned batch) { write(start, count, batch); }
	void read(unsigned char first, unsigned n);
	void fill(Rgb *color, unsigned char first, unsigned n);
	void copy(Palette *from, unsigned char first, unsigned n);
	void writeOne(Rgb *color, unsigned char index);
	unsigned char step(Palette *target);
	void loadAndWrite(MemHandle *from);
	void fadeToColor(Rgb *color, unsigned batch, unsigned char first, unsigned n);
	void fadeFromColor(Rgb *color, unsigned batch, unsigned char first, unsigned n);
};

/* A working copy of a palette, faded to and from a color while the original stays as it was. */
struct FadePalette : Palette {
	FadePalette(Palette &p) : Palette(0, 256) { merge(p); }
	FadePalette &operator=(Palette &p) { merge(p); return *this; }
};

int pascal StepPaletteComponents(unsigned char *current, unsigned char *target, unsigned count);
extern "C" {
void far pascal WriteDacEntry(unsigned char index, Rgb *color);
void far pascal WriteDac(unsigned char first, int count, Rgb *from);
void far pascal ReadDacEntry(unsigned char index, Rgb *color);
void far pascal ReadDac(unsigned char first, unsigned count, Rgb *to);
}

#endif
