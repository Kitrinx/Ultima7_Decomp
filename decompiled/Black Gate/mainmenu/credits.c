/* Black Gate MAINMENU.EXE, resident segment 2 (file offsets 0x00a021 to 0x00b5a6, 5509 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include <conio.h>
#include <stdlib.h>
#include "lowlevel.h"
#include "rgbpal.h"
#include "fadepal.h"
#include "controls.h"
#include "textspr.h"
#include "farbuf.h"
#include "textfile.h"
#include "scroll.h"
#include "systimer.h"
#include "music.h"
#include "specard.h"
#include "cflxbuf.h"
#include "debug.h"
#include "credits.h"

/* Defined by the main menu. */
extern char *MusicFlex;
extern char SpeechEnabled;
extern int FadeTicks;
extern FlexTextPrinter MenuFont;
extern MusicSystem Music;
extern FlexSpeechCache SpeechCache;
extern RgbColor Black;
extern RgbColor Red;

char *DataPath(char *dir, char *name);
void Delay(unsigned ticks);
void CreateFlag(char *name);

#define SCROLL_LINES    21      /* lines on screen at once, reused as they scroll off */
#define CENTRE_X        159
#define FLIGHT_POINTS   96

/* A stopwatch that can be stopped and restarted. */
struct Stopwatch {
	char running;
	unsigned long start, total;
	Stopwatch() { total = start = 0; }
	void begin() { start = TickCount; running = 1; }
};

/* The thunderclap of the bolt that ends the butterfly. */
SoundEffect ZapSound(0, 116, 60, 127, 60, 0, 0);

/* The butterfly's flight, one point every three steps. */
static int flightX[FLIGHT_POINTS] = {
	6, 18, 30, 41, 52, 62, 70, 78, 86, 95, 104, 113, 122, 132, 139, 146, 151, 155, 157, 158, 157, 155, 151, 146,
	139, 132, 124, 116, 108, 102, 96, 93, 93, 93, 95, 99, 109, 111, 118, 125, 132, 140, 148, 157, 164, 171, 178,
	184, 190, 196, 203, 211, 219, 228, 237, 246, 254, 259, 262, 264, 265, 265, 263, 260, 256, 251, 245, 239, 232,
	226, 219, 212, 208, 206, 206, 209, 212, 216, 220, 224, 227, 234, 231, 232, 233, 233, 233, 233, 234, 236, 239,
	243, 247, 250, 258, 265
};
static int flightY[FLIGHT_POINTS] = {
	155, 153, 151, 150, 149, 148, 148, 148, 148, 149, 150, 150, 150, 149, 147, 142, 137, 131, 125, 118, 110, 103,
	98, 94, 92, 91, 91, 91, 92, 95, 99, 104, 110, 117, 123, 127, 131, 134, 135, 135, 135, 135, 135, 134, 132, 129,
	127, 123, 119, 115, 112, 109, 104, 102, 101, 102, 109, 109, 114, 119, 125, 131, 138, 144, 149, 152, 156, 158,
	159, 159, 158, 155, 150, 144, 137, 130, 124, 118, 112, 105, 99, 93, 86, 80, 73, 66, 59, 53, 47, 42, 38, 35, 32,
	29, 26, 25
};

void ShowButterfly(unsigned speed)
{
	FadingPalette palette;
	palette.load(DataPath(StaticPath, "intropal.dat"), 4);
	palette.apply();
	FadingPalette flash;
	flash.load(DataPath(StaticPath, "intropal.dat"), 7);
	Screen screen(0);
	BouncingSprite butterfly(DataPath(StaticPath, "endshape.flx"), 14, 0, &screen, 0);
	Sprite zap(".\\static\\sprites.vga", 1, 0, &screen, 0);

	zap.hide();
	{
		Stopwatch clock;
		Song song(&Music, MusicFlex, 0);

		clock.begin();
		song.play();
		int point = 0;
		int frames = 0;
		while (point++ < FLIGHT_POINTS - 1 && !kbhit()) {
			butterfly.setPath(flightX[point - 1], flightY[point - 1], flightX[point], flightY[point], 3);
			while (butterfly.step != butterfly.steps) {
				if (random(5) < 4)
					butterfly.cycleFrames(0, 3);
				butterfly.advance();
				screen.paint(0, 0);
				frames++;
				while (Stopwatch_getElapsed(&clock) / speed < frames)
					;
			}
		}
		butterfly.frame = 3;
		screen.paint(0, 0);
		Delay(10);
		butterfly.frame = 4;
		screen.paint(0, 0);
		Delay(25);
		butterfly.frame = 3;
		screen.paint(0, 0);
		Delay(15);
		butterfly.frame = 4;
		screen.paint(0, 0);
		Delay(25);
		butterfly.frame = 3;
		screen.paint(0, 0);
		Delay(25);
		butterfly.frame = 2;
		screen.paint(0, 0);
		Delay(25);
		butterfly.frame = 1;
		screen.paint(0, 0);
		Delay(25);
		butterfly.frame = 0;
		screen.paint(0, 0);
		Delay(20);
		song.fadeOut(0);
	}
	butterfly.hide();
	screen.paint(0, 0);
	flash.apply();
	ZapSound.play(-1);
	zap.frame = 0;
	zap.show();
	zap.moveTo(butterfly.x, butterfly.y);
	while (zap.lastFrame() > zap.frame) {
		screen.paint(0, 0);
		zap.stepForward();
		Delay(4);
	}
	zap.hide();
	screen.paint(0, 0);
	if (SpeechEnabled) {
		SpeechCard.resetPlayback();
		SpeechCache.playEntry(DataPath(StaticPath, "u7speech.spc"), 21, 0);
		SpeechCard.play(&SpeechCache);
		while (!SpeechFinished)
			;
		SpeechCard.stop();
	}
	FillView(&ScreenView, 0);
	flash.fadeToColor(&Black, 0, 0, 255, FadeTicks);
	FillView(&ScreenView, 0);
}

void ShowTheEnd(unsigned long pause)
{
	FadingPalette palette;
	FadingPalette fader;

	palette.fill(&Black, 0, 255);
	palette.apply();
	palette.load(DataPath(StaticPath, "intropal.dat"), 6);
	Screen screen(0);
	TextFile text(DataPath(StaticPath, "mainshp.flx"), 21);
	int count = text.count;
	TextSprite **lines = new TextSprite *[count];
	int spacing = 10;
	TextSprite **line = lines;
	int i;

	for (i = 0; i < count; i++, line++) {
		*line = new TextSprite((void far *) MenuFont.shape, 0, &screen, 0);
		(*line)->moveTo(CENTRE_X, i * spacing + 30);
		(*line)->setAlign(ALIGN_CENTRE);
		(*line)->spacing = 1;
		(*line)->setText(text.getLine(i), -1);
	}
	screen.paint(0, 0);
	palette.fadeFromColor(&Black, 0, 0, 255, FadeTicks);
	Timer timer;
	Timer_delay(&timer, pause);
	for (i = 1; i < count; i++)
		lines[i]->hide();
	if (SpeechEnabled) {
		SpeechCard.resetPlayback();
		SpeechCache.playEntry(DataPath(StaticPath, "u7speech.spc"), 21, 0);
		SpeechCard.play(&SpeechCache);
	}
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
	if (SpeechEnabled) {
		while (!SpeechFinished)
			;
		SpeechCard.stop();
	}
	palette.fadeToColor(&Red, 0, 0, 255, FadeTicks);
	palette.fadeToColor(&Black, 0, 0, 255, FadeTicks);
	FillView(&ScreenView, 0);
}

void ShowQuotes(unsigned long stepTicks, int unused)
{
	Song song(&Music, MusicFlex, 5);
	song.play();
	FadingPalette palette;
	palette.fill(&Black, 0, 255);
	palette.apply();
	palette.load(DataPath(StaticPath, "intropal.dat"), 6);
	palette.apply();
	Screen screen(0);
	TextFile text(DataPath(StaticPath, "mainshp.flx"), 16);
	FarBuffer image;
	image.load(DataPath(StaticPath, "mainshp.flx"), 20);
	int next = 0;
	int spacing = 10;
	unsigned char done;
	int oinks;
	int top;
	unsigned long start, now, elapsed;
	char moved;
	int bottom;
	ScrollLine *lines[SCROLL_LINES];
	{
		ScrollLine **line = lines;
		for (int i = 0; i < SCROLL_LINES; i++, line++) {
			*line = new ScrollLine((void far *) MenuFont.shape, 0, &screen, 0);
			(*line)->moveTo(CENTRE_X, i * spacing + 200);
			(*line)->setAlign(ALIGN_CENTRE);
			(*line)->setSpacing(1);
			(*line)->setOwnImage(image.data);
			if (next >= text.count)
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
				if (next >= text.count) {
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
			for (int n = 0; n < SCROLL_LINES; n++) {
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
		if (kbhit()) {
			getch();
			done = 1;
		}
	}
	if (done)
		goto finish;
	while (!lines[bottom]->aboveMiddle()) {
		now = TickCount;
		elapsed = now - start;
		moved = 0;
		while (elapsed > stepTicks) {
			moved = 1;
			for (int n = 0; n < SCROLL_LINES; n++)
				if (!lines[n]->aboveTop())
					lines[n]->moveBy(0, -1);
			elapsed -= stepTicks;
		}
		if (moved) {
			screen.paint(0, 0);
			start = now - elapsed;
		}
		if (kbhit())
			break;
	}
	while (!song.finished())
		if (kbhit()) {
			getch();
			break;
		}
finish:
	song.fadeOut(120);
	palette.fadeToColor(&Black, 0, 0, 255, FadeTicks);
	FillView(&ScreenView, 0);
	for (int j = 0; j < SCROLL_LINES; j++)
		delete lines[j];
}

void ShowCredits(unsigned long stepTicks, int unused)
{
	FadingPalette palette;
	palette.fill(&Black, 0, 255);
	palette.apply();
	palette.load(DataPath(StaticPath, "intropal.dat"), 6);
	palette.apply();
	Screen screen(0);
	TextFile text(DataPath(StaticPath, "mainshp.flx"), 14);
	FarBuffer image;
	image.load(DataPath(StaticPath, "mainshp.flx"), 20);
	int next = 0;
	int spacing = 10;
	unsigned char done;
	int unusedCount;
	int top;
	unsigned long start, now, elapsed;
	char moved;
	int bottom;
	ScrollLine *lines[SCROLL_LINES];
	{
		int y = 200;
		ScrollLine **line = lines;
		for (int i = 0; i < SCROLL_LINES; i++, line++) {
			*line = new ScrollLine((void far *) MenuFont.shape, 0, &screen, 0);
			(*line)->moveTo(CENTRE_X, y);
			(*line)->setAlign(ALIGN_CENTRE);
			(*line)->setSpacing(1);
			(*line)->setOwnImage(image.data);
			if (next >= text.count)
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
	Song song(&Music, MusicFlex, 4);
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
				if (next >= text.count) {
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
			for (int n = 0; n < SCROLL_LINES; n++)
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
		if (kbhit()) {
			getch();
			done = 1;
		}
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
				for (int n = 0; n < SCROLL_LINES; n++)
					if (!lines[n]->aboveTop())
						lines[n]->moveBy(0, -1);
				elapsed -= stepTicks;
			}
			if (moved) {
				screen.paint(0, 0);
				start = now - elapsed;
			}
			if (kbhit())
				break;
		}
		if (SpeechEnabled) {
			SpeechCard.resetPlayback();
			SpeechCache.playEntry(DataPath(StaticPath, "u7speech.spc"), 30, 0);
			SpeechCard.play(&SpeechCache);
		}
		while (!song.finished() || !SpeechFinished)
			if (kbhit()) {
				getch();
				break;
			}
	}
	song.fadeOut(120);
	if (SpeechEnabled)
		SpeechCard.stop();
	palette.fadeToColor(&Red, 0, 0, 255, FadeTicks);
	palette.fadeToColor(&Black, 0, 0, 255, FadeTicks);
	FillView(&ScreenView, 0);
	for (int j = 0; j < SCROLL_LINES; j++)
		delete lines[j];
}
