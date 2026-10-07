/* Serpent Isle MAINMENU.EXE, resident segment 2 (file offsets 0x00ab9a to 0x00badf, 3909 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <stdlib.h>
#include "plat.h"
#include "dosio.h"
#include "lowlevel.h"
#include "../shared/rgbpal.h"
#include "../shared/fadepal.h"
#include "controls.h"
#include "textspr.h"
#include "../shared/farbuf.h"
#include "../shared/textfile.h"
#include "scroll.h"
#include "systimer.h"
#include "../shared/music.h"
#include "../shared/specard.h"
#include "../shared/cflxbuf.h"
#include "../shared/keys.h"
#include "credits.h"
#include "mainmenu.h"

namespace MainMenu {

using Shared::FadingPalette;
using Shared::TextFile;
using Shared::Song;
using Shared::SpeechCard;
using Shared::KeyHit;
using Shared::GetKey;

#define SCROLL_LINES    21      /* lines on screen at once, reused as they scroll off */
#define CENTRE_X        159

void ShowTheEnd(uint32_t pause)
{
	FadingPalette palette;
	FadingPalette fader;

	palette.fill(&Black, 0, 255);
	palette.apply();
	palette.load(DataPath(StaticPath, "mainshp.flx"), 26);
	Screen screen(0);
	TextFile text(DataPath(StaticPath, "mainshp.flx"), 21);
	int16_t count = text.getCount();
	TextSprite **lines = new TextSprite *[count];
	int16_t spacing = 10;
	TextSprite **line = lines;
	int16_t i;

	for (i = 0; i < count; i++, line++) {
		*line = new TextSprite(LinearToPointer(MenuFont.getShape()), 0, &screen, 0);
		(*line)->moveTo(CENTRE_X, i * spacing + 30);
		(*line)->setAlign(ALIGN_CENTRE);
		(*line)->setSpacing(1);
		(*line)->setText(text.getLine(i), -1);
	}
	screen.paint(0, 0);
	palette.fadeFromColor(&Black, 0, 0, 255, FadeTicks);
	Timer timer;
	Timer_delay(&timer, pause);
	for (i = 1; i < count; i++)
		lines[i]->hide();
	lines[0]->moveTo(CENTRE_X, 100);
	lines[0]->setText("\\CTHE END OF ULTIMA VII", -1);
	fader = palette;
	fader.fadeToColor(&Black, 0, 0, 255, FadeTicks);
	screen.paint(0, 0);
	fader = palette;
	fader.fadeFromColor(&Black, 0, 0, 255, FadeTicks);
	Timer_delay(&timer, 300);
	lines[0]->setText("\\CTHE END OF BRITANNIA AS YOU KNOW IT...", -1);
	fader = palette;
	fader.fadeToColor(&Black, 0, 0, 255, FadeTicks);
	screen.paint(0, 0);
	fader = palette;
	fader.fadeFromColor(&Black, 0, 0, 255, FadeTicks);
	Timer_delay(&timer, 300);
	palette.fadeToColor(&Red, 0, 0, 255, FadeTicks);
	palette.fadeToColor(&Black, 0, 0, 255, FadeTicks);
	FillView(&ScreenView, 0);
}

void ShowQuotes(uint32_t stepTicks, int16_t unused)
{
	Song song(&Music, DataPath(StaticPath, "mainshp.flx"), MusicResourceType + 31);
	song.play();
	FadingPalette palette;
	palette.fill(&Black, 0, 255);
	palette.apply();
	palette.load(DataPath(StaticPath, "mainshp.flx"), 26);
	palette.apply();
	Screen screen(0);
	TextFile text(DataPath(StaticPath, "mainshp.flx"), 16);
	FarBuffer image;
	image.load(DataPath(StaticPath, "mainshp.flx"), 20);
	int16_t next = 0;
	int16_t spacing = 10;
	uint8_t done;
	int16_t oinks;
	int16_t top;
	uint32_t start, now, elapsed;
	int8_t moved;
	int16_t bottom;
	ScrollLine *lines[SCROLL_LINES];
	{
		ScrollLine **line = lines;
		for (int16_t i = 0; i < SCROLL_LINES; i++, line++) {
			*line = new ScrollLine(LinearToPointer(MenuFont.getShape()), 0, &screen, 0);
			(*line)->moveTo(CENTRE_X, i * spacing + 200);
			(*line)->setAlign(ALIGN_CENTRE);
			(*line)->setSpacing(1);
			(*line)->setOwnImage(image.get());
			if (next >= text.getCount())
				break;
			(*line)->setText(text.getLine(next++));
		}
	}
	done = 0;
	oinks = 0;
	top = 0;
	bottom = top - 1;
	if (bottom < 0)
		bottom = SCROLL_LINES - 1;
	screen.paint(0, 0);
	start = TickCount;
	while (!done) {
		now = TickCount;
		elapsed = now - start;
		moved = 0;
		while (elapsed > stepTicks) {
			moved = 1;
			if (lines[top]->aboveTop()) {
				lines[top]->clearMarks();
				if (next >= text.getCount()) {
					done = 1;
					break;
				}
				if (!lines[bottom]->aboveMiddle())
					lines[top]->moveTo(CENTRE_X, lines[bottom]->getY() + spacing);
				else
					lines[top]->moveTo(CENTRE_X, 200);
				lines[top]->setText(text.getLine(next++));
				if (++top >= SCROLL_LINES)
					top = 0;
				if (++bottom >= SCROLL_LINES)
					bottom = 0;
			}
			for (int16_t n = 0; n < SCROLL_LINES; n++) {
				if (!lines[n]->aboveTop()) {
					lines[n]->moveBy(0, -1);
					if (lines[n]->isMarked(0) && lines[n]->aboveMiddle(0)) {
						if (oinks++ % 5 == 0)
							lines[n]->setText("\\LOINK! OINK!", 0);
						else
							lines[n]->setText("\\Loink! oink!", 0);
						lines[n]->moveBy(2, 0, 0);
					}
				}
			}
			elapsed -= stepTicks;
		}
		if (done) {
			done = 0;
			break;
		}
		if (moved) {
			screen.paint(0, 0);
			start = now - elapsed;
		}
		if (KeyHit()) {
			GetKey();
			done = 1;
		}
		plat_yield();
	}
	if (done)
		goto finish;
	while (!lines[bottom]->aboveMiddle()) {
		now = TickCount;
		elapsed = now - start;
		moved = 0;
		while (elapsed > stepTicks) {
			moved = 1;
			for (int16_t n = 0; n < SCROLL_LINES; n++)
				if (!lines[n]->aboveTop())
					lines[n]->moveBy(0, -1);
			elapsed -= stepTicks;
		}
		if (moved) {
			screen.paint(0, 0);
			start = now - elapsed;
		}
		if (KeyHit())
			break;
		plat_yield();
	}
	while (!song.finished()) {
		if (KeyHit()) {
			GetKey();
			break;
		}
		plat_yield();
	}
finish:
	song.fadeOut(120);
	palette.fadeToColor(&Black, 0, 0, 255, FadeTicks);
	FillView(&ScreenView, 0);
	for (int16_t j = 0; j < SCROLL_LINES; j++)
		delete lines[j];
}

void ShowCredits(uint32_t stepTicks, int16_t unused)
{
	FadingPalette palette;
	palette.fill(&Black, 0, 255);
	palette.apply();
	palette.load(DataPath(StaticPath, "mainshp.flx"), 26);
	palette.apply();
	Screen screen(0);
	TextFile text(DataPath(StaticPath, "mainshp.flx"), 14);
	FarBuffer image;
	image.load(DataPath(StaticPath, "mainshp.flx"), 20);
	int16_t next = 0;
	int16_t spacing = 10;
	uint8_t done;
	int16_t unusedCount;
	int16_t top;
	uint32_t start, now, elapsed;
	int8_t moved;
	int16_t bottom;
	ScrollLine *lines[SCROLL_LINES];
	{
		int16_t y = 200;
		ScrollLine **line = lines;
		for (int16_t i = 0; i < SCROLL_LINES; i++, line++) {
			*line = new ScrollLine(LinearToPointer(MenuFont.getShape()), 0, &screen, 0);
			(*line)->moveTo(CENTRE_X, y);
			(*line)->setAlign(ALIGN_CENTRE);
			(*line)->setSpacing(1);
			(*line)->setOwnImage(image.get());
			if (next >= text.getCount())
				break;
			(*line)->setText(text.getLine(next++));
			y += (*line)->isText() ? spacing : (*line)->imageHeight();
		}
	}
	done = 0;
	top = 0;
	unusedCount = 0;
	bottom = top - 1;
	if (bottom < 0)
		bottom = SCROLL_LINES - 1;
	Song song(&Music, DataPath(StaticPath, "mainshp.flx"), MusicResourceType + 29);
	song.play();
	screen.paint(0, 0);
	start = TickCount;
	while (!done) {
		now = TickCount;
		elapsed = now - start;
		moved = 0;
		while (elapsed > stepTicks) {
			moved = 1;
			if (lines[top]->aboveTop()) {
				lines[top]->clearMarks();
				if (next >= text.getCount()) {
					done = 1;
					break;
				}
				if (!lines[bottom]->aboveMiddle()) {
					ScrollLine *last = lines[bottom];
					if (last->isText())
						lines[top]->moveTo(CENTRE_X, last->getY() + spacing);
					else
						lines[top]->moveTo(CENTRE_X, last->getY() + last->imageHeight());
				} else
					lines[top]->moveTo(CENTRE_X, 200);
				lines[top]->setText(text.getLine(next++));
				if (++top >= SCROLL_LINES)
					top = 0;
				if (++bottom >= SCROLL_LINES)
					bottom = 0;
			}
			for (int16_t n = 0; n < SCROLL_LINES; n++)
				if (!lines[n]->aboveTop())
					lines[n]->moveBy(0, -1);
			elapsed -= stepTicks;
		}
		if (done) {
			done = 0;
			break;
		}
		if (moved) {
			screen.paint(0, 0);
			start = now - elapsed;
		}
		if (KeyHit()) {
			GetKey();
			done = 1;
		}
		plat_yield();
	}
	if (done == 0) {
		/* Watching the credits to the end opens the quotes. */
		CreateFlag(DataPath(StaticPath, "quotes.flg"));
		while (!lines[bottom]->aboveMiddle()) {
			now = TickCount;
			elapsed = now - start;
			moved = 0;
			while (elapsed > stepTicks) {
				moved = 1;
				for (int16_t n = 0; n < SCROLL_LINES; n++)
					if (!lines[n]->aboveTop())
						lines[n]->moveBy(0, -1);
				elapsed -= stepTicks;
			}
			if (moved) {
				screen.paint(0, 0);
				start = now - elapsed;
			}
			if (KeyHit())
				break;
			plat_yield();
		}
		while (!song.finished() || !SpeechCard.finished()) {
			if (KeyHit()) {
				GetKey();
				break;
			}
			plat_yield();
		}
	}
	song.fadeOut(120);
	palette.fadeToColor(&Red, 0, 0, 255, FadeTicks);
	palette.fadeToColor(&Black, 0, 0, 255, FadeTicks);
	FillView(&ScreenView, 0);
	for (int16_t j = 0; j < SCROLL_LINES; j++)
		delete lines[j];
}

}
