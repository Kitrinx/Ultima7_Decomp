/* Black Gate INTRO.EXE, one module of resident segment 1 (file offsets 0x005411 to 0x00922f, 15902 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include <conio.h>
#include <iostream.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>
#include "lowlevel.h"
#include "vidmode.h"
#include "screen.h"
#include "memapi.h"
#include "errors.h"
#include "oops.h"
#include "vstring.h"
#include "chkfile.h"
#include "view.h"
#include "systimer.h"
#include "colbuf.h"
#include "fadepal.h"
#include "controls.h"
#include "textspr.h"
#include "music.h"
#include "cflxbuf.h"
#include "specard.h"
#include "synctrk.h"
#include "textfile.h"
#include "intro.h"

#define AUDIO_OFF   1
#define AUDIO_ON    2

#define KEY_ESCAPE      27
#define KEY_SKIP        19      /* Ctrl-S cuts the scene short */

/* The launcher's exit codes. */
#define EXIT_MENU       0
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

/* The BIOS video mode to go back to; 3 is color text. */
struct VideoMode {
	unsigned char mode;
	VideoMode() : mode(3) {}
};

/* Audio settings: each is 0 unset, AUDIO_OFF or AUDIO_ON. */
struct AudioOptions {
	unsigned char music, speech, effects;
	AudioOptions() { music = speech = effects = 0; }
};

/* The sound setup: a music device with its port, and a digital device. */
struct SoundConfig {
	char device;            /* 's' Sound Blaster, 'a' Adlib, 'r' Roland, 'p' none */
	int musicPort;
	int speechPort;
	int irq;
	int dma;
	char speechEnabled;
	SoundConfig();
	void reset();
	void readConfig(char *name);
	void close();
	int isRoland();
	int isAdlib();
	int isSoundBlaster();
	int hasMusic();
};

/* A stopwatch that can be stopped and restarted. */
struct Stopwatch {
	char running;
	unsigned long start, total;
	Stopwatch() { total = start = 0; }
	void begin() { start = TickCount; running = 1; }
};

/* Fills a view with frames of random pixels in one color, as a television's static. */
extern "C" void far DrawStatic(View *view, int frames, int color);

char *StaticPath = ".\\static\\";
char *GamedatPath = ".\\gamedat\\";
VideoMode OriginalVideoMode;
FadingPalette OriginalPalette;
int SpeedDivisor = 32;

/* The butterfly's flight, one point every few steps. */
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

FileSpeechCache SpeechCache;
MusicSystem Music;
FatalHandler PreviousFatalHook = 0;
RgbColor Black;
RgbColor Red;
char MusicFlexName[80];
char *MusicFlex = MusicFlexName;
unsigned char SpeechEnabled = 0;
SoundConfig SoundSetup;
AudioOptions AudioSettings;
CtrlBreakTrap CtrlBreakHook;
BiosHook SystemServicesHook;
char OptionDelimiters[] = "\r\n= ";
char MusicKeyword[] = "MUSIC";
char SpeechKeyword[] = "SPEECH";
char SfxKeyword[] = "SFX";
char InterruptKeyword[] = "INTERRUPT";
char PortKeyword[] = "PORT";
char AdlibKeyword[] = "ADLIB";
char RolandKeyword[] = "ROLAND";
char OnKeyword[] = "ON";
char OffKeyword[] = "OFF";

/* The sound effects, played on the music card. */
SoundEffect StaticSound(4, 100, 41, 64, 60, 0, 0);
SoundEffect PowerOffSound(0, 99, 48, 64, 90, 0, 0);
SoundEffect AppearSound(4, 62, 48, 127, 60, 0, 0);
SoundEffect UnusedAppearSound(4, 62, 48, 127, 60, 0, 0);
SoundEffect UnusedLowSound(4, 63, 24, 64, 120, 0, 0);
SoundEffect PunchSound(0, 105, 48, 127, 20, 0, 0);
SoundEffect MoongateSound(4, 106, 48, 64, 120, 0, 0);
SoundEffect EnterSound(0, 107, 41, 127, 5, 0, 0);
SoundEffect VanishSound(4, 109, 48, 96, 5, 0, 0);
SoundEffect UnusedTickSound(4, 110, 48, 64, 5, 0, 0);
SoundEffect DotSound(4, 111, 48, 64, 15, 0, 0);
VideoMode UnusedIntroVideoMode;

char *DataPath(char *dir, char *name)
{
	static char path[80];

	strcpy(path, dir);
	strcat(path, name);
	return path;
}

/* Waits up to ticks for a key. Escape ends the introduction; the skip key is left for the caller,
 * and the result says it came. */
unsigned char WaitForKey(unsigned ticks)
{
	Timer timer(ticks);
	int key;

	Timer_restart(&timer);
	while (!Timer_hasFinished(&timer) && !kbhit())
		;
	if (kbhit()) {
		if ((key = getch()) == KEY_ESCAPE) {
			Quit();
		} else if (key == KEY_SKIP) {
			ungetch(key);
			return 1;
		}
	}
	return 0;
}

/* Waits ticks, keys or not. */
void Delay(unsigned ticks)
{
	Timer timer(ticks);

	Timer_restart(&timer);
	while (!Timer_hasFinished(&timer))
		;
}

char *OtherDataPath(char *dir, char *name)
{
	static char path[30];

	strcpy(path, dir);
	strcat(path, name);
	return path;
}

/* Lord British presents: the card fades in from black, stays ticks, and fades out. */
void ShowPresents(int ticks, int fadeInDelay, int fadeOutDelay)
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
	if (kbhit())
		HandleKey();
}

/* The title, and the butterfly flying across it to the music; the skip key cuts it short. */
void ShowTitle(int ticks, int unusedTicks, int fadeInDelay, int speed, int unusedFlag)
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
		while (Stopwatch_getElapsed(&clock) < FLIGHT_START && !kbhit())
			;
		int point = 0;
		int frames = FLIGHT_START;
		int lastAngle = 0;
		int angle = 0;
		while (point++ < FLIGHT_POINTS - 1 && !kbhit()) {
			lastAngle = angle;
			butterfly.setPath(flightX[point - 1], flightY[point - 1], flightX[point], flightY[point], 256, 256,
				lastAngle, angle, speed);
			while (butterfly.step != butterfly.steps && !kbhit()) {
				if (random(5) < 4)
					butterfly.cycleFrames(0, 3);
				butterfly.advance();
				screen.paint(-1, 0);
				frames++;
				while (Stopwatch_getElapsed(&clock) < frames)
					;
			}
		}
		if (kbhit())
			HandleKey();
		butterfly.frame = 3;
		screen.paint(-1, 0);
		Delay(10);
		if (kbhit())
			HandleKey();
		butterfly.frame = 4;
		screen.paint(-1, 0);
		Delay(25);
		if (kbhit())
			HandleKey();
		butterfly.frame = 3;
		screen.paint(-1, 0);
		Delay(15);
		if (kbhit())
			HandleKey();
		butterfly.frame = 4;
		screen.paint(-1, 0);
		Delay(25);
		if (kbhit())
			HandleKey();
		butterfly.frame = 3;
		screen.paint(-1, 0);
		Delay(25);
		if (kbhit())
			HandleKey();
		butterfly.frame = 2;
		screen.paint(-1, 0);
		Delay(25);
		if (kbhit())
			HandleKey();
		butterfly.frame = 1;
		screen.paint(-1, 0);
		Delay(25);
		if (kbhit())
			HandleKey();
		butterfly.frame = 0;
		screen.paint(-1, 0);
		Delay(20);
		if (kbhit())
			HandleKey();
		while (!song.finished() && !kbhit())
			;
		if (kbhit())
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
void ShowStatic(int firstFrames, int secondFrames, int thirdFrames, int firstPause, int secondPause)
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
void ShowGuardian(int cycles, int delay, int firstInRate, int firstOutRate, int secondInRate,
	int secondOutRate, int unusedRate, char *trackFile, int trackEntry, int staticFrames, int pause,
	int endPause, int lineTicks)
{
	char *trackPath = new char[strlen(trackFile) + 1];
	if (!trackPath)
		ReportOutOfNearMemory();
	strcpy(trackPath, trackFile);
	FadingPalette palette;
	palette.load(DataPath(StaticPath, "intropal.dat"), INTROPAL_MONITOR);
	palette.apply();
	Song song(&Music, MusicFlex, 2);
	song.play();
	Screen screen;
	screen.drawShape(ENDSHAPE_MONITOR, DataPath(StaticPath, "endshape.flx"), 0, 0, 0);
	int cycle;
	for (cycle = 0; cycle < cycles; cycle++) {
		if (kbhit()) {
			HandleKey();
			break;
		}
		palette.rotate(16, 93, 1);
		Delay(delay);
	}
	int wait;
	{
		Sprite firstSwirl(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_FIRST_SWIRL, 1, &screen, 1);
		Sprite secondSwirl(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_SECOND_SWIRL, 1, &screen, 1);
		int step;

		firstSwirl.x = 118;
		firstSwirl.y = 48;
		secondSwirl.x = 147;
		secondSwirl.y = 62;
		AppearSound.play(-1);
		firstSwirl.frame = 9;
		step = 0;
		while (!kbhit() && firstSwirl.frame > 1) {
			screen.paint(-1, 0);
			if (step++ % firstInRate == 0)
				firstSwirl.frame--;
			palette.rotate(16, 93, 1);
		}
		if (!kbhit()) {
			AppearSound.stop();
			VanishSound.play(-1);
			step = 0;
			while (!kbhit() && firstSwirl.frame <= 9) {
				screen.paint(-1, 0);
				if (step++ % firstOutRate == 0)
					firstSwirl.frame++;
				palette.rotate(16, 93, 1);
			}
			if (!kbhit()) {
				firstSwirl.visible = 0;
				firstSwirl.drawn = 0;
				screen.paint(-1, 0);
				VanishSound.stop();
				step = 0;
				while (step++ < pause && !kbhit()) {
					palette.rotate(16, 93, 1);
					Delay(delay);
				}
				if (!kbhit()) {
					AppearSound.play(-1);
					secondSwirl.frame = 1;
					step = 0;
					while (!kbhit() && secondSwirl.frame < 8) {
						screen.paint(-1, 0);
						if (step++ % secondInRate == 0)
							secondSwirl.frame++;
						palette.rotate(16, 93, 1);
					}
					if (!kbhit()) {
						AppearSound.stop();
						VanishSound.play(-1);
						step = 0;
						while (!kbhit() && secondSwirl.frame >= 1) {
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
		if (kbhit()) {
			HandleKey();
			secondSwirl.visible = 0;
			secondSwirl.drawn = 0;
			screen.paint(-1, 0);
			goto done;
		}
	}
	wait = 0;
	while (wait++ < pause && !kbhit()) {
		palette.rotate(16, 93, 1);
		Delay(delay);
	}
	if (kbhit())
		HandleKey();
	{
		Sprite face(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_FACE, 1, &screen, 1);
		int step;

		face.x = 160;
		face.y = 100;
		AppearSound.play(-1);
		step = 0;
		while (!kbhit() && face.frameCount - 1 > face.frame) {
			if (step++ % secondInRate == 0)
				face.frame++;
			screen.paint(-1, 0);
			palette.rotate(16, 93, 1);
		}
		AppearSound.stop();
	}
	if (kbhit()) {
		HandleKey();
		goto done;
	}
	{
		SyncTrack track(trackPath, trackEntry);
		Guardian guardian(&screen);
		int i;

		guardian.moveTo(160, 100);
		guardian.frame = 1;
		TextFile lines(DataPath(StaticPath, "mainshp.flx"), MAINSHP_SPEECH_TEXT);
		TextSprite subtitle(DataPath(StaticPath, "mainshp.flx"), MAINSHP_FONT, 0, &screen, 1);
		subtitle.moveTo(159, 180);
		subtitle.visible = 0;
		subtitle.drawn = 0;
		subtitle.spacing = -2;
		int line = 0;
		SyncCue *cue;
		subtitle.setText(lines.getLine(line));
		subtitle.align = ALIGN_CENTRE;
		i = 0;
		while (i++ < 20) {
			palette.rotate(93, 16, 1);
			Delay(delay);
		}
		guardian.setMood(1);
		guardian.setMouthShape(1);
		screen.paint(-1, 0);
		i = 0;
		while (i++ < 5) {
			palette.rotate(93, 16, 1);
			Delay(delay);
		}
		guardian.setMood(1);
		guardian.setMouthShape(0);
		screen.paint(-1, 0);
		i = 0;
		while (i++ < 9) {
			palette.rotate(93, 16, 1);
			Delay(delay);
		}
		guardian.setMouthShape(3);
		screen.paint(-1, 0);
		i = 0;
		while (i++ < 10) {
			palette.rotate(93, 16, 1);
			Delay(delay);
		}
		guardian.setMood(1);
		guardian.setMouthShape(0);
		screen.paint(-1, 0);
		i = 0;
		while (i++ < 15) {
			palette.rotate(93, 16, 1);
			Delay(delay);
		}
		screen.paint(-1, 0);
		unsigned char done = 0;
		if (SpeechEnabled) {
			SpeechCard.resetPlayback();
			SpeechCache.play(DataPath(StaticPath, "introsnd.dat"), 5);
			SpeechCard.play(&SpeechCache);
		}
		if (!SpeechEnabled)
			subtitle.visible = 1;
		Stopwatch clock;
		Stopwatch lineClock;
		clock.begin();
		lineClock.begin();
		cue = track.next(Stopwatch_getElapsed(&clock));
		while ((!SpeechFinished || !done) && !(kbhit() && SpeechEnabled)) {
			if (!SpeechEnabled && (kbhit() || Stopwatch_getElapsed(&lineClock) > lineTicks)) {
				if (kbhit())
					getch();
				if (++line >= lines.count)
					break;
				subtitle.setText(lines.getLine(line));
				Stopwatch_stop(&lineClock);
				lineClock.total = 0;
				lineClock.begin();
			}
			if (BlockConsumed)
				SpeechCache.queueNextBlock();
			if (Stopwatch_getElapsed(&clock) >= cue->time && !done) {
				switch (cue->value) {
				case CUE_RESTART:
					if (!SpeechEnabled) {
						Stopwatch_stop(&clock);
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
				cue = track.next(Stopwatch_getElapsed(&clock));
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
	if (kbhit()) {
		HandleKey();
		goto done;
	}
	{
		Sprite face(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_FACE, 1, &screen, 1);
		int step;

		face.x = 160;
		face.y = 100;
		VanishSound.play(-1);
		face.frame = face.frameCount - 1;
		step = 0;
		while (!kbhit() && face.frame > 0) {
			if (step++ % secondInRate == 0)
				face.frame--;
			screen.paint(-1, 0);
			palette.rotate(16, 93, 1);
		}
		VanishSound.stop();
	}
	if (kbhit()) {
		HandleKey();
		goto done;
	}
	screen.paint(-1, 0);
	wait = 0;
	while (wait++ < endPause && !kbhit()) {
		palette.rotate(16, 93, 1);
		Delay(delay);
	}
	if (kbhit())
		HandleKey();
	song.fadeOut(0);
	StaticSound.play(-1);
	DrawStatic(&ScreenView, staticFrames, STATIC_COLOR);
	{
		Sprite dot(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_DOT, 0, &screen, 1);
		int step;

		FillView(&screen, 0);
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
}

/*
 * The Avatar's desk. The view pulls back from the dot on the monitor to the whole monitor, a fist
 * strikes it three times, and the room pans across the cloth map and down to the Orb of the Moons,
 * a caption at each stage.
 */
void ShowDesk(int ticks, int captionTicks, int panTicks, int captionDelay, int delay)
{
	FadingPalette palette;
	palette.fill(&Black, 0, PALETTE_COLORS - 1);
	palette.load(DataPath(StaticPath, "intropal.dat"), INTROPAL_DESK);
	Screen screen;
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
		int x, y;

		title.attach(&screen);
		dot.attach(&screen);
		topLeft.setPath(138, 144, 160, 100, 512, 256, 0, 0, 1320 / SpeedDivisor);
		topRight.setPath(138, 144, 160, 100, 512, 256, 0, 0, 1320 / SpeedDivisor);
		bottomLeft.setPath(138, 144, 160, 100, 512, 256, 0, 0, 1320 / SpeedDivisor);
		bottomRight.setPath(138, 144, 160, 100, 512, 256, 0, 0, 1320 / SpeedDivisor);
		dot.setPath(160, 104, 170, 80, 256, 256, 0, 0, 1320 / SpeedDivisor);
		Stopwatch clock;
		int frames = 0;
		clock.begin();
		while (!kbhit() && topLeft.step != topLeft.steps) {
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
			while (Stopwatch_getElapsed(&clock) < frames)
				;
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
			View monitor(92, 26, 251, 130);
			monitor.id = ScreenView.id;
			monitor.rows = ScreenView.rows;
			MovingSprite fist(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_FIST, 0, &screen, 0);
			fist.x = 0;
			fist.y = 150;
			fist.scale = 200;
			int blow = 0;
			Stopwatch blowClock;
			int blowFrames = 0;
			blowClock.begin();
			while (blow++ < 3 && !kbhit()) {
				fist.slideBy(0, 0, 0, 33, 1);
				while (!kbhit() && fist.step != fist.steps) {
					fist.advance();
					screen.paint(-1, 0);
					blowFrames++;
					while (Stopwatch_getElapsed(&blowClock) < blowFrames)
						;
				}
				if (kbhit()) {
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
				while (!kbhit() && fist.step != fist.steps) {
					if (blow < 3)
						DrawStatic(&monitor, 1, STATIC_COLOR);
					else
						FillView(&monitor, 0);
					fist.advance();
					screen.paint(-1, 0);
					blowFrames++;
					while (Stopwatch_getElapsed(&blowClock) < blowFrames)
						;
				}
			}
			if (kbhit())
				HandleKey();
		}
		StaticSound.stop();
		screen.paint(-1, 0);
		if (WaitForKey(ticks))
			HandleKey();
		{
			Sprite amiss(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_AMISS, 0, &screen, 0);
			char key;

			amiss.x = 160;
			amiss.y = 150;
			screen.paint(-1, 0);
			captionClock.begin();
			while (Stopwatch_getElapsed(&captionClock) < captionTicks && !kbhit())
				;
			if (kbhit()) {
				if ((key = getch()) == KEY_ESCAPE) {
					Quit();
				} else {
					ungetch(key);
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
		int step = 0;
		Stopwatch panClock;
		panClock.begin();
		while (!kbhit() && topLeft.step != topLeft.steps) {
			if (step++ > captionDelay)
				longTime.visible = 1;
			topLeft.advance();
			topRight.advance();
			bottomLeft.advance();
			bottomRight.advance();
			map.advance();
			title.advance();
			frames++;
			while (Stopwatch_getElapsed(&panClock) < frames)
				;
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
		int cycle;

		beckons.x = 160;
		beckons.y = 10;
		desk.x = 319;
		desk.y = 199;
		topRight.slideBy(0, -52, 0, 0, 1716 / SpeedDivisor);
		bottomRight.slideBy(0, -52, 0, 0, 1716 / SpeedDivisor);
		map.slideBy(0, -52, 0, 0, 1716 / SpeedDivisor);
		desk.slideBy(0, -52, 0, 0, 1716 / SpeedDivisor);
		title.slideBy(0, -52, 0, 0, 1716 / SpeedDivisor);
		cycle = 0;
		int frames = 0;
		Stopwatch clock;
		clock.begin();
		while (topRight.step != topRight.steps && !kbhit()) {
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
			while (Stopwatch_getElapsed(&clock) < frames)
				;
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
			int i;

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

/* The circle of stones behind the house, and the moongate standing in it. The darkness parts, the
 * captions ask their question, and the view closes in on the moongate. */
void ShowMoongate(int delay, int glowCycles, int unusedCycles, int captionDelay, int endCycles)
{
	FadingPalette palette;
	palette.fill(&Black, 0, PALETTE_COLORS - 1);
	palette.apply();
	palette.load(DataPath(StaticPath, "intropal.dat"), INTROPAL_MOONGATE);
	Screen screen;
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
		int cycle = 0;
		while (cycle++ < glowCycles && !kbhit()) {
			palette.rotate(254, 240, 1);
			Delay(delay);
		}
		if (kbhit())
			HandleKey();
	}
	{
		Sprite whyMoongate(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_WHY_MOONGATE, 0, &screen, 0);
		Sprite onePath(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_ONE_PATH, 0, &screen, 0);
		int step;

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
		while (!kbhit() && leftCurtain.step != leftCurtain.steps) {
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
			screen.paint(-1, 0);
			Delay(0);
		}
		if (kbhit()) {
			HandleKey();
			leftCurtain.finish();
			rightCurtain.finish();
			screen.paint(-1, 0);
		}
		onePath.visible = 0;
		onePath.drawn = 0;
		screen.paint(-1, 0);
		step = 0;
		while (step++ < endCycles) {
			palette.rotate(254, 240, 1);
			Delay(delay);
		}
	}
	topLeft.slideBy(60, 150, 2048, 0, 12);
	right.slideBy(60, 150, 2048, 0, 12);
	bottomLeft.slideBy(60, 150, 2048, 0, 12);
	bottomRight.slideBy(60, 150, 2048, 0, 12);
	while (!kbhit() && topLeft.step != topLeft.steps) {
		topLeft.advance();
		right.advance();
		bottomLeft.advance();
		bottomRight.advance();
		palette.apply();
		palette.rotate(254, 240, 1);
		screen.paint(-1, 0);
	}
	if (kbhit())
		HandleKey();
	MoongateSound.stop();
	EnterSound.play(-1);
	FillView(&ScreenView, 0);
	Delay(60);
}

/* Routes fatal errors through OnFatalError, and opens the far heap. */
void InstallFatalHook(void)
{
	PreviousFatalHook = SwapFatalHook(OnFatalError);
	StartFarHeap(0);
}

/* Blackens the screen and gives back the far heap; with restorePalette also the palette and the
 * video mode. */
void RestoreSystem(char restorePalette)
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
	if (restorePalette)
		SetVideoMode(&OriginalVideoMode.mode);
	while (kbhit())
		getch();
}

/* Escape ends the introduction; other keys are dropped. */
void HandleKey(void)
{
	if (kbhit() && getch() == KEY_ESCAPE)
		Quit();
}

/* Stops the music and shuts down the speech card, the speech file and the fast timer. */
void CloseDevices(void)
{
	Music.stop();
	SpeechCard.SoundBlaster::~SoundBlaster();
	SpeechCache.FileSpeechCache::~FileSpeechCache();
	SystemTimer.SysTimer::~SysTimer();
}

/* Ends the introduction; the launcher goes on to the main menu. */
void QuitToDos(void)
{
	CloseDevices();
	RestoreSystem(0);
	exit(EXIT_DONE);
}

/* Runs the main menu in place of this program. Nothing calls it. */
void ReturnToMenu(void)
{
	RestoreSystem(1);
	execl("mainmenu.exe", "mainmenu.exe", "V", 0, 0);
	exit(EXIT_MENU);
}

void Quit(void)
{
	QuitToDos();
	exit(EXIT_DONE);
}

/* A fatal error puts the screen and palette back before the message. */
void OnFatalError(void)
{
	CloseDevices();
	RestoreSystem(1);
	if (PreviousFatalHook)
		PreviousFatalHook();
}

/* Keeps the video mode and palette to restore, blackens the screen and gives the drawing view its
 * buffer. */
void InitEnvironment(void)
{
	GetVideoMode(&OriginalVideoMode.mode);
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

/* The MUSIC, SFX and SPEECH lines of an options file; false if it cannot be read. */
unsigned char ReadAudioOptions(char *filename, AudioOptions *settings)
{
	DataFile options(filename, 1);
	if (options.reopen(1) != 1)
		return 0;
	char *save;
	char *setting;
	long length;
	char line[80];
	char *value;
	settings->music = AUDIO_ON;
	settings->speech = AUDIO_ON;
	settings->effects = AUDIO_ON;
	for (;;) {
		length = options.readUntil('\r', line, 79L);
		if (length == 0)
			break;
		line[length] = 0;
		setting = NextToken(line, OptionDelimiters, &save);
		if (stricmp(setting, MusicKeyword) == 0) {
			settings->music = AUDIO_ON;
			while ((value = NextToken(0, OptionDelimiters, &save)) != 0) {
				if (stricmp(value, OnKeyword) == 0) {
					settings->music = AUDIO_ON;
				} else if (stricmp(value, OffKeyword) == 0) {
					settings->music = AUDIO_OFF;
				}
			}
		}
		if (stricmp(setting, SfxKeyword) == 0) {
			settings->effects = AUDIO_ON;
			if ((value = NextToken(0, OptionDelimiters, &save)) != 0) {
				if (stricmp(value, OnKeyword) == 0) {
					settings->effects = AUDIO_ON;
				} else if (stricmp(value, OffKeyword) == 0) {
					settings->effects = AUDIO_OFF;
				}
			}
		}
		if (stricmp(setting, SpeechKeyword) == 0) {
			settings->speech = AUDIO_ON;
			while ((value = NextToken(0, OptionDelimiters, &save)) != 0) {
				if (stricmp(value, OnKeyword) == 0) {
					settings->speech = AUDIO_ON;
				} else if (stricmp(value, OffKeyword) == 0) {
					settings->speech = AUDIO_OFF;
				}
			}
		}
	}
	return 1;
}

/* The sound card setup from configuration, then the player's audio choices from preferences. */
void ConfigureSound(char *configuration, char *preferences, int *irq, int *port, unsigned char *device, int *dma)
{
	char *preferencesPath;

	*irq = 7;
	*port = 0x220;
	*dma = 1;
	*device = 0;
	preferencesPath = new char[strlen(preferences) + 1];
	if (preferencesPath == 0)
		ReportOutOfNearMemory();
	strcpy(preferencesPath, preferences);
	SoundSetup.readConfig(configuration);
	if ((unsigned char) SoundSetup.isAdlib() || (unsigned char) SoundSetup.isSoundBlaster()) {
		strcpy(MusicFlexName, DataPath(StaticPath, "introadm.dat"));
		*device = MUSIC_DEVICE_ADLIB;
	}
	if ((unsigned char) SoundSetup.isRoland()) {
		strcpy(MusicFlexName, DataPath(StaticPath, "intrordm.dat"));
		*device = MUSIC_DEVICE_MT32;
	}
	*irq = SoundSetup.irq;
	*port = SoundSetup.speechPort;
	*dma = SoundSetup.dma;
	if (SoundSetup.speechEnabled)
		SpeechEnabled = 1;
	else
		SpeechEnabled = 0;
	if ((unsigned char) SoundSetup.hasMusic()) {
		EnableMusic();
		EnableSfx();
	}
	if (ReadAudioOptions(preferencesPath, &AudioSettings)) {
		if (AudioSettings.music == AUDIO_OFF)
			DisableMusic();
		if (AudioSettings.effects == AUDIO_OFF)
			DisableSfx();
		SpeechEnabled = 0;
		if (AudioSettings.speech == AUDIO_ON && SoundSetup.speechEnabled)
			SpeechEnabled = 1;
	}
	delete preferencesPath;
}

/* Always passes. */
int CheckInstallation(void)
{
	return 1;
}

/* Counts how many times the screen can be painted in 150 ticks. Nothing calls it; SpeedDivisor keeps
 * its default. */
void MeasureSpeed(void)
{
	VideoMode mode;
	GetVideoMode(&mode.mode);
	int paints = 0;
	FadingPalette palette;
	palette.fill(&Black, 0, PALETTE_COLORS - 1);
	palette.apply();
	palette.load(DataPath(StaticPath, "intropal.dat"), INTROPAL_PRESENTS);
	Screen screen;
	screen.drawShape(ENDSHAPE_PRESENTS, DataPath(StaticPath, "endshape.flx"), 0, 0, 0);
	Stopwatch clock;
	clock.begin();
	while (Stopwatch_getElapsed(&clock) < 150) {
		paints++;
		screen.paint(-1, 0);
		/* busy work, standing in for a scene's own */
		for (int i = 0; i < 1000; i++) {
			int work = i;
			work = random(20);
			work *= i;
		}
	}
	SpeedDivisor = paints;
}

/*
 * The launcher runs this with its password first; a second argument starting with A or R picks
 * the Adlib or Roland music over the configured card. The six scenes play in turn.
 */
void main(int argc, char **argv)
{
	unsigned char device;
	int irq, port, dma;

	if (argc < 2)
		FatalError("Type ULTIMA7 to play Ultima VII\n");
	if (stricmp(argv[1], "EREIAMJH"))
		FatalError("Type ULTIMA7 to play Ultima VII\n");
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
	{
		MovingSprite unused;

		ShowGuardian(120, 3, 2, 5, 5, 5, 2, DataPath(StaticPath, "mainshp.flx"), MAINSHP_SYNC_TRACK, 20, 50, 40,
			305);
		ShowDesk(60, 200, 100, 40, 10);
	}
	ShowMoongate(5, 15, 40, 8, 40);
	QuitToDos();
}
