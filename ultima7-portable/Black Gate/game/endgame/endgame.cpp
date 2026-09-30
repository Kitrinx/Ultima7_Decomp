/* Black Gate ENDGAME.EXE: endgame.c, the ending. The Guardian at the Black Gate, the Gate
 * destroyed, the Guardian's parting threat, then the epilogue and how long the game took.
 */

#include "u7port.h"
#include "plat.h"
#include "arena.h"
#include "dosio.h"
#include "lowlevel.h"
#include "view.h"
#include "memapi.h"
#include "screen.h"
#include "systimer.h"
#include "gtimer.h"
#include "preload.h"
#include "../ultima7/programs.h"
#include <new>

/* U7's sound setup and audio switches, read the way the game reads them. */
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

struct AudioOptions {
	uint8_t music, speech, effects;
};

#include "endview.h"
#include "iff.h"
#include "palette.h"
#include "flic.h"
#include "textwin.h"
#include "speech.h"
#include "ailmt32.h"
#include "xmidi.h"

namespace Endgame {

#define EXIT_CREDITS    7

#define FRAME_TICKS     4
#define SHAPE_CYCLE     8       /* overlay selectors 0-7; 0 draws nothing */

#define LEAD_IN_FRAMES  171     /* flic1's first frame, held before it plays */
#define NO_FRAME        155     /* where the Guardian cries "No! You must not!" */
#define DAMN_FRAME      34      /* flic2 draws the overlay only after this frame */

#define PAGE_TICKS      1000    /* each epilogue page, unless a key is pressed */
#define LAST_TICKS      36000L

static int16_t FadeSpeed = 1;
static int16_t SceneDelay = 90;

static ::View Screen;
static ::View *const ScreenView = &Screen;

static void ReadError()
{
	plat_fatal("Error reading data file.");
}

/* Waits ticks, or less if a key is pressed; says whether one was. */
static uint8_t WaitOrKey(uint32_t ticks)
{
	Timer timer;

	Timer_set(&timer, ticks);
	while (!Timer_hasFinished(&timer) && !plat_key_available())
		plat_yield();
	if (plat_key_available()) {
		plat_key_get();
		return 1;
	}
	return 0;
}

/* Reads the current chunk into the far heap, where the shape drawing can reach it. */
static int32_t LoadShapes(IffFile *file, void **memory)
{
	*memory = AllocateFarHeap(file->chunk.size, 0);
	if (*memory == 0)
		plat_fatal("Out of memory.");
	file->readChunk(*memory);
	return (int32_t) PointerToLinear(*memory);
}

/* Loads the named font, with the spacing every font of the ending uses. */
static ShapeFont *LoadFont(IffFile *file, const char *name, void **memory)
{
	ShapeFont *font;

	FindNamedChunk(file, "FONT", name);
	font = new ShapeFont(LoadShapes(file, memory));
	font->setSpacing(-1, -1, 0);
	return font;
}

/* One page of text on a black screen, faded in from black and out again. */
static void ShowPage(TextBox *box, Palette *palette, FadePalette *fade, Rgb *black,
	const char *const *lines, const int16_t *rows, int16_t count)
{
	FillView(ScreenView, 0);
	for (int16_t i = 0; i < count; i++)
		box->print(lines[i], rows[i], JUSTIFY_CENTER);
	*fade = *palette;
	fade->fadeFromColor(black, FadeSpeed, 0, 256);
	WaitOrKey(PAGE_TICKS);
	*fade = *palette;
	fade->fadeToColor(black, FadeSpeed, 0, 256);
}

static int16_t Run(int16_t argc, char **argv)
{
	if (argc < 2 || stricmp(argv[1], "EREIAMJH")) {
		plat_log("Type ULTIMA7 to play Ultima VII\n");
		return 1;
	}
	/* argv[1] again, not argv[2]: a fade speed given here becomes 0. */
	if (argc >= 3)
		FadeSpeed = (int16_t) atol(argv[1]);
	if (argc >= 4)
		SceneDelay = (int16_t) atol(argv[2]);

	int16_t song = 0;
	InitVgaScreen(&Screen, 0);
	SystemTimer.install();

	uint8_t speech, music;
	AudioOptions options;
	options.music = options.speech = options.effects = AUDIO_ON;
	ReadAudioOptions("gamedat\\options.cfg", &options);
	speech = options.speech == AUDIO_ON;
	music = options.music == AUDIO_ON;

	SoundConfig config;
	config.readConfig("u7.cfg");
	if (!config.speechEnabled)
		speech = 0;
	/* Music plays only on the MT-32: any configured music device becomes one. Without the synth
	 * the ending is silent. */
	SoundDriver *musicDriver = 0;
	if (config.hasMusic()) {
		if (Mt32Open()) {
			musicDriver = new SoundDriver("static\\xmidi.mt");
			song = 1;
		}
	}
	if (musicDriver == 0)
		music = 0;

	Timer timer;
	::View view;
	view.clip.x0 = 0;
	view.clip.y0 = 0;
	view.clip.x1 = SCREEN_WIDTH - 1;
	view.clip.y1 = SCREEN_HEIGHT - 1;
	if (!AllocateDrawBuffer(&view, 0xff, 0))
		plat_fatal("Out of memory.");
	IffFile file;
	file.open("static\\endgame.dat");
	file.enterForm();

	/* Without speech the Guardian's lines are printed. */
	void *fontData = 0;
	ShapeFont *font = 0;
	TextWindow *window = 0;
	if (!speech) {
		font = LoadFont(&file, "font2", &fontData);
		window = new TextWindow(&view, font);
	}

	file.findChunk("SHAP");
	void *shapeData;
	int32_t shapes = LoadShapes(&file, &shapeData);

	XmidiPlayer *score = 0;
	if (music) {
		score = new XmidiPlayer(musicDriver, "static\\endscore.xmi");
		score->play(0);
		while (!score->isDone(0))
			plat_yield();
	}

	/* The Guardian at the Gate. */
	{
		SpeechStream *voice = 0;
		uint8_t *voc1 = 0;
		if (speech) {
			FindNamedChunk(&file, "VOCF", "voc1");
			voc1 = new uint8_t[file.chunk.size];
			voice = new SpeechStream(voc1, file.chunk.size);
			file.readChunk(voc1);
		}
		Flic flic;
		if (!flic.load(&file, "flic1"))
			ReadError();
		if (music)
			score->play(song);
		int16_t frames = flic.frameCount();
		int16_t i;
		for (i = 0; i < LEAD_IN_FRAMES; i++) {
			Timer_set(&timer, FRAME_TICKS);
			flic.rewind();
			flic.nextFrame();
			flic.show(&view);
			int16_t shape = i < SHAPE_CYCLE ? i : i % SHAPE_CYCLE;
			if (shape)
				DrawShape(&view, 0, 0, shapes, shape);
			CopyView(&view, ScreenView);
			Timer_wait(&timer);
		}
		flic.rewind();
		uint8_t sayNo = 0;
		for (i = 0; i < frames; i++) {
			Timer_set(&timer, FRAME_TICKS);
			if (i == NO_FRAME) {
				if (speech)
					voice->play(-1);
				else
					sayNo++;
			}
			flic.nextFrame();
			flic.show(&view);
			int16_t shape = i < SHAPE_CYCLE ? i : i % SHAPE_CYCLE;
			if (shape)
				DrawShape(&view, 0, 0, shapes, shape);
			if (sayNo)
				window->print("%Y%JNo!  You must not!", 175, JUSTIFY_CENTER);
			CopyView(&view, ScreenView);
			Timer_wait(&timer);
		}
		if (voice) {
			voice->stopPlayback();
			delete voice;
		}
		delete[] voc1;
	}

	/* The Gate destroyed. */
	{
		Flic flic;
		if (!flic.load(&file, "flic2"))
			ReadError();
		SpeechStream *speaker = 0;
		uint8_t *voc2 = 0;
		if (speech) {
			FindNamedChunk(&file, "VOCF", "voc2");
			voc2 = new uint8_t[file.chunk.size];
			speaker = new SpeechStream(voc2, file.chunk.size);
			file.readChunk(voc2);
			speaker->play(-1);
		}
		if (music) {
			score->play(song + 1);
			score->stop(song);
		}
		int16_t frames = flic.frameCount();
		int16_t i;
		for (i = 0; i < frames; i++) {
			Timer_set(&timer, FRAME_TICKS);
			flic.nextFrame();
			flic.show(&view);
			if (i > DAMN_FRAME) {
				int16_t shape = i % SHAPE_CYCLE;
				if (shape)
					DrawShape(&view, 0, 0, shapes, shape);
			}
			if (speaker)
				speaker->service();
			else
				window->print("%Y%JDamn you Avatar!  Damn you!", 175, JUSTIFY_CENTER);
			CopyView(&view, ScreenView);
			Timer_wait(&timer);
		}
		if (speech) {
			/* The last frame stays; the overlay keeps drawing over it. */
			while (!speaker->isDone()) {
				for (i = 0; i < SHAPE_CYCLE; i++) {
					Timer_set(&timer, FRAME_TICKS);
					speaker->service();
					int16_t shape = i % SHAPE_CYCLE;
					if (shape)
						DrawShape(ScreenView, 0, 0, shapes, shape);
					Timer_wait(&timer);
				}
			}
		} else {
			Timer_set(&timer, 120);
			Timer_wait(&timer);
		}
		delete speaker;
		delete[] voc2;
		FreeFarHeap(shapeData);

		Palette palette;
		palette.load(0, 256, (Rgb *) flic.getPalette());
		palette.write(0, 256, 0);
		Rgb black(0, 0, 0);
		FadePalette fade(palette);
		fade.fadeToColor(&black, FadeSpeed, 0, 256);
		Timer_set(&timer, SceneDelay);
		Timer_wait(&timer);

		void *cardData;
		ShapeFont *cardFont = LoadFont(&file, "font1", &cardData);
		::Rect screen = { 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1 };
		TextBox box(screen, ScreenView, cardFont);
		FillView(ScreenView, 0);
		box.print("%Y%JThe Black Gate is destroyed.", 80, JUSTIFY_CENTER);
		fade = palette;
		fade.fadeFromColor(&black, 2, 0, 256);
		Timer_set(&timer, 100);
		Timer_wait(&timer);
		fade = palette;
		fade.fadeToColor(&black, 2, 0, 256);
		FillView(ScreenView, 0);
		box.print("%Y%JThe Guardian has been stopped.", 80, JUSTIFY_CENTER);
		fade = palette;
		fade.fadeFromColor(&black, 2, 0, 256);
		Timer_set(&timer, 100);
		Timer_wait(&timer);
		fade.fadeToColor(&black, 2, 0, 256);
		delete cardFont;
		FreeFarHeap(cardData);
	}

	if (!speech) {
		delete window;
		delete font;
		FreeFarHeap(fontData);
		font = LoadFont(&file, "font3", &fontData);
		window = new TextWindow(&view, font);
	}

	/* The Guardian's parting threat, then the epilogue. */
	{
		Flic flic;
		if (!flic.load(&file, "flic3"))
			ReadError();
		SpeechStream *speaker = 0;
		uint8_t *voc3 = 0;
		if (speech) {
			FindNamedChunk(&file, "VOCF", "voc3");
			voc3 = new uint8_t[file.chunk.size];
			speaker = new SpeechStream(voc3, file.chunk.size);
			file.readChunk(voc3);
		}
		Rgb black(0, 0, 0);
		flic.nextFrame();
		flic.showInColor(ScreenView, &black);
		Palette palette;
		palette.load(0, 256, (Rgb *) flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&black, FadeSpeed, 0, 256);
		int16_t frames = flic.frameCount();
		Timer hold;
		Timer_set(&hold, SceneDelay + 100);
		while (!Timer_hasFinished(&hold)) {
			for (int16_t i = 0; i < frames; i++) {
				Timer_set(&timer, FRAME_TICKS);
				flic.nextFrame();
				flic.show(&view);
				CopyView(&view, ScreenView);
				Timer_wait(&timer);
			}
			flic.rewind();
		}
		if (speaker)
			speaker->play(-1);
		/* No score counts as silent. */
		if (speech) {
			while (!speaker->isDone() || (score && score->isPlaying(song + 1))) {
				for (int16_t i = 0; i < frames; i++) {
					Timer_set(&timer, FRAME_TICKS);
					speaker->service();
					flic.nextFrame();
					flic.show(&view);
					CopyView(&view, ScreenView);
					Timer_wait(&timer);
				}
				flic.rewind();
			}
		} else {
			Timer_set(&hold, 1200);
			while (!Timer_hasFinished(&hold) || (score && score->isPlaying(song + 1))) {
				for (int16_t i = 0; i < frames; i++) {
					Timer_set(&timer, FRAME_TICKS);
					flic.nextFrame();
					flic.show(&view);
					window->print("%Y%JAvatar!  You think you have won?", 40, JUSTIFY_CENTER);
					window->print("%Y%JThink again!  You are unable to", 55, JUSTIFY_CENTER);
					window->print("%Y%Jleave Britannia, whereas I am free", 70, JUSTIFY_CENTER);
					window->print("%Y%Jto enter other worlds!  Hmmm...", 85, JUSTIFY_CENTER);
					window->print("%Y%JPerhaps your puny Earth shall be", 100, JUSTIFY_CENTER);
					window->print("%Y%Jmy NEXT target!", 115, JUSTIFY_CENTER);
					CopyView(&view, ScreenView);
					Timer_wait(&timer);
				}
				flic.rewind();
			}
		}
		fade.fadeToColor(&black, FadeSpeed, 0, 256);
		delete speaker;
		delete[] voc3;

		{
			void *pageData;
			ShapeFont *pageFont = LoadFont(&file, "font4", &pageData);
			::Rect screen = { 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1 };
			TextBox box(screen, ScreenView, pageFont);
			while (plat_key_available())
				plat_key_get();

			static const char *const page1[] = {
				"%Y%JIn the months following the climactic",
				"%Y%Jbattle at The Black Gate, Britannia",
				"%Y%Jis set upon the long road to recovery",
				"%Y%Jfrom its various plights.",
				"%Y%JUpon your return to Britain,",
				"%Y%JLord British decreed that",
				"%Y%JThe Fellowship be outlawed,",
				"%Y%Jand all of the branches were",
				"%Y%Jsoon destroyed.",
			};
			static const int16_t rows1[] = { 25, 40, 55, 70, 100, 115, 130, 145, 160 };
			static const char *const page2[] = {
				"%Y%JThe frustration you feel at having been",
				"%Y%Jstranded in Britannia is somewhat",
				"%Y%Jalleviated by the satisfaction that you",
				"%Y%Jsolved the gruesome murders committed",
				"%Y%Jby The Fellowship and even avenged the",
				"%Y%Jdeath of Spark's father.",
			};
			static const int16_t rows2[] = { 55, 70, 85, 100, 115, 130 };
			static const char *const page3[] = {
				"%Y%JAnd although you are, at the moment, ",
				"%Y%Jhelpless to do anything about",
				"%Y%JThe Guardian's final threat,",
				"%Y%Janother thought nags at you...",
				"%Y%Jwhat became of Batlin, the fiend",
				"%Y%Jwho got away?",
			};
			static const char *const page4[] = {
				"%Y%JThat is another story...",
				"%Y%Jone that will take you",
				"%Y%Jto a place called",
				"%Y%JThe Serpent Isle...",
			};
			static const int16_t rows4[] = { 70, 85, 100, 115 };
			ShowPage(&box, &palette, &fade, &black, page1, rows1, 9);
			ShowPage(&box, &palette, &fade, &black, page2, rows2, 6);
			ShowPage(&box, &palette, &fade, &black, page3, rows2, 6);
			ShowPage(&box, &palette, &fade, &black, page4, rows4, 4);
			delete pageFont;
			FreeFarHeap(pageData);
		}

		/* How long the game took, when the game left a record of it. */
		GameDate date;
		if (plat_file_exists("static\\endstats.dat") && date.load("static\\endstats.dat")) {
			void *statsData;
			ShapeFont *statsFont = LoadFont(&file, "font4", &statsData);
			::Rect screen = { 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1 };
			TextBox box(screen, ScreenView, statsFont);
			FillView(ScreenView, 0);
			box.print("%Y%JCongratulations!", 40, JUSTIFY_CENTER);
			box.print("%Y%JYou have completed Ultima VII in", 55, JUSTIFY_CENTER);
			{
				char comma = 0;
				char text[120];
				char part[40];
				strcpy(text, "%Y%J");
				if (date.year) {
					snprintf(part, sizeof part, "%d year", (int16_t) date.year);
					strcat(text, part);
					if ((int16_t) date.year > 1)
						strcat(text, "s");
					comma = 1;
				}
				if (date.month) {
					if (comma)
						strcat(text, ", ");
					snprintf(part, sizeof part, "%d month", (int16_t) date.month);
					strcat(text, part);
					if ((int16_t) date.month > 1)
						strcat(text, "s");
					comma = 1;
				}
				if (date.day) {
					if (comma)
						strcat(text, ", & ");
					snprintf(part, sizeof part, "%d day", (int16_t) date.day);
					strcat(text, part);
					if ((int16_t) date.day > 1)
						strcat(text, "s");
					strcat(text, ".");
				}
				box.print(text, 70, JUSTIFY_CENTER);
			}
			box.print("%Y%JPlease write Lord British,", 85, JUSTIFY_CENTER);
			box.print("%Y%Jin care of Origin Systems Inc.,", 100, JUSTIFY_CENTER);
			box.print("%Y%Jtelling him of your", 115, JUSTIFY_CENTER);
			box.print("%Y%Jaccomplishment!", 130, JUSTIFY_CENTER);
			fade = palette;
			fade.fadeFromColor(&black, FadeSpeed, 0, 256);
			while (!WaitOrKey(LAST_TICKS))
				;
			fade = palette;
			fade.fadeToColor(&black, FadeSpeed, 0, 256);
			delete statsFont;
			FreeFarHeap(statsData);
		}
	}
	file.close();

	/* Tells the game the ending has been seen. */
	{
		int16_t seen = plat_file_create("static\\endgame.flg");
		if (seen >= 0) {
			plat_file_write(seen, "Congratulations!", 17);
			plat_file_close(seen);
		}
	}

	delete score;
	delete musicDriver;
	return EXIT_CREDITS;
}

}

extern "C" int16_t EndgameMain(int16_t argc, char **argv)
{
	return Endgame::Run(argc, argv);
}

extern "C" void ResetEndgameEndgameGlobals(void)
{
	Endgame::FadeSpeed = 1;
	Endgame::SceneDelay = 90;
	memset((void *) &Endgame::Screen, 0, sizeof Endgame::Screen);
}

extern "C" void ConstructEndgameEndgameGlobals(void)
{
	new (&Endgame::Screen) ::View();
}
