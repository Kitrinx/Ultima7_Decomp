#ifndef ENDGAME_FLIC_H
#define ENDGAME_FLIC_H

struct View;

namespace Endgame {

struct IffFile;
struct Rgb;

#define FLIC_HEADER     128
#define FRAME_HEADER    16
#define FRAME_MAGIC     0xF1FA
#define SCREEN_BYTES    64000

/* A FLIC animation held whole in memory and decoded a frame at a time. */
struct Flic {
	uint8_t *data;
	int32_t size;
	uint16_t frames;
	int32_t next;
	uint8_t *palette;
	uint8_t *bitmap;
	Flic()
	{
		data = 0;
		size = 0;
		frames = 0;
		next = 0;
		palette = new uint8_t[768];
		bitmap = new uint8_t[SCREEN_BYTES];
	}
	~Flic() { delete[] data; delete[] bitmap; delete[] palette; }
	uint8_t load(IffFile *file, const char *name);
	void rewind() { next = FLIC_HEADER; }
	uint8_t nextFrame();
	void decode(const uint8_t *chunk, uint16_t chunks);
	void show(struct ::View *view);
	void showInColor(struct ::View *view, Rgb *color);
	int16_t frameCount() { return frames; }
	uint8_t *getPalette() { return palette; }
};

}

#endif
