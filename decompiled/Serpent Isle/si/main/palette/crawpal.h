#ifndef CRAWPAL_H
#define CRAWPAL_H

/* bytes of voodoo memory one palette takes: 256 colors of three words */
#define PALETTE_SIZE 1536L

/* 256 colors kept in voodoo memory, with a color map that cycling rotates */
struct Palette {
	long colors;
	int order[256];
	int modified;

	Palette();
	Palette(long src);
	int changed() { return modified != 0; }
	void apply(unsigned char mode);
	void allocate();
	void loadFile(char *name);
	void setColors(long src);
	void cycle();
	void cycleRange(int);
	void copyColors(long src);
};

#endif
