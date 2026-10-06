/* Serpent Isle ENDGAME.EXE, resident segment 4 (file offsets 0x008588 to 0x008be2, 1626 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include <io.h>
#include <mem.h>
#include <string.h>
#include "dosio.h"
#include "linear.h"
#include "easyfile.h"
#include "chunknam.h"
#include "iff.h"
#include "view.h"
#include "palette.h"
#include "flic.h"

#define SCREEN_BYTES    64000

Flic::Flic(long *address)
{
	data = next = *address;
	palette.allocate(768, FAR_MEMORY, 0, 1, "flic palette");
	bitmap.allocate(SCREEN_BYTES, FAR_MEMORY, 0, 1, "flic bitmap");
}

/* Finds the FLIC chunk called name in the file and loads it. */
unsigned char Flic::load(IffFile *file, char *name)
{
	file->findChunk("FLIC");
	while (file->isId("FLIC")) {
		ChunkName chunkName(file);
		if (strcmp(chunkName, name)) {
			file->skipChunk();
			file->readHeader();
		} else
			break;
	}
	long unusedSize = file->loadChunk(data);
	CopyLinearToFar(&header, next, FLIC_HEADER);
	next += FLIC_HEADER;
	return 1;
}

/* Loads a FLIC file whole. */
unsigned char Flic::load(char *name)
{
	int handle = OpenFileOrFail(name);
	if (handle < 0)
		return 0;
	long length = filelength(handle);
	if (ReadHandleToVoodoo(handle, 0, length, &data) != length)
		return 0;
	CopyLinearToFar(&header, next, FLIC_HEADER);
	next += FLIC_HEADER;
	DosClose(handle);
	return 1;
}

void Flic::rewind()
{
	next = data + FLIC_HEADER;
}

/* Reads the next frame and decodes it into the bitmap. */
unsigned char Flic::nextFrame()
{
	frameSize = PeekLong(next);
	next += 4;
	frameMagic = PeekWord(next);
	next += 2;
	if (frameMagic != FRAME_MAGIC)
		return 0;
	chunks = PeekWord(next);
	next += 10;
	frameBuffer.release(0);
	frameBuffer.allocate(frameSize, FAR_MEMORY, 0, 1, "flic frame buffer");
	CopyLinearToFar(frameBuffer.lock(), next, frameSize);
	next += frameSize;
	next -= FRAME_HEADER;
	decode();
	return 1;
}

void Flic::decode()
{
	long size;
	int type;
	unsigned char huge *chunk;
	unsigned char far *screen;

	chunk = (unsigned char huge *)frameBuffer.pointer();
	for (unsigned i = 1; i <= chunks; i++) {
		screen = (unsigned char far *)bitmap.pointer();
		size = *(long huge *)chunk;
		chunk += 4;
		size -= 4;
		type = *(int huge *)chunk;
		chunk += 2;
		size -= 2;
		switch (type) {
		case FLI_COLOR:
			DecodeColor(chunk, palette.lock());
			chunk += size;
			break;
		case FLI_LC:
			DecodeDelta(chunk, screen);
			chunk += size;
			break;
		case FLI_BLACK:
			_fmemset(screen, 0, SCREEN_BYTES);
			break;
		case FLI_BRUN:
			DecodeBrun(chunk, screen, SCREEN_HEIGHT);
			chunk += size;
			break;
		case FLI_COPY:
			_fmemcpy(chunk, screen, SCREEN_BYTES);
			break;
		}
	}
}

/* Sets the frame's palette and copies its bitmap to the view. */
void Flic::show(View *view)
{
	void far *pixels;
	void far *image;
	Palette colors;

	colors.load(0, 256, (Rgb far *)palette.lock());
	colors.write(0, 256, 0);
	pixels = view->pixels();
	image = bitmap.pointer();
	_fmemcpy(pixels, image, SCREEN_BYTES);
}

/* Copies the bitmap to the view with every color set to one, ready to fade in. */
void Flic::showInColor(View *view, Rgb *color)
{
	Rgb *colors = new Rgb[256];
	for (int i = 0; i < 256; i++) {
		colors[i].red = color->red;
		colors[i].green = color->green;
		colors[i].blue = color->blue;
	}
	Palette fill(0, 256, colors);
	fill.write(0);
	void far *pixels = view->pixels();
	void far *image = bitmap.pointer();
	_fmemcpy(pixels, image, SCREEN_BYTES);
}

void Flic::play()
{
	for (unsigned i = 0; i < header.frames; i++) {
		nextFrame();
		show(CurrentView);
	}
}
