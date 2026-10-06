/* Serpent Isle ENDGAME.EXE, resident segment 26 (file offsets 0x00c213 to 0x00c811, 1534 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include "palette.h"
#include "memsys.h"
#include "vidmode.h"

/* Makes room for n entries from first. */
void Palette::init(unsigned char first, int n)
{
	start = first;
	count = n;
	colors = new Rgb[n];
}

/* Copies every entry from far memory. */
void Palette::set(Rgb far *from)
{
	Rgb far *p = from;
	Rgb *c = colors;

	for (unsigned i = 0; i < count; i++, p++, c++) {
		c->red = p->red;
		c->green = p->green;
		c->blue = p->blue;
	}
}

/* Loads a palette resource: a start word, a count word and the entries. */
void Palette::load(MemHandle *from)
{
	int far *p;

	if (colors) {
		delete colors;
		colors = 0;
	}
	p = (int far *)from->lock();
	start = *p++;
	count = *p++;
	colors = new Rgb[count];
	if (colors)
		set((Rgb far *)p);
	from->release(0);
}

void Palette::load(unsigned char first, int n, Rgb far *from)
{
	init(first, n);
	set(from);
}

/* Reads the whole run back from the DAC. */
void Palette::read()
{
	if (colors)
		ReadDac(start, count, colors);
}

/* Writes n entries from first to the DAC, batch entries each vertical retrace; all at once when batch
 * is 0. */
void Palette::write(unsigned char first, unsigned n, unsigned batch)
{
	Rgb *c;

	if (colors == 0)
		return;
	if (start > first)
		first = start;
	if (start + count < first + n)
		n = start + count - first;
	c = colors + (first - start);
	if (batch) {
		unsigned size = n / batch;
		unsigned char index = first;
		int left = n;
		if (n % batch)
			size++;
		while (size > 0) {
			WaitForRetrace();
			WriteDac(index, size, c);
			index += size;
			c += size;
			size = Smaller(left = left - size, size);
		}
	} else
		WriteDac(first, n, c);
}

/* Reads n entries from first back from the DAC. */
void Palette::read(unsigned char first, unsigned n)
{
	if (colors) {
		if (start > first)
			first = start;
		if (start + count < first + n)
			n = start + count - first;
		ReadDac(first, n, colors + (first - start));
	}
}

/* Sets n entries from first to one color. */
void Palette::fill(Rgb *color, unsigned char first, unsigned n)
{
	if (colors) {
		if (start > first)
			first = start;
		if (start + count < first + n)
			n = start + count - first;
		Rgb *c = colors;
		/* moves the run's own pointer; the fill still starts at the old one */
		colors = colors + (first - start);
		for (unsigned i = 0; i < n; i++, c++) {
			c->red = color->red;
			c->green = color->green;
			c->blue = color->blue;
		}
	}
}

/* Copies all of another palette's entries into this one at the other's start; first and n are only
 * clipped. */
void Palette::copy(Palette *from, unsigned char first, unsigned n)
{
	if (colors == 0 || from->colors == 0)
		return;
	if (start > first)
		first = start;
	if (start + count < first + n)
		n = start + count - first;
	Rgb *s = from->colors;
	Rgb *d = colors + from->start;
	for (unsigned i = 0; i < from->count; i++, s++, d++) {
		d->red = s->red;
		d->green = s->green;
		d->blue = s->blue;
	}
}

/* Writes one entry, if it is in the run. */
void Palette::writeOne(Rgb *color, unsigned char index)
{
	if (start < index && start + count > index)
		WriteDacEntry(index, color);
}

/* Moves every entry one step toward target's. Returns 0 once they are all there. */
unsigned char Palette::step(Palette *target)
{
	unsigned char *current = (unsigned char *)(colors + target->start);
	unsigned char *goal = (unsigned char *)target->colors;

	return StepPaletteComponents(current, goal, target->count * 3);
}

Palette::~Palette()
{
	if (colors)
		delete colors;
}

void Palette::loadAndWrite(MemHandle *from)
{
	load(from);
	write(start, count, 0);
}

/* Fades n entries from first to color. */
void Palette::fadeToColor(Rgb *color, unsigned batch, unsigned char first, unsigned n)
{
	Rgb *solid = new Rgb[n];

	for (unsigned i = 0; i < n; i++) {
		solid[i].red = color->red;
		solid[i].blue = color->blue;
		solid[i].green = color->green;
	}
	Palette target(first, n, solid);
	while (step(&target))
		write(start, count, batch);
}

/* Fades n entries from first in from color to what the palette holds now. */
void Palette::fadeFromColor(Rgb *color, unsigned batch, unsigned char first, unsigned n)
{
	Palette target(first, n);
	target.copy(this, start, count);
	fill(color, first, n);
	while (step(&target))
		write(start, count, batch);
}
