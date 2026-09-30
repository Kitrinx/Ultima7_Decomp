#ifndef CRAWPAL_H
#define CRAWPAL_H

/* bytes of voodoo memory one palette takes: 256 colors of three words */
#define PALETTE_SIZE INT32_C(1536)

/* 256 colors kept in voodoo memory, with a color map that cycling rotates */
struct Palette {
	int32_t colors;
	int16_t order[256];
	int16_t modified;

	Palette();
	Palette(int32_t src);
	int16_t changed() { return modified != 0; }
	void apply(uint8_t mode);
	void allocate();
	void loadFile(char *name);
	void setColors(int32_t src);
	void cycle();
	void cycleRange(int16_t);
	void copyColors(int32_t src);
};

#endif
