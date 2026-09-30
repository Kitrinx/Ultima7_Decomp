#ifndef FLIC_H
#define FLIC_H

#include "memsys.h"

struct IffFile;
struct View;
struct Rgb;

#define FLIC_HEADER     128
#define FRAME_HEADER    16
#define FRAME_MAGIC     0xF1FAL

/* Chunk types inside a frame. */
#define FLI_COLOR       11
#define FLI_LC          12
#define FLI_BLACK       13
#define FLI_BRUN        15
#define FLI_COPY        16

/* The start of a FLIC file. */
struct FlicHeader {
	long size;
	unsigned magic;
	unsigned frames;
	unsigned width;
	unsigned height;
	char rest[114];
};

/* A FLIC animation kept in linear memory and decoded a frame at a time. */
struct Flic {
	FlicHeader header;
	long frameSize;
	unsigned frameMagic;
	unsigned chunks;
	char reserved[8];
	MemHandle frameBuffer;
	MemHandle palette;
	MemHandle bitmap;
	long data;
	long next;
	Flic(long *address);
	~Flic() {}
	unsigned char load(IffFile *file, char *name);
	unsigned char load(char *name);
	void rewind();
	unsigned char nextFrame();
	void decode();
	void show(View *view);
	void showInColor(View *view, Rgb *color);
	void play();
	int frameCount() { return header.frames; }
	long length() { return header.size; }
	void far *getPalette() { return palette.pointer(); }
};

#ifdef __cplusplus
extern "C" {
#endif
void far DecodeBrun(void far *data, void far *screen, int lines);
void far DecodeDelta(void far *data, void far *screen);
void far DecodeColor(void far *data, void far *palette);
#ifdef __cplusplus
}
#endif

#endif
