/* Black Gate INTRO.EXE, intro.c: the introduction. Lord British presents, the title and the
 * butterfly, static, the Guardian on the monitor, the Avatar's desk and the moongate.
 *
 * The launcher runs it with its password first. It exits with 2, when it ends or on Escape, and
 * the launcher goes on to the main menu; 1 on a fatal error.
 */

#include "u7port.h"
#include "plat.h"
#include "arena.h"
#include "dosio.h"
#include "lowlevel.h"
#include "vidmode.h"
#include "screen.h"
#include "memapi.h"
#include "oops.h"
#include "chkfile.h"
#include "view.h"
#include "systimer.h"
#include "u7event.h"
#include "preload.h"
#include "cflxbuf.h"
#include "specard.h"
#include "midiplay.h"
#include "../ultima7/programs.h"
#include "../shared/errors.h"
#include "../shared/fadepal.h"
#include "../shared/music.h"
#include "../shared/keys.h"
#include "../shared/textfile.h"
#include "controls.h"
#include "textspr.h"
#include "synctrk.h"
#include "cfilcach.h"
#include "pacing.h"
#include "intro.h"
#include <new>

/* U7's sound configuration and audio options, read with U7's code. */
struct AudioOptions {
	uint8_t music, speech, effects;
};

struct SoundConfig {
	int8_t device;            /* 's' Sound Blaster, 'a' Adlib, 'r' Roland, 'p' none */
	int16_t musicPort;
	int16_t speechPort;
	int16_t irq;
	int16_t dma;
	int8_t speechEnabled;
	SoundConfig();
	void reset();
	void readConfig(char *name);
	void close();
	int16_t isRoland();
	int16_t isAdlib();
	int16_t isSoundBlaster();
	int16_t hasMusic();
};

namespace Intro {

using Shared::FadingPalette;
using Shared::RgbColor;
using Shared::Song;
using Shared::SoundEffect;
using Shared::KeyHit;
using Shared::GetKey;

#define KEY_ESCAPE      27
#define KEY_SKIP        19      /* Ctrl-S cuts the scene short */

#define EXIT_DONE       2

#define SPEECH_BUFFER   2596
#define SPEECH_RATE     16000
#define FLIGHT_POINTS   96      /* the butterfly's path over the title */
#define FADE_TICKS      60
#define FLIGHT_START    990     /* ticks into the song the butterfly takes off */
#define STATIC_COLOR    7

/* INTROPAL.DAT entries. */
#define INTROPAL_DESK           1
#define INTROPAL_MONITOR        2
#define INTROPAL_PRESENTS       3
#define INTROPAL_TITLE          4
#define INTROPAL_MOONGATE       5

/* ENDSHAPE.FLX entries. */
#define ENDSHAPE_LEFT_CURTAIN           0
#define ENDSHAPE_RIGHT_CURTAIN          1
#define ENDSHAPE_STONES_TOP_LEFT        2
#define ENDSHAPE_STONES_RIGHT           3
#define ENDSHAPE_STONES_BOTTOM_LEFT     4
#define ENDSHAPE_STONES_BOTTOM_RIGHT    5
#define ENDSHAPE_MAP                    6
#define ENDSHAPE_MONITOR_TOP_RIGHT      7
#define ENDSHAPE_MONITOR_BOTTOM_RIGHT   8
#define ENDSHAPE_MONITOR_TOP_LEFT       9
#define ENDSHAPE_MONITOR_BOTTOM_LEFT    10
#define ENDSHAPE_DESK                   11
#define ENDSHAPE_FIST                   12
#define ENDSHAPE_TITLE          13
#define ENDSHAPE_BUTTERFLY      14
#define ENDSHAPE_PRESENTS       17
#define ENDSHAPE_TITLE_BACK     18
#define ENDSHAPE_MONITOR        19
#define ENDSHAPE_DOT            20
#define ENDSHAPE_AMISS          21      /* "Something is obviously amiss." */
#define ENDSHAPE_LONG_TIME      22      /* "It has been a long time..." */
#define ENDSHAPE_BECKONS        23      /* "The mystical Orb beckons you..." */
#define ENDSHAPE_GATEWAYS       24      /* "It has opened gateways..." */
#define ENDSHAPE_BEHIND_HOUSE   25      /* "Behind your house lies the circle of stones." */
#define ENDSHAPE_WHY_MOONGATE   26      /* "Why is a Moongate already there?" */
#define ENDSHAPE_ONE_PATH       28      /* "You have but one path to the answer..." */
#define ENDSHAPE_MONITOR_TITLE  29      /* the title on the monitor */
#define ENDSHAPE_FIRST_SWIRL    33
#define ENDSHAPE_SECOND_SWIRL   34
#define ENDSHAPE_FACE           35

/* MAINSHP.FLX entries. */
#define MAINSHP_FONT            3
#define MAINSHP_SPEECH_TEXT     13      /* the Guardian's words as subtitles */
#define MAINSHP_SYNC_TRACK      15      /* cues for his face */

/* Sync track cue values; from CUE_HEAD up a cue picks the Guardian's head frame. */
#define CUE_RESTART         0
#define CUE_MOOD_2          1
#define CUE_MOOD_1          2
#define CUE_MOOD_0          3
#define CUE_MOUTH_0         4
#define CUE_MOUTH_1         5
#define CUE_MOUTH_2         6
#define CUE_MOUTH_CLOSED    7
#define CUE_HEAD            8

/* A stopwatch that can be stopped and restarted; laid out as U7's, whose functions read it. */
struct Stopwatch {
	int8_t running;
	uint32_t start, total;
	Stopwatch() { running = 0; total = start = 0; }
	void begin() { start = TickCount; running = 1; }
	uint32_t elapsed() { return Stopwatch_getElapsed((::Stopwatch *) this); }
	void stop() { Stopwatch_stop((::Stopwatch *) this); }
};

/* Thrown to end the introduction: the scenes unwind, freeing what they hold, before the exit. */
struct Ending {
	int16_t code;
};

char *const StaticPath = ".\\static\\";
char *const GamedatPath = ".\\gamedat\\";
const int16_t SpeedDivisor = 32;

static FadingPalette OriginalPalette;

/* The butterfly's flight, one point every few steps. */
static const int16_t flightX[FLIGHT_POINTS] = {
	6, 18, 30, 41, 52, 62, 70, 78, 86, 95, 104, 113, 122, 132, 139, 146, 151, 155, 157, 158, 157, 155, 151, 146,
	139, 132, 124, 116, 108, 102, 96, 93, 93, 93, 95, 99, 109, 111, 118, 125, 132, 140, 148, 157, 164, 171, 178,
	184, 190, 196, 203, 211, 219, 228, 237, 246, 254, 259, 262, 264, 265, 265, 263, 260, 256, 251, 245, 239, 232,
	226, 219, 212, 208, 206, 206, 209, 212, 216, 220, 224, 227, 234, 231, 232, 233, 233, 233, 233, 234, 236, 239,
	243, 247, 250, 258, 265
};
static const int16_t flightY[FLIGHT_POINTS] = {
	155, 153, 151, 150, 149, 148, 148, 148, 148, 149, 150, 150, 150, 149, 147, 142, 137, 131, 125, 118, 110, 103,
	98, 94, 92, 91, 91, 91, 92, 95, 99, 104, 110, 117, 123, 127, 131, 134, 135, 135, 135, 135, 135, 134, 132, 129,
	127, 123, 119, 115, 112, 109, 104, 102, 101, 102, 109, 109, 114, 119, 125, 131, 138, 144, 149, 152, 156, 158,
	159, 159, 158, 155, 150, 144, 137, 130, 124, 118, 112, 105, 99, 93, 86, 80, 73, 66, 59, 53, 47, 42, 38, 35, 32,
	29, 26, 25
};

static FileSpeechCache Speech;
static Shared::MusicSystem Music;
static FatalHandler PreviousFatalHook = 0;
static RgbColor Black;
static RgbColor Red;
static char MusicFlexName[80];
static char *const MusicFlex = MusicFlexName;
static uint8_t SpeechEnabled = 0;
static SoundConfig SoundSetup;
static AudioOptions AudioSettings;
static uint8_t DevicesClosed = 0;

/* The sound effects, played on the music card. */
static SoundEffect StaticSound(4, 100, 41, 64, 60, 0, 0);
static SoundEffect PowerOffSound(0, 99, 48, 64, 90, 0, 0);
static SoundEffect AppearSound(4, 62, 48, 127, 60, 0, 0);
static SoundEffect PunchSound(0, 105, 48, 127, 20, 0, 0);
static SoundEffect MoongateSound(4, 106, 48, 64, 120, 0, 0);
static SoundEffect EnterSound(0, 107, 41, 127, 5, 0, 0);
static SoundEffect VanishSound(4, 109, 48, 96, 5, 0, 0);
static SoundEffect DotSound(4, 111, 48, 64, 15, 0, 0);

static void HandleKey(void);
static void Quit(void);

static char path[80];

char *DataPath(char *dir, char *name)
{
	strcpy(path, dir);
	strcat(path, name);
	return path;
}

/* Waits up to ticks for a key. Escape ends the introduction; the skip key is left for the caller,
 * and the result says it came. */
static uint8_t WaitForKey(uint16_t ticks)
{
	Timer timer(ticks);
	int16_t key;

	Timer_restart(&timer);
	while (!Timer_hasFinished(&timer) && !KeyHit())
		plat_yield();
	if (KeyHit()) {
		if ((key = GetKey()) == KEY_ESCAPE) {
			Quit();
		} else if (key == KEY_SKIP) {
			UnreadKey(key);
			return 1;
		}
	}
	return 0;
}

/* Waits ticks, keys or not. */
static void Delay(uint16_t ticks)
{
	Timer timer(ticks);

	Timer_restart(&timer);
	while (!Timer_hasFinished(&timer))
		plat_yield();
}

/* Waits until a stopwatch reaches a tick count. */
static void WaitUntil(Stopwatch *clock, uint32_t ticks)
{
	while (clock->elapsed() < ticks)
		plat_yield();
}

/* Lord British presents: the card fades in from black, stays ticks, and fades out. */
static void ShowPresents(int16_t ticks, int16_t fadeInDelay, int16_t fadeOutDelay)
{
	FadingPalette palette;
	palette.fill(&Black, 0, PALETTE_COLORS - 1);
	palette.apply();
	palette.load(DataPath(StaticPath, "intropal.dat"), INTROPAL_PRESENTS);
	Screen screen;
	screen.drawShape(ENDSHAPE_PRESENTS, DataPath(StaticPath, "endshape.flx"), 0, 0, 0);
	screen.paint(-1, 0);
	palette.fadeFromColor(&Black, fadeInDelay, 0, PALETTE_COLORS - 1, FADE_TICKS);
	WaitForKey(ticks);
	palette.fadeToColor(&Black, fadeOutDelay, 0, PALETTE_COLORS - 1, FADE_TICKS);
	if (KeyHit())
		HandleKey();
}

/* The title, and the butterfly flying across it to the music; the skip key cuts it short. */
static void ShowTitle(int16_t ticks, int16_t unusedTicks, int16_t fadeInDelay, int16_t speed, int16_t unusedFlag)
{
	FadingPalette palette;
	palette.fill(&Black, 0, PALETTE_COLORS - 1);
	palette.apply();
	palette.load(DataPath(StaticPath, "intropal.dat"), INTROPAL_TITLE);
	/* steps between two points of the flight, for this machine's speed */
	speed = speed * 33 / SpeedDivisor;
	if (speed < 1)
		speed = 1;
	Screen screen;
	MovingSprite title(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_TITLE, 0, &screen, 1);
	BouncingSprite butterfly(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_BUTTERFLY, 0, &screen, 1);
	screen.drawShape(ENDSHAPE_TITLE_BACK, DataPath(StaticPath, "endshape.flx"), 0, 0, 0);
	Song song(&Music, MusicFlex, 0);
	song.play();
	Stopwatch clock;
	clock.begin();
	butterfly.x = flightX[0];
	butterfly.y = flightY[0];
	title.x = 160;
	title.y = 50;
	butterfly.visible = 0;
	butterfly.drawn = 0;
	screen.paint(-1, 0);
	if (WaitForKey(ticks)) {
		HandleKey();
	} else {
		palette.fadeFromColor(&Black, fadeInDelay, 0, PALETTE_COLORS - 1, FADE_TICKS);
		screen.paint(-1, 0);
		butterfly.visible = 1;
		while (clock.elapsed() < FLIGHT_START && !KeyHit())
			plat_yield();
		int16_t point = 0;
		uint32_t frames = FLIGHT_START;
		int16_t lastAngle = 0;
		int16_t angle = 0;
		while (point++ < FLIGHT_POINTS - 1 && !KeyHit()) {
			lastAngle = angle;
			butterfly.setPath(flightX[point - 1], flightY[point - 1], flightX[point], flightY[point], 256, 256,
				lastAngle, angle, speed);
			while (butterfly.step != butterfly.steps && !KeyHit()) {
				if (random(5) < 4)
					butterfly.cycleFrames(0, 3);
				butterfly.advance();
				screen.paint(-1, 0);
				frames++;
				WaitUntil(&clock, frames);
			}
		}
		if (KeyHit())
			HandleKey();
		/* the butterfly settles, its wings opening and closing */
		static const struct {
			int16_t frame, ticks;
		} settle[] = { { 3, 10 }, { 4, 25 }, { 3, 15 }, { 4, 25 }, { 3, 25 }, { 2, 25 }, { 1, 25 }, { 0, 20 } };
		for (uint16_t i = 0; i < sizeof settle / sizeof settle[0]; i++) {
			butterfly.frame = settle[i].frame;
			screen.paint(-1, 0);
			Delay(settle[i].ticks);
			if (KeyHit())
				HandleKey();
		}
		while (!song.finished() && !KeyHit())
			plat_yield();
		if (KeyHit())
			HandleKey();
	}
	{
		FadingPalette flash;

		flash.load(DataPath(StaticPath, "intropal.dat"), INTROPAL_MONITOR);
		song.fadeOut(0);
		palette.fill(&Black, 0, PALETTE_COLORS - 1);
		palette.apply();
		StaticSound.play(1);
		DrawStatic(&ScreenView, 1, STATIC_COLOR);
		flash.apply();
	}
}

/* The title gives way to static, in three bursts with a pause between each; a still picture shows
 * through the pauses. */
static void ShowStatic(int16_t firstFrames, int16_t secondFrames, int16_t thirdFrames, int16_t firstPause,
	int16_t secondPause)
{
	FadingPalette palette;
	palette.load(DataPath(StaticPath, "intropal.dat"), INTROPAL_MONITOR);
	palette.apply();
	Screen screen;
	screen.drawShape(ENDSHAPE_MONITOR, DataPath(StaticPath, "endshape.flx"), 0, 0, 0);
	DrawStatic(&ScreenView, firstFrames, STATIC_COLOR);
	StaticSound.stop();
	screen.paint(-1, 0);
	if (WaitForKey(firstPause)) {
		HandleKey();
	} else {
		StaticSound.play(-1);
		DrawStatic(&ScreenView, secondFrames, STATIC_COLOR);
		StaticSound.stop();
		screen.paint(-1, 0);
		if (WaitForKey(secondPause)) {
			HandleKey();
		} else {
			StaticSound.play(-1);
			DrawStatic(&ScreenView, thirdFrames, STATIC_COLOR);
			StaticSound.stop();
		}
	}
	screen.paint(-1, 0);
}

/*
 * The Guardian appears on the monitor and speaks. The monitor's colors cycle throughout. Two red
 * swirls come and go, the face forms, and the Guardian talks: his mouth, mood and head follow
 * the cues of a sync track, timed by the speech or, without it, by the subtitles. The face then
 * fades, static takes the monitor, and it shrinks to a dot. A key cuts any stage short.
 */
static void ShowGuardian(int16_t cycles, int16_t delay, int16_t firstInRate, int16_t firstOutRate,
	int16_t secondInRate, int16_t secondOutRate, int16_t unusedRate, char *trackFile, int16_t trackEntry,
	int16_t staticFrames, int16_t pause, int16_t endPause, int16_t lineTicks)
{
	char *trackPath = new char[strlen(trackFile) + 1];
	strcpy(trackPath, trackFile);
	FadingPalette palette;
	palette.load(DataPath(StaticPath, "intropal.dat"), INTROPAL_MONITOR);
	palette.apply();
	Song song(&Music, MusicFlex, 2);
	song.play();
	Screen screen;
	screen.drawShape(ENDSHAPE_MONITOR, DataPath(StaticPath, "endshape.flx"), 0, 0, 0);
	int16_t cycle;
	for (cycle = 0; cycle < cycles; cycle++) {
		if (KeyHit()) {
			HandleKey();
			break;
		}
		palette.rotate(16, 93, 1);
		Delay(delay);
	}
	int16_t wait;
	{
		Sprite firstSwirl(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_FIRST_SWIRL, 1, &screen, 1);
		Sprite secondSwirl(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_SECOND_SWIRL, 1, &screen, 1);
		int16_t step;

		firstSwirl.x = 118;
		firstSwirl.y = 48;
		secondSwirl.x = 147;
		secondSwirl.y = 62;
		AppearSound.play(-1);
		firstSwirl.frame = 9;
		step = 0;
		while (!KeyHit() && firstSwirl.frame > 1) {
			screen.paint(-1, 0);
			if (step++ % firstInRate == 0)
				firstSwirl.frame--;
			palette.rotate(16, 93, 1);
		}
		if (!KeyHit()) {
			AppearSound.stop();
			VanishSound.play(-1);
			step = 0;
			while (!KeyHit() && firstSwirl.frame <= 9) {
				screen.paint(-1, 0);
				if (step++ % firstOutRate == 0)
					firstSwirl.frame++;
				palette.rotate(16, 93, 1);
			}
			if (!KeyHit()) {
				firstSwirl.visible = 0;
				firstSwirl.drawn = 0;
				screen.paint(-1, 0);
				VanishSound.stop();
				step = 0;
				while (step++ < pause && !KeyHit()) {
					palette.rotate(16, 93, 1);
					Delay(delay);
				}
				if (!KeyHit()) {
					AppearSound.play(-1);
					secondSwirl.frame = 1;
					step = 0;
					while (!KeyHit() && secondSwirl.frame < 8) {
						screen.paint(-1, 0);
						if (step++ % secondInRate == 0)
							secondSwirl.frame++;
						palette.rotate(16, 93, 1);
					}
					if (!KeyHit()) {
						AppearSound.stop();
						VanishSound.play(-1);
						step = 0;
						while (!KeyHit() && secondSwirl.frame >= 1) {
							screen.paint(-1, 0);
							if (step++ % secondOutRate == 0)
								secondSwirl.frame--;
							palette.rotate(16, 93, 1);
						}
						VanishSound.stop();
						secondSwirl.visible = 0;
						secondSwirl.drawn = 0;
						screen.paint(-1, 0);
					}
				}
			}
		}
		if (KeyHit()) {
			HandleKey();
			secondSwirl.visible = 0;
			secondSwirl.drawn = 0;
			screen.paint(-1, 0);
			goto done;
		}
	}
	wait = 0;
	while (wait++ < pause && !KeyHit()) {
		palette.rotate(16, 93, 1);
		Delay(delay);
	}
	if (KeyHit())
		HandleKey();
	{
		Sprite face(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_FACE, 1, &screen, 1);
		int16_t step;

		face.x = 160;
		face.y = 100;
		AppearSound.play(-1);
		step = 0;
		while (!KeyHit() && face.frameCount - 1 > face.frame) {
			if (step++ % secondInRate == 0)
				face.frame++;
			screen.paint(-1, 0);
			palette.rotate(16, 93, 1);
		}
		AppearSound.stop();
	}
	if (KeyHit()) {
		HandleKey();
		goto done;
	}
	{
		SyncTrack track(trackPath, trackEntry);
		Guardian guardian(&screen);
		int16_t i;

		guardian.moveTo(160, 100);
		guardian.frame = 1;
		Shared::TextFile lines(DataPath(StaticPath, "mainshp.flx"), MAINSHP_SPEECH_TEXT);
		TextSprite subtitle(DataPath(StaticPath, "mainshp.flx"), MAINSHP_FONT, 0, &screen, 1);
		subtitle.moveTo(159, 180);
		subtitle.visible = 0;
		subtitle.drawn = 0;
		subtitle.spacing = -2;
		int16_t line = 0;
		SyncCue *cue;
		subtitle.setText(lines.getLine(line));
		subtitle.align = ALIGN_CENTRE;
		/* the face settles before he speaks */
		static const struct {
			int8_t mood, mouth;
			int16_t cycles;
		} settle[] = { { -1, -1, 20 }, { 1, 1, 5 }, { 1, 0, 9 }, { -1, 3, 10 }, { 1, 0, 15 } };
		for (uint16_t k = 0; k < sizeof settle / sizeof settle[0]; k++) {
			if (settle[k].mood >= 0)
				guardian.setMood(settle[k].mood);
			if (settle[k].mouth >= 0) {
				guardian.setMouthShape(settle[k].mouth);
				screen.paint(-1, 0);
			}
			i = 0;
			while (i++ < settle[k].cycles) {
				palette.rotate(93, 16, 1);
				Delay(delay);
			}
		}
		screen.paint(-1, 0);
		uint8_t done = 0;
		if (SpeechEnabled) {
			SpeechCard.resetPlayback();
			Speech.play(DataPath(StaticPath, "introsnd.dat"), 5);
			SpeechCard.play(&Speech);
		}
		if (!SpeechEnabled)
			subtitle.visible = 1;
		Stopwatch clock;
		Stopwatch lineClock;
		clock.begin();
		lineClock.begin();
		cue = track.next((uint16_t) clock.elapsed());
		while ((!SpeechFinished || !done) && !(KeyHit() && SpeechEnabled)) {
			if (!SpeechEnabled && (KeyHit() || lineClock.elapsed() > (uint32_t) lineTicks)) {
				if (KeyHit())
					GetKey();
				if (++line >= lines.count)
					break;
				subtitle.setText(lines.getLine(line));
				lineClock.stop();
				lineClock.total = 0;
				lineClock.begin();
			}
			if (BlockConsumed)
				Speech.queueNextBlock();
			if (clock.elapsed() >= cue->time && !done) {
				switch (cue->value) {
				case CUE_RESTART:
					if (!SpeechEnabled) {
						clock.stop();
						clock.total = 0;
						track.current = 0;
						clock.begin();
					} else {
						done = 1;
					}
					break;
				case CUE_MOOD_2:
					guardian.setMood(2);
					break;
				case CUE_MOOD_1:
					guardian.setMood(1);
					break;
				case CUE_MOOD_0:
					guardian.setMood(0);
					break;
				case CUE_MOUTH_0:
					guardian.setMouthShape(0);
					break;
				case CUE_MOUTH_1:
					guardian.setMouthShape(1);
					break;
				case CUE_MOUTH_2:
					guardian.setMouthShape(2);
					break;
				case CUE_MOUTH_CLOSED:
					guardian.setMouthShape(3);
					break;
				default:
					guardian.frame = cue->value - CUE_HEAD;
				}
				screen.paint(-1, 0);
				cue = track.next((uint16_t) clock.elapsed());
			}
			palette.rotate(93, 16, 1);
		}
		if (subtitle.visible) {
			subtitle.restoreUnder();
			subtitle.visible = 0;
			subtitle.drawn = 0;
			screen.paint(-1, 0);
		}
	}
	if (KeyHit()) {
		HandleKey();
		goto done;
	}
	{
		Sprite face(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_FACE, 1, &screen, 1);
		int16_t step;

		face.x = 160;
		face.y = 100;
		VanishSound.play(-1);
		face.frame = face.frameCount - 1;
		step = 0;
		while (!KeyHit() && face.frame > 0) {
			if (step++ % secondInRate == 0)
				face.frame--;
			screen.paint(-1, 0);
			palette.rotate(16, 93, 1);
		}
		VanishSound.stop();
	}
	if (KeyHit()) {
		HandleKey();
		goto done;
	}
	screen.paint(-1, 0);
	wait = 0;
	while (wait++ < endPause && !KeyHit()) {
		palette.rotate(16, 93, 1);
		Delay(delay);
	}
	if (KeyHit())
		HandleKey();
	song.fadeOut(0);
	StaticSound.play(-1);
	DrawStatic(&ScreenView, staticFrames, STATIC_COLOR);
	{
		Sprite dot(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_DOT, 0, &screen, 1);
		int16_t step;

		FillView((View *) &screen, 0);
		StaticSound.stop();
		PowerOffSound.play(1);
		DotSound.play(2);
		for (step = 0; step < 100; step++) {
			dot.x = random(3) + 159;
			dot.y = random(3) + 99;
			screen.paint(-1, 0);
		}
		dot.visible = 0;
		dot.drawn = 0;
	}
done:
	AppearSound.stop();
	VanishSound.stop();
	song.fadeOut(0);
	if (SpeechEnabled)
		SpeechCard.stop();
	delete[] trackPath;
}

/*
 * The Avatar's desk. The view pulls back from the dot on the monitor to the whole monitor, a fist
 * strikes it three times, and the room pans across the cloth map and down to the Orb of the Moons,
 * a caption at each stage.
 */
static void ShowDesk(int16_t ticks, int16_t captionTicks, int16_t panTicks, int16_t captionDelay, int16_t delay)
{
	FadingPalette palette;
	palette.fill(&Black, 0, PALETTE_COLORS - 1);
	palette.load(DataPath(StaticPath, "intropal.dat"), INTROPAL_DESK);
	Screen screen;
	screen.paintTime = PAINT_TIME_DESK_SCALED;
	MovingSprite dot(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_DOT, 0, &screen, 0);
	dot.x = 160;
	dot.y = 100;
	screen.paint(-1, 0);
	palette.apply();
	MovingSprite map(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_MAP, 0, &screen, 0);
	MovingSprite topRight(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_MONITOR_TOP_RIGHT, 0, &screen, 0);
	MovingSprite bottomRight(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_MONITOR_BOTTOM_RIGHT, 0, &screen, 0);
	MovingSprite title(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_MONITOR_TITLE, 0, &screen, 0);
	Song song(&Music, MusicFlex, 1);
	song.play();
	Stopwatch captionClock;
	title.x = 171;
	title.y = 78;
	title.visible = 0;
	title.drawn = 0;
	{
		MovingSprite topLeft(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_MONITOR_TOP_LEFT, 0, &screen, 0);
		MovingSprite bottomLeft(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_MONITOR_BOTTOM_LEFT, 0, &screen,
			0);
		int16_t x, y;

		title.attach(&screen);
		dot.attach(&screen);
		topLeft.setPath(138, 144, 160, 100, 512, 256, 0, 0, 1320 / SpeedDivisor);
		topRight.setPath(138, 144, 160, 100, 512, 256, 0, 0, 1320 / SpeedDivisor);
		bottomLeft.setPath(138, 144, 160, 100, 512, 256, 0, 0, 1320 / SpeedDivisor);
		bottomRight.setPath(138, 144, 160, 100, 512, 256, 0, 0, 1320 / SpeedDivisor);
		dot.setPath(160, 104, 170, 80, 256, 256, 0, 0, 1320 / SpeedDivisor);
		Stopwatch clock;
		uint32_t frames = 0;
		clock.begin();
		while (!KeyHit() && topLeft.step != topLeft.steps) {
			topLeft.advance();
			topRight.advance();
			bottomLeft.advance();
			bottomRight.advance();
			dot.advance();
			x = dot.x;
			y = dot.y;
			dot.x += random(3) - 1;
			dot.y += random(3) - 1;
			screen.paint(-1, 0);
			dot.x = x;
			dot.y = y;
			frames++;
			WaitUntil(&clock, frames);
		}
		HandleKey();
		topLeft.finish();
		topRight.finish();
		bottomLeft.finish();
		bottomRight.finish();
		dot.finish();
		screen.paint(-1, 0);
		DotSound.stop();
		dot.visible = 0;
		dot.drawn = 0;
		{
			View monitor;
			monitor.clip.x0 = 92;
			monitor.clip.y0 = 26;
			monitor.clip.x1 = 251;
			monitor.clip.y1 = 130;
			monitor.segment = ScreenView.segment;
			monitor.rowTable = ScreenView.rowTable;
			MovingSprite fist(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_FIST, 0, &screen, 0);
			fist.x = 0;
			fist.y = 150;
			fist.scale = 200;
			int16_t blow = 0;
			Stopwatch blowClock;
			uint32_t blowFrames = 0;
			blowClock.begin();
			while (blow++ < 3 && !KeyHit()) {
				/* the fist swings up 33 degrees in one step and falls back in three */
				fist.slideBy(0, 0, 0, 33, 1);
				while (!KeyHit() && fist.step != fist.steps) {
					fist.advance();
					screen.paint(-1, 0);
					blowFrames++;
					WaitUntil(&blowClock, blowFrames);
				}
				if (KeyHit()) {
					HandleKey();
					fist.visible = 0;
					fist.drawn = 0;
					title.visible = 1;
					break;
				}
				if (blow > 1)
					StaticSound.stop();
				PunchSound.play(-1);
				title.visible = 1;
				screen.paint(-1, 0);
				if (blow < 3) {
					title.visible = 0;
					title.drawn = 0;
				}
				StaticSound.play(-1);
				fist.slideBy(0, 0, 0, -33, 3);
				blowClock.total = 0;
				blowFrames = 0;
				blowClock.begin();
				while (!KeyHit() && fist.step != fist.steps) {
					if (blow < 3)
						DrawStatic(&monitor, 1, STATIC_COLOR);
					else
						FillView(&monitor, 0);
					fist.advance();
					screen.paint(-1, 0);
					blowFrames++;
					WaitUntil(&blowClock, blowFrames);
				}
			}
			if (KeyHit())
				HandleKey();
		}
		StaticSound.stop();
		screen.paintTime = PAINT_TIME_DESK;
		screen.paint(-1, 0);
		if (WaitForKey(ticks))
			HandleKey();
		{
			Sprite amiss(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_AMISS, 0, &screen, 0);
			int16_t key;

			amiss.x = 160;
			amiss.y = 150;
			screen.paint(-1, 0);
			captionClock.begin();
			while (captionClock.elapsed() < (uint32_t) captionTicks && !KeyHit())
				plat_yield();
			if (KeyHit()) {
				if ((key = GetKey()) == KEY_ESCAPE) {
					Quit();
				} else {
					UnreadKey(key);
					return;
				}
			}
		}
		Sprite longTime(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_LONG_TIME, 0, &screen, 0);
		longTime.x = 160;
		longTime.y = 150;
		longTime.visible = 0;
		longTime.drawn = 0;
		map.x = 319;
		map.y = -1;
		topLeft.slideBy(-194, 0, 0, 0, 3234 / SpeedDivisor);
		topRight.slideBy(-194, 0, 0, 0, 3234 / SpeedDivisor);
		bottomLeft.slideBy(-194, 0, 0, 0, 3234 / SpeedDivisor);
		bottomRight.slideBy(-194, 0, 0, 0, 3234 / SpeedDivisor);
		map.slideBy(-194, 0, 0, 0, 3234 / SpeedDivisor);
		title.slideBy(-194, 0, 0, 0, 3234 / SpeedDivisor);
		int16_t step = 0;
		Stopwatch panClock;
		panClock.begin();
		/* frames carries on from the pull back, so the pan holds still that long first */
		while (!KeyHit() && topLeft.step != topLeft.steps) {
			if (step++ > captionDelay)
				longTime.visible = 1;
			topLeft.advance();
			topRight.advance();
			bottomLeft.advance();
			bottomRight.advance();
			map.advance();
			title.advance();
			frames++;
			WaitUntil(&panClock, frames);
			screen.paint(-1, 0);
		}
		HandleKey();
		topLeft.finish();
		topRight.finish();
		bottomLeft.finish();
		bottomRight.finish();
		map.finish();
		title.finish();
		screen.paint(-1, 0);
	}
	screen.paint(-1, 0);
	if (WaitForKey(panTicks))
		HandleKey();
	{
		MovingSprite desk(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_DESK, 0, &screen, 0);
		Sprite beckons(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_BECKONS, 0, &screen, 0);
		int16_t cycle;

		beckons.x = 160;
		beckons.y = 10;
		desk.x = 319;
		desk.y = 199;
		topRight.slideBy(0, -52, 0, 0, 1716 / SpeedDivisor);
		bottomRight.slideBy(0, -52, 0, 0, 1716 / SpeedDivisor);
		map.slideBy(0, -52, 0, 0, 1716 / SpeedDivisor);
		desk.slideBy(0, -52, 0, 0, 1716 / SpeedDivisor);
		screen.paintTime = PAINT_TIME_DESK_DOWN;
		title.slideBy(0, -52, 0, 0, 1716 / SpeedDivisor);
		cycle = 0;
		uint32_t frames = 0;
		Stopwatch clock;
		clock.begin();
		while (topRight.step != topRight.steps && !KeyHit()) {
			topRight.advance();
			bottomRight.advance();
			map.advance();
			desk.advance();
			title.advance();
			if (cycle++ % 2) {
				palette.rotate(240, 244, 0);
				palette.rotate(245, 249, 0);
				palette.rotate(250, 254, 0);
				palette.apply();
			}
			screen.paint(-1, 0);
			frames++;
			WaitUntil(&clock, frames);
		}
		HandleKey();
		topRight.finish();
		bottomRight.finish();
		map.finish();
		desk.finish();
		title.finish();
		screen.paint(-1, 0);
		beckons.visible = 0;
		beckons.drawn = 0;
		{
			Sprite gateways(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_GATEWAYS, 0, &screen, 0);
			int16_t i;

			gateways.x = 160;
			gateways.y = 10;
			screen.paint(-1, 0);
			i = 0;
			while (i++ < 20) {
				palette.rotate(240, 244, 0);
				palette.rotate(245, 249, 0);
				palette.rotate(250, 254, 0);
				palette.apply();
				Delay(delay);
			}
		}
		palette.fadeToColor(&Black, 3, 0, PALETTE_COLORS - 1, FADE_TICKS);
		song.fadeOut(100);
		Delay(50);
	}
}

/* A curtain's share of a paint, from the screen columns it covers. Enlarged, fewer of its own
 * rows and pixels reach the screen. */
static uint32_t CurtainTime(Sprite *curtain, int16_t columns)
{
	uint32_t rows, pixels;

	if (columns <= 0)
		return 0;
	rows = (uint32_t) SCREEN_HEIGHT * 256 / curtain->scale;
	pixels = rows * columns * 256 / curtain->scale;
	return rows * PAINT_TIME_CURTAIN_ROW + pixels * PAINT_TIME_CURTAIN_PIXEL_NS / 1000;
}

/* A paint of the stones and curtains. The left curtain ends at its x, the right one starts at its. */
static uint32_t CurtainPaintTime(Sprite *left, Sprite *right)
{
	int16_t leftColumns = left->x >= SCREEN_WIDTH ? SCREEN_WIDTH : left->x + 1;
	int16_t rightColumns = right->x < 0 ? SCREEN_WIDTH : SCREEN_WIDTH - right->x;

	return PAINT_TIME_STONES + CurtainTime(left, leftColumns) + CurtainTime(right, rightColumns);
}

/* The circle of stones behind the house, and the moongate standing in it. The darkness parts, the
 * captions ask their question, and the view closes in on the moongate. */
static void ShowMoongate(int16_t delay, int16_t glowCycles, int16_t unusedCycles, int16_t captionDelay,
	int16_t endCycles)
{
	FadingPalette palette;
	palette.fill(&Black, 0, PALETTE_COLORS - 1);
	palette.apply();
	palette.load(DataPath(StaticPath, "intropal.dat"), INTROPAL_MOONGATE);
	Screen screen;
	screen.paintTime = PAINT_TIME_STONES;
	MovingSprite topLeft(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_STONES_TOP_LEFT, 0, &screen, 0);
	MovingSprite right(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_STONES_RIGHT, 0, &screen, 0);
	MovingSprite bottomLeft(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_STONES_BOTTOM_LEFT, 0, &screen, 0);
	MovingSprite bottomRight(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_STONES_BOTTOM_RIGHT, 0, &screen, 0);
	MovingSprite leftCurtain(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_LEFT_CURTAIN, 0, &screen, 0);
	MovingSprite rightCurtain(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_RIGHT_CURTAIN, 0, &screen, 0);
	MoongateSound.play(2);
	topLeft.x = 160;
	topLeft.y = 100;
	right.x = 160;
	right.y = 100;
	bottomLeft.x = 160;
	bottomLeft.y = 101;
	bottomRight.x = 160;
	bottomRight.y = 101;
	leftCurtain.x = 210;
	leftCurtain.y = 0;
	rightCurtain.x = 110;
	rightCurtain.y = 0;
	{
		FadingPalette captionPalette;
		captionPalette = palette;
		captionPalette.fill(&Black, 0, 10);
		captionPalette.fill(&Black, 12, PALETTE_COLORS - 1);
		Sprite behindHouse(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_BEHIND_HOUSE, 0, &screen, 0);
		behindHouse.x = 160;
		behindHouse.y = 150;
		screen.paint(-1, 0);
		captionPalette.fadeFromColor(&Black, 1, 11, 11, FADE_TICKS);
		if (WaitForKey(120))
			HandleKey();
		palette.fadeFromColor(&Black, 1, 16, PALETTE_COLORS - 1, FADE_TICKS);
		int16_t cycle = 0;
		while (cycle++ < glowCycles && !KeyHit()) {
			palette.rotate(254, 240, 1);
			Delay(delay);
		}
		if (KeyHit())
			HandleKey();
	}
	{
		Sprite whyMoongate(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_WHY_MOONGATE, 0, &screen, 0);
		Sprite onePath(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_ONE_PATH, 0, &screen, 0);
		int16_t step;

		whyMoongate.visible = 0;
		whyMoongate.drawn = 0;
		onePath.visible = 0;
		onePath.drawn = 0;
		whyMoongate.x = 160;
		whyMoongate.y = 150;
		onePath.x = 160;
		onePath.y = 150;
		screen.paint(-1, 0);
		leftCurtain.slideBy(-240, 0, 160, 0, 2640 / SpeedDivisor);
		rightCurtain.slideBy(240, 0, 160, 0, 2640 / SpeedDivisor);
		step = 0;
		while (!KeyHit() && leftCurtain.step != leftCurtain.steps) {
			if (step++ > captionDelay)
				whyMoongate.visible = 1;
			if (step > 35) {
				whyMoongate.visible = 0;
				whyMoongate.drawn = 0;
			}
			if (step > 40)
				onePath.visible = 1;
			leftCurtain.advance();
			rightCurtain.advance();
			palette.rotate(254, 240, 1);
			screen.paintTime = CurtainPaintTime(&leftCurtain, &rightCurtain);
			screen.paint(-1, 0);
			Delay(0);
		}
		if (KeyHit()) {
			HandleKey();
			leftCurtain.finish();
			rightCurtain.finish();
			screen.paint(-1, 0);
		}
		onePath.visible = 0;
		onePath.drawn = 0;
		screen.paintTime = PAINT_TIME_STONES;
		screen.paint(-1, 0);
		step = 0;
		while (step++ < endCycles) {
			palette.rotate(254, 240, 1);
			Delay(delay);
		}
	}
	screen.paintTime = PAINT_TIME_STONES_ENLARGED;
	topLeft.slideBy(60, 150, 2048, 0, 12);
	right.slideBy(60, 150, 2048, 0, 12);
	bottomLeft.slideBy(60, 150, 2048, 0, 12);
	bottomRight.slideBy(60, 150, 2048, 0, 12);
	while (!KeyHit() && topLeft.step != topLeft.steps) {
		topLeft.advance();
		right.advance();
		bottomLeft.advance();
		bottomRight.advance();
		palette.apply();
		palette.rotate(254, 240, 1);
		screen.paint(-1, 0);
	}
	if (KeyHit())
		HandleKey();
	MoongateSound.stop();
	EnterSound.play(-1);
	FillView(&ScreenView, 0);
	Delay(60);
}

/* Blackens the screen and gives back the far heap; with restorePalette also the palette. */
static void RestoreSystem(char restorePalette)
{
	FillView(&ScreenView, 0);
	if (restorePalette) {
		OriginalPalette.apply();
	} else {
		FadingPalette palette;

		palette.fill(&Black, 0, PALETTE_COLORS - 1);
		palette.apply();
	}
	ResetFarHeap(0);
	CloseFarHeap(0);
	while (KeyHit())
		GetKey();
}

/* Escape ends the introduction; other keys are dropped. */
static void HandleKey(void)
{
	if (KeyHit() && GetKey() == KEY_ESCAPE)
		Quit();
}

/* Stops the music and shuts down the speech card, the speech file and the fast timer. */
static void CloseDevices(void)
{
	if (DevicesClosed)
		return;
	DevicesClosed = 1;
	Music.stop();
	SpeechCard.SoundBlaster::~SoundBlaster();
	Speech.FileSpeechCache::~FileSpeechCache();
	SystemTimer.SysTimer::~SysTimer();
}

/* Ends the introduction; the launcher goes on to the main menu. */
static void Quit(void)
{
	throw Ending{ EXIT_DONE };
}

/* A fatal error puts the screen and palette back before the message. */
static void OnFatalError(void)
{
	CloseDevices();
	RestoreSystem(1);
	if (PreviousFatalHook)
		PreviousFatalHook();
}

/* Routes fatal errors through OnFatalError, and opens the far heap. */
static void InstallFatalHook(void)
{
	PreviousFatalHook = SwapFatalHook(OnFatalError);
	StartFarHeap(0);
}

/* Keeps the palette to restore, blackens the screen and gives the drawing view its buffer. */
static void InitEnvironment(void)
{
	Black.red = Black.blue = Black.green = 0;
	Red.red = 63;
	Red.blue = 0;
	Red.green = 0;
	SetDisplayMode(0);
	OriginalPalette.capture();
	FadingPalette palette;
	palette.fill(&Black, 0, PALETTE_COLORS - 1);
	palette.apply();
	Viewport.clip.x0 = 0;
	Viewport.clip.y0 = 0;
	Viewport.clip.x1 = SCREEN_WIDTH - 1;
	Viewport.clip.y1 = SCREEN_HEIGHT - 1;
	if (!AllocateDrawBuffer(&Viewport, 0, 0))
		ReportOutOfFarMemory();
}

/* The sound card setup from configuration, then the player's audio choices from preferences.
 * Music plays only on the MT-32 here: any music card becomes one while MIDI output is there,
 * as in the game. */
static void ConfigureSound(char *configuration, char *preferences, int16_t *irq, int16_t *port,
	uint8_t *device, int16_t *dma)
{
	char *preferencesPath;

	*irq = 7;
	*port = 0x220;
	*dma = 1;
	*device = 0;
	preferencesPath = new char[strlen(preferences) + 1];
	strcpy(preferencesPath, preferences);
	SoundSetup.readConfig(configuration);
	if ((uint8_t) SoundSetup.isAdlib() || (uint8_t) SoundSetup.isSoundBlaster())
		*device = MUSIC_DEVICE_ADLIB;
	if ((uint8_t) SoundSetup.isRoland())
		*device = MUSIC_DEVICE_MT32;
	*irq = SoundSetup.irq;
	*port = SoundSetup.speechPort;
	*dma = SoundSetup.dma;
	if (SoundSetup.speechEnabled)
		SpeechEnabled = 1;
	else
		SpeechEnabled = 0;
	if ((uint8_t) SoundSetup.hasMusic()) {
		Shared::EnableMusic();
		Shared::EnableSfx();
	}
	if (ReadAudioOptions(preferencesPath, &AudioSettings)) {
		if (AudioSettings.music == AUDIO_OFF)
			Shared::DisableMusic();
		if (AudioSettings.effects == AUDIO_OFF)
			Shared::DisableSfx();
		SpeechEnabled = 0;
		if (AudioSettings.speech == AUDIO_ON && SoundSetup.speechEnabled)
			SpeechEnabled = 1;
	}
	delete[] preferencesPath;
}

/* The music card named on the command line or configured, made the MT-32 if there is one.
 * Without one, songs and effects are off, as with no card configured. */
static uint8_t ChooseMusicDevice(uint8_t device)
{
	if (device != 0 && plat_midi_available()) {
		strcpy(MusicFlexName, DataPath(StaticPath, "intrordm.dat"));
		return MUSIC_DEVICE_MT32;
	}
	Shared::DisableMusic();
	Shared::DisableSfx();
	return 0;
}

/* A second argument starting with A or R picks the Adlib or Roland music over the configured card.
 * The six scenes play in turn. */
static void Main(int16_t argc, char **argv)
{
	uint8_t device;
	int16_t irq, port, dma;

	Shared::SetKeyHandler(HandleKey);
	if (argc < 2)
		Shared::FatalError("Type ULTIMA7 to play Ultima VII\n");
	if (stricmp(argv[1], "EREIAMJH"))
		Shared::FatalError("Type ULTIMA7 to play Ultima VII\n");
	device = 0;
	ConfigureSound("u7.cfg", DataPath(GamedatPath, "options.cfg"), &irq, &port, &device, &dma);
	if (argc >= 3) {
		switch (argv[2][0]) {
		case 'A':
		case 'a':
			device = MUSIC_DEVICE_ADLIB;
			break;
		case 'R':
		case 'r':
			device = MUSIC_DEVICE_MT32;
			break;
		}
	}
	device = ChooseMusicDevice(device);
	InstallFatalHook();
	SystemTimer.install();
	InitEnvironment();
	{
		char driver[80];

		strcpy(driver, DataPath(StaticPath, "u7strax.drv"));
		Music.start(device, driver, DataPath(StaticPath, "u7intro.tim"));
	}
	if (SpeechEnabled)
		SpeechCard.init(SPEECH_BUFFER, SPEECH_RATE, port, irq, dma);
	ShowPresents(60, 0, 0);
	ShowTitle(230, 380, 0, 3, 1);
	ShowStatic(660 / SpeedDivisor, 330 / SpeedDivisor, 165 / SpeedDivisor, 6, 12);
	ShowGuardian(120, 3, 2, 5, 5, 5, 2, DataPath(StaticPath, "mainshp.flx"), MAINSHP_SYNC_TRACK, 20, 50, 40,
		305);
	ShowDesk(60, 200, 100, 40, 10);
	ShowMoongate(5, 15, 40, 8, 40);
	Quit();
}

}

extern "C" int16_t IntroMain(int16_t argc, char **argv)
{
	try {
		Intro::Main(argc, argv);
	} catch (Intro::Ending &ending) {
		Intro::CloseDevices();
		Intro::RestoreSystem(0);
		return ending.code;
	}
	return EXIT_DONE;
}

extern "C" void ResetIntroIntroGlobals(void)
{
	memset((void *) &Intro::OriginalPalette, 0, sizeof Intro::OriginalPalette);
	memset((void *) &Intro::Speech, 0, sizeof Intro::Speech);
	Intro::PreviousFatalHook = 0;
	memset(&Intro::Black, 0, sizeof Intro::Black);
	memset(&Intro::Red, 0, sizeof Intro::Red);
	memset(Intro::MusicFlexName, 0, sizeof Intro::MusicFlexName);
	Intro::SpeechEnabled = 0;
	memset((void *) &Intro::SoundSetup, 0, sizeof Intro::SoundSetup);
	memset(&Intro::AudioSettings, 0, sizeof Intro::AudioSettings);
	Intro::DevicesClosed = 0;
	memset((void *) &Intro::StaticSound, 0, sizeof Intro::StaticSound);
	memset((void *) &Intro::PowerOffSound, 0, sizeof Intro::PowerOffSound);
	memset((void *) &Intro::AppearSound, 0, sizeof Intro::AppearSound);
	memset((void *) &Intro::PunchSound, 0, sizeof Intro::PunchSound);
	memset((void *) &Intro::MoongateSound, 0, sizeof Intro::MoongateSound);
	memset((void *) &Intro::EnterSound, 0, sizeof Intro::EnterSound);
	memset((void *) &Intro::VanishSound, 0, sizeof Intro::VanishSound);
	memset((void *) &Intro::DotSound, 0, sizeof Intro::DotSound);
	memset(Intro::path, 0, sizeof Intro::path);
}

extern "C" void ConstructIntroIntroGlobals(void)
{
	new (&Intro::OriginalPalette) Shared::FadingPalette();
	new (&Intro::Speech) Intro::FileSpeechCache();
	new (&Intro::Music) Shared::MusicSystem();
	new (&Intro::SoundSetup) SoundConfig();
	new (&Intro::StaticSound) Shared::SoundEffect(4, 100, 41, 64, 60, 0, 0);
	new (&Intro::PowerOffSound) Shared::SoundEffect(0, 99, 48, 64, 90, 0, 0);
	new (&Intro::AppearSound) Shared::SoundEffect(4, 62, 48, 127, 60, 0, 0);
	new (&Intro::PunchSound) Shared::SoundEffect(0, 105, 48, 127, 20, 0, 0);
	new (&Intro::MoongateSound) Shared::SoundEffect(4, 106, 48, 64, 120, 0, 0);
	new (&Intro::EnterSound) Shared::SoundEffect(0, 107, 41, 127, 5, 0, 0);
	new (&Intro::VanishSound) Shared::SoundEffect(4, 109, 48, 96, 5, 0, 0);
	new (&Intro::DotSound) Shared::SoundEffect(4, 111, 48, 64, 15, 0, 0);
}
