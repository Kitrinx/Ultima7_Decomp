/* Black Gate MAINMENU.EXE, resident segment 25 (file offsets 0x00f86f to 0x00ff17, 1704 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <stdlib.h>
#include "chkfile.h"
#include "flex.h"
#include "vidmode.h"
#include "rgbpal.h"

void RgbColor::set(RgbColor *color)
{
	red = color->red;
	green = color->green;
	blue = color->blue;
}

void CopyColors(RgbColor *colors, RgbPalette *palette)
{
	int count = PALETTE_COLORS;
	RgbColor *to = colors;
	RgbColor *from = palette->colors;

	while (count--)
		(to++)->set(from++);
}

/* Loads DAC registers 0 to 254 through the BIOS. */
void SetVgaPalette(RgbColor *colors)
{
	asm {
		push es
		xor bx, bx
		mov cx, 255
		push ds
		pop es
		mov dx, colors
		mov al, 12h
		mov ah, 10h
		int 10h
		pop es
	}
}

void SetDacColor(int index, RgbColor *color)
{
	asm {
		mov dx, 3c8h
		mov ax, index
		out dx, al
		inc dx
		mov bx, color
		mov ax, [bx]
		out dx, al
		mov al, ah
		out dx, al
		mov al, [bx+2]
		out dx, al
	}
}

void GetDacColor(int index, RgbColor *color)
{
	asm {
		mov dx, 3c7h
		mov ax, index
		out dx, al
		mov bx, color
		mov dx, 3c9h
		in al, dx
		mov [bx], al
		in al, dx
		mov [bx+1], al
		in al, dx
		mov [bx+2], al
	}
}

/* Steps each level one toward zero; returns 1 once all are zero. */
unsigned char DimColors(unsigned char *level)
{
	unsigned char dark = 1;
	int i;

	for (i = 0; i < 256; i++, level++)
		if (*level != 0) {
			dark = 0;
			(*level)--;
		}
	return dark;
}

void CopyColor(RgbColor *to, RgbColor *from)
{
	to->red = from->red;
	to->green = from->green;
	to->blue = from->blue;
}

void RgbPalette::randomize()
{
	int i;

	for (i = 0; i < PALETTE_COLORS; i++) {
		colors[i].red = random(64);
		colors[i].green = random(64);
		colors[i].blue = random(64);
	}
}

/* Reads the palette the card is showing. */
void RgbPalette::capture()
{
	RgbColor *color = colors;
	int i;

	for (i = 0; i < PALETTE_COLORS; i++, color++)
		GetDacColor(i, color);
}

void RgbPalette::apply()
{
	RgbColor *color = colors;
	int i;

	WaitForRetrace();
	for (i = 0; i < PALETTE_COLORS; i++, color++)
		SetDacColor(i, color);
}

/* The file holds each channel as a word; keep the low bytes. */
void RgbPalette::loadFile(char *name)
{
	DataFile file(name, 1);
	int buf[PALETTE_COLORS * 3];

	file.read(buf, sizeof buf);
	int count = PALETTE_COLORS * 3;
	unsigned char *dst = &colors[0].red;
	int *src = buf;

	while (count--)
		*dst++ = *src++;
}

void RgbPalette::load(char *flexName, int i)
{
	Flex flex;
	FlexEntry entry;
	int buf[PALETTE_COLORS * 3];

	flex.open(flexName);
	flex.getEntry(i, &entry);
	if (entry.size > sizeof buf)
		entry.size = sizeof buf;
	flex.readEntry(&entry, buf, 0);
	int count = PALETTE_COLORS * 3;
	unsigned char *dst = &colors[0].red;
	int *src = buf;

	while (count--)
		*dst++ = *src++;
	flex.close();
}

void RgbPalette::saveFile(char *name)
{
	DataFile file(name, 0);

	file.write(colors, sizeof colors);
}

/* Sets colors first to last to one color. */
void RgbPalette::fill(RgbColor *color, int first, int last)
{
	for (; first <= last; first++) {
		colors[first].red = color->red;
		colors[first].green = color->green;
		colors[first].blue = color->blue;
	}
}

RgbPalette &RgbPalette::operator=(RgbPalette &palette)
{
	int i;

	for (i = 0; i < PALETTE_COLORS; i++) {
		colors[i].red = palette.colors[i].red;
		colors[i].green = palette.colors[i].green;
		colors[i].blue = palette.colors[i].blue;
	}
	return *this;
}

int RgbPalette::fadeStep(RgbPalette *unused)
{
	WaitForRetrace();
	return fade.advance();
}

/* Copies this palette into copy, then fills copy's first to last with color. */
void RgbPalette::fillCopy(RgbColor *color, RgbPalette *copy, int first, int last)
{
	*copy = *this;
	copy->fill(color, first, last);
}

/* Saves this palette into saved, then fills first to last with color. */
void RgbPalette::fillSaving(RgbColor *color, RgbPalette *saved, int first, int last)
{
	*saved = *this;
	fill(color, first, last);
}

/* Fades first to last to color, delay retraces a step, over ticks steps. */
void RgbPalette::fadeToColor(RgbColor *color, int delay, int first, int last, int ticks)
{
	RgbPalette target;
	int wait;

	fadeTicks = ticks;
	fillCopy(color, &target, first, last);
	fade.prepare(&colors[0].red, &target.colors[0].red, sizeof colors, fadeTicks, 0);
	while (fade.advance()) {
		if (delay > 1) {
			wait = delay - 1;
			while (wait--)
				WaitForRetrace();
		}
		apply();
	}
}

/* Fills first to last with color, then fades back to the palette as it was. */
void RgbPalette::fadeFromColor(RgbColor *color, int delay, int first, int last, int ticks)
{
	RgbPalette target;
	int wait;

	fadeTicks = ticks;
	fillSaving(color, &target, first, last);
	fade.prepare(&colors[0].red, &target.colors[0].red, sizeof colors, fadeTicks, 0);
	while (fade.advance()) {
		if (delay > 1) {
			wait = delay - 1;
			while (wait--)
				WaitForRetrace();
		}
		apply();
	}
}

/* Moves colors first to last one place toward first, wrapping first round to last; shows them if asked. */
void RgbPalette::rotate(int first, int last, unsigned char show)
{
	RgbColor saved;
	int i;

	if (first < last) {
		CopyColor(&saved, &colors[first]);
		for (i = first; i < last; i++)
			CopyColor(&colors[i], &colors[i + 1]);
		CopyColor(&colors[last], &saved);
	} else {
		CopyColor(&saved, &colors[first]);
		for (i = first; i > last; i--)
			CopyColor(&colors[i], &colors[i - 1]);
		CopyColor(&colors[last], &saved);
		i = first;
		first = last;
		last = i;
	}
	if (show) {
		WaitForRetrace();
		for (i = first; i <= last; i++)
			SetDacColor(i, &colors[i]);
	}
}
