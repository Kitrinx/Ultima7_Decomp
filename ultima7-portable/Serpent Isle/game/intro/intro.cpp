/* Serpent Isle INTRO.EXE, resident segment 1 (file offsets 0x005f8b to 0x008f7f, 12276 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 * File name inferred: Black Gate INTRO.EXE's main module, which this one closely follows.
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
#include "preload.h"
#include "../serpent/programs.h"
#include <new>
#include <stdarg.h>

/* The sound setup from serpent.cfg and the audio switches, read the way the game reads them. */
struct SoundConfig {
	int8_t device;            /* 's' Sound Blaster, 'a' Adlib, 'r' Roland, 'p' none */
	int16_t musicPort;
	int16_t speechPort;
	int16_t irq;
	int16_t dma;
	int8_t speechEnabled;
	int8_t optionEnabled;
	SoundConfig();
	void reset();
	void readConfig(char *name);
	void close();
	int16_t isRoland();
	int16_t isAdlib();
	int16_t isSoundBlaster();
	int16_t hasMusic();
	int8_t getSpeechEnabled() { return speechEnabled; }
	int8_t getOptionEnabled() { return optionEnabled; }
	int16_t getSpeechPort() { return speechPort; }
	int16_t getIrq() { return irq; }
	int16_t getDma() { return dma; }
};

struct AudioOptions {
	uint8_t music, speech, effects;
};

#include "../endgame/endview.h"
#include "../endgame/iff.h"
#include "../endgame/palette.h"
#include "../endgame/flic.h"
#include "../endgame/textwin.h"
#include "../endgame/sounddrv.h"
#include "../endgame/xmidi.h"
#include "../endgame/digital.h"
#include "../endgame/speech.h"

namespace Intro {

using Endgame::DrawShape;
using Endgame::FadePalette;
using Endgame::FarAllocate;
using Endgame::FindNamedChunk;
using Endgame::Flic;
using Endgame::IffFile;
using Endgame::Palette;
using Endgame::Rgb;
using Endgame::ShapeFont;
using Endgame::SoundDriver;
using Endgame::SpeechStream;
using Endgame::TextWindow;
using Endgame::VocSound;
using Endgame::XmidiSong;

}

#include "intro.h"

namespace Intro {

/* Size of the linear block the scenes are loaded into. */
#define LINEAR_SIZE     INT32_C(0xFFFFE)

#define ESC             27

#define SPEECH_BUFFER   0xC000

/* Where the Guardian's mouth is drawn over his face. */
#define MOUTH_X         124
#define MOUTH_Y         110

/* Finds the chunk of type id called name in the current form. */
#define FIND_CHUNK(file, id, name)  FindNamedChunk(&file, id, name)

/* Escape ends the program at once. */
#define CHECK_ESCAPE()                              \
	if (plat_key_available() && plat_key_get() == ESC) \
		Cleanup(1)

static ::View Screen;
static ::View *const ScreenView = &Screen;
static ::View *const CurrentView = &Screen;
AudioOptions Options;
SoundConfig Config;
int16_t FadeSpeed = 1;
IffFile DataFile;
XmidiSong *Score = 0;
SoundDriver *SpeechDriver = 0;
SoundDriver *MusicDriver = 0;
Rgb Black(0, 0, 0);
void *FontData = 0;
uint8_t Speech = 0;
ShapeFont *TextFont = 0;
TextWindow *Window = 0;
int16_t Jive = 0;
static char WorkBuffer[256];
/* The Guardian's mouth frame for each frame of his speech. */
const int16_t GuardianMouth[] = {
	1, 1, 6, 3, 7, 3, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1,
	2, 1, 7, 2, 2, 2, 5, 2, 2, 1, 1, 2, 3, 4, 5, 2, 2, 3, 5, 3,
	2, 7, 2, 3, 4, 1, 2, 7, 2, 1, 2, 6, 1, 2, 2, 4, 4, 5, 2, 2,
	6, 3, 2, 2, 3, 2, 1, 1, 1, 3, 5, 2, 1, 6, 3, 1, 2, 3, 4, 2,
	1, 4, 7, 3, 2, 4, 5, 2, 2, 3, 1, 2, 2, 7, 2, 4, 2, 2, 2, 1,
	3, 2, 1, 4, 2, 6, 3, 2, 2, 2, 3, 7, 2, 4, 2, 5, 2, 5, 2, 2,
	1, 3, 2, 3, 2, 1, 1, 1, 1, 1, 1, 5, 2, 2, 3, 2, 4, 4, 3, 2,
	3, 2, 2, 4, 2, 6, 6, 3, 2, 2, 1, 6, 1, 3, 2, 2, 3, 2, 2, 7,
	3, 2, 1, 1, 6, 2, 4, 3, 2, 2, 3, 3, 2, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

#define MOUTH_COUNT     ((int16_t) (sizeof GuardianMouth / sizeof GuardianMouth[0]))

/* Far-heap memory, freed with its scope; the drawing code reaches it by its linear address. */
struct MemHandle {
	void *memory;
	MemHandle() { memory = 0; }
	~MemHandle() { if (memory) FreeFarHeap(memory); }
	void allocate(int32_t size) { memory = FarAllocate(size, 0, "Out of memory."); }
	void *pointer() { return memory; }
	int32_t linear() { return (int32_t) PointerToLinear(memory); }
};

/* Waits ticks, or less if a key is pressed; says whether one was. */
uint8_t WaitOrKey(uint32_t ticks)
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

/* Blacks out the screen, or scrambles the palette after an error, then puts everything back and exits. */
void Cleanup(int16_t normal)
{
	FadePalette palette;

	if (normal)
		palette.fill(&Black);
	else
		for (int16_t i = 0; i < 255; i++) {
			Rgb color;
			color.red = random(63);
			color.green = random(63);
			color.blue = random(63);
			palette.writeOne(&color, i);
		}
	palette.write(0);
	if (DataFile.handle >= 0)
		DataFile.close();
	if (FontData)
		FreeFarHeap(FontData);
	if (Score)
		delete Score;
	if (SpeechDriver)
		delete SpeechDriver;
	if (MusicDriver)
		delete MusicDriver;
	while (plat_key_available())
		plat_key_get();
	plat_exit(EXIT_SELECTOR);
}

void Error(char *format, ...)
{
	if (format != WorkBuffer) {
		va_list args;
		va_start(args, format);
		vsnprintf(WorkBuffer, sizeof WorkBuffer, format, args);
	}
	Cleanup(0);
}

void Quit(char *message)
{
	Error(message);
}

void ReadError()
{
	Quit("Error reading data file.");
}

void LoadFont(View *view)
{
	FIND_CHUNK(DataFile, "FONT", "font1");
	FontData = FarAllocate(DataFile.chunk.size, 0, "Out of memory.");
	DataFile.readChunk(FontData);
	TextFont = new ShapeFont((int32_t) PointerToLinear(FontData));
	TextFont->setSpacing(-1, -1, 0);
	Window = new TextWindow(view, TextFont);
}

Yapper::Yapper(int printerOnly)
{
	data = 0;
	sound = 0;
	this->printerOnly = printerOnly;
}

Yapper::Yapper(char *name)
{
	data = 0;
	sound = 0;
	printerOnly = 0;
	load(name);
}

Yapper::~Yapper()
{
	if (!Speech)
		return;
	release();
}

void Yapper::release()
{
	if (printerOnly)
		return;
	wait(0);
	if (data)
		FreeFarHeap(data);
	if (sound)
		delete sound;
	data = 0;
	sound = 0;
}

void Yapper::load(char *name)
{
	if (!Speech)
		return;
	release();
	FIND_CHUNK(DataFile, "VOCF", name);
	data = FarAllocate(DataFile.chunk.size, 0, "Out of memory.");
	if (data) {
		DataFile.readChunk(data);
		sound = new VocSound(SpeechDriver, data);
	}
	printerOnly = 0;
}

void Yapper::play()
{
	if (Speech && sound)
		sound->play(-1);
}

/* Prints the line as a caption unless it is being spoken. */
void Yapper::say(char *text, int16_t y, int16_t justify)
{
	if (Speech && sound && !printerOnly)
		return;
	char line[256];
	snprintf(line, sizeof line, "*Y*J%s", text);
	line[0] = '%';
	line[2] = '%';
	Window->print(line, y, justify);
}

void Yapper::wait(int16_t ticks)
{
	if (printerOnly)
		Error("Cannot wait for printer-only yappers");
	if (sound) {
		while (!sound->isDone())
			plat_yield();
	} else if (ticks >= 1) {
		Timer timer;
		Timer_set(&timer, ticks * 4);
		Timer_wait(&timer);
	}
}

uint8_t Yapper::isDone()
{
	if (printerOnly)
		return 1;
	if (sound)
		return sound->isDone();
	else
		return 1;
}

static int16_t Run(int16_t argc, char **argv)
{
	while (plat_key_available())
		plat_key_get();
	if (argc < 2) {
		plat_log("Type SERPENT to play Serpent Isle.\n");
		return 1;
	}
	if (stricmp(argv[1], "hisss")) {
		plat_log("Type SERPENT to play Serpent Isle.\n");
		return 1;
	} else if (argc >= 3)
		Jive = stricmp(argv[2], "jive") == 0;

	InitVgaScreen(&Screen, 0);
	SystemTimer.install();

	uint8_t music = 0;
	uint8_t sfx = 0;
	Options.music = Options.speech = Options.effects = AUDIO_ON;
	ReadAudioOptions("gamedat\\options.cfg", &Options);
	Speech = Options.speech == AUDIO_ON;
	music = Options.music == AUDIO_ON;
	sfx = Options.effects == AUDIO_ON;

	Config.readConfig("serpent.cfg");
	if (Config.getSpeechEnabled()) {
		char *driver = 0;
		if (Config.getOptionEnabled())
			driver = "static\\sbpdig.adv";
		else
			driver = "static\\sbdig.adv";
		SpeechDriver = new SoundDriver(driver, 0, Config.getSpeechPort(), Config.getIrq(), Config.getDma(), 0, 0);
		/* No speech output: speech off, where DOS stopped with a message. */
		if (!SpeechDriver->isOpen()) {
			delete SpeechDriver;
			SpeechDriver = 0;
			Speech = 0;
		}
	} else
		Speech = 0;
	char *driver = 0;
	char *timbres = 0;
	char song[26];
	if (Config.hasMusic()) {
		/* Any music device plays the MT-32 score. */
		if (Config.isRoland() || Config.isAdlib() || Config.isSoundBlaster()) {
			driver = "static\\mt32mpu.adv";
			timbres = "static\\xmidi.mt";
			strcpy(song, "static\\r_sintro.xmi");
		} else if (Config.isAdlib() || Config.isSoundBlaster()) {
			driver = "static\\adlib.adv";
			timbres = "static\\xmidi.ad";
			strcpy(song, "static\\a_sintro.xmi");
		}
		if (driver)
			MusicDriver = new SoundDriver(driver, timbres, 0, 0, 0, 0, 1);
		/* Without an MT-32 there is no music. */
		if (MusicDriver && !MusicDriver->isOpen()) {
			delete MusicDriver;
			MusicDriver = 0;
			driver = 0;
		}
	}
	if (!MusicDriver)
		music = sfx = 0;

	Timer sceneTimer;
	Timer frameTimer;
	::View view;
	view.clip.x = 0;
	view.clip.y = 0;
	view.clip.x1 = SCREEN_WIDTH - 1;
	view.clip.y1 = SCREEN_HEIGHT - 1;
	if (!AllocateDrawBuffer(&view, 0xff, 0))
		plat_fatal("Out of memory.");
	DataFile.open("static\\intro.dat");
	DataFile.enterForm();
	LoadFont(&view);
	CHECK_ESCAPE();

	/* The Origin logo; the MT-32 sets up its drums meanwhile. */
	{
		Flic flic;
		if (!flic.load(&DataFile, "lblogo"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, FadeSpeed, 0, 256);
		Timer_set(&sceneTimer, 100);
		/* Without music DOS compared against the empty string at the start of the data segment. */
		if (driver && !strcmp(driver, "static\\mt32mpu.adv")) {
			XmidiSong *percussion = 0;
			percussion = new XmidiSong(MusicDriver, "static\\percussy.xmi", 0);
			percussion->play();
			while (!percussion->isDone())
				plat_yield();
			delete percussion;
		}
		Timer_wait(&sceneTimer);
		fade.fadeToColor(&Black, FadeSpeed, 0, 256);
	}
	CHECK_ESCAPE();

	if (music)
		Score = new XmidiSong(MusicDriver, song, 0);
	else
		Score = 0;

	/* Lord British's castle in a thunderstorm. */
	{
		Flic flic;
		if (!flic.load(&DataFile, "castle"))
			ReadError();
		Yapper thunder("thndr");
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		thunder.play();
		/* DOS then set the AdLib volume if driver == "static\\adlib.adv": two different strings'
		 * addresses, never equal. */
		if (Score && Speech == 1)
			Score->play();
		Palette palette;
		palette.load(0, 256, (Rgb *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		int16_t frames = flic.frameCount();
		int16_t frame = 0;
		int16_t count = 0;
		Timer_set(&sceneTimer, 50);
		while (!Timer_hasFinished(&sceneTimer)) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 4);
			flic.nextFrame();
			flic.show(&view);
			CopyView(&view, CurrentView);
			frame++;
			if (frame == frames) {
				frame = 0;
				flic.rewind();
			}
			Timer_wait(&frameTimer);
		}
		flic.rewind();
		int16_t flash = 0;
		Timer_set(&sceneTimer, 250);
		while (!Timer_hasFinished(&sceneTimer)) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 4);
			flic.rewind();
			flic.nextFrame();
			flash = random(15);
			if (flash == 1)
				flic.nextFrame();
			if (flash == 2) {
				flic.nextFrame();
				flic.nextFrame();
			}
			flic.show(&view);
			Yapper caption(0);
			if (Jive)
				caption.say("Dick British's Castle", 140, JUSTIFY_CENTER);
			else
				caption.say("Lord British's Castle", 140, JUSTIFY_CENTER);
			CopyView(&view, CurrentView);
			Timer_wait(&frameTimer);
		}
		thunder.play();
		count = 0;
		Timer_set(&sceneTimer, 300);
		while (!Timer_hasFinished(&sceneTimer)) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 4);
			flic.rewind();
			flic.nextFrame();
			flash = random(15);
			if (flash == 1)
				flic.nextFrame();
			if (flash == 2) {
				flic.nextFrame();
				flic.nextFrame();
			}
			flic.show(&view);
			count++;
			if (count > 10) {
				Yapper caption(0);
				caption.say("Eighteen months after the destruction", 140, JUSTIFY_CENTER);
				caption.say("of the Black Gate and the", 155, JUSTIFY_CENTER);
				caption.say("dismantling of The Fellowship", 170, JUSTIFY_CENTER);
			}
			CopyView(&view, CurrentView);
			Timer_wait(&frameTimer);
		}
		flic.rewind();
		flic.nextFrame();
		flic.show(&view);
		CopyView(&view, CurrentView);
		fade.fadeToColor(&Black, FadeSpeed, 0, 256);
		FillView(ScreenView, 0);
	}
	CHECK_ESCAPE();

	/* As above, the AdLib volume was never set. */
	if (Score && Speech == 0)
		Score->play();

	/* The guards bring Lord British what they found among Batlin's belongings. */
	{
		Flic flic;
		if (!flic.load(&DataFile, "sanctum1"))
			ReadError();
		Yapper guard(0);
		Yapper captain(0);
		Yapper lordBritish(0);
		guard.load("guard1");
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, FadeSpeed, 0, 256);
		fade = palette;
		int16_t frames = flic.frameCount();
		int16_t i = 0;
		for (i = 1; i < 35; i++) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 3);
			flic.nextFrame();
			flic.show(&view);
			CopyView(&view, CurrentView);
			Timer_wait(&frameTimer);
		}
		guard.play();
		for (i = 35; i < 60; i++) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 4);
			flic.nextFrame();
			flic.show(&view);
			if (Jive)
				guard.say("        Yo, homes", 185, JUSTIFY_CENTER);
			else
				guard.say("        My leige", 185, JUSTIFY_CENTER);
			CopyView(&view, CurrentView);
			Timer_wait(&frameTimer);
		}
		flic.show(&view);
		CopyView(&view, CurrentView);
		guard.wait(3);
		guard.release();
		captain.load("guard2");
		captain.play();
		for (i = 60; i < 76; i++) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 4);
			flic.nextFrame();
			flic.show(&view);
			captain.say("     All we found among Batlin's", 174, JUSTIFY_CENTER);
			captain.say("   belongings was this enchanted scroll...", 187, JUSTIFY_CENTER);
			CopyView(&view, CurrentView);
			Timer_wait(&frameTimer);
		}
		captain.wait(90);
		flic.show(&view);
		CopyView(&view, CurrentView);
		captain.wait(3);
		flic.show(&view);
		captain.say("     and a map showing the way to", 174, JUSTIFY_CENTER);
		captain.say("    a place called the Serpent Isle.", 187, JUSTIFY_CENTER);
		CopyView(&view, CurrentView);
		captain.wait(116);
		captain.release();
		flic.show(&view);
		CopyView(&view, CurrentView);
		captain.wait(3);
		lordBritish.load("lb1");
		lordBritish.play();
		for (i = 77; i < 81; i++) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 4);
			flic.nextFrame();
			flic.show(&view);
			if (Jive)
				lordBritish.say("Iree. Slap it down there!       ", 185, JUSTIFY_CENTER);
			else
				lordBritish.say("Indeed.                 ", 174, JUSTIFY_CENTER);
			lordBritish.say("Put it on the table.       ", 187, JUSTIFY_CENTER);
			CopyView(&view, CurrentView);
			Timer_wait(&frameTimer);
		}
		lordBritish.wait(66);
		fade.fadeToColor(&Black, 0, 0, 256);
		FillView(ScreenView, 0);
	}

	/* Lord British opens the scroll. */
	{
		Flic flic;
		if (!flic.load(&DataFile, "sanctum2"))
			ReadError();
		Yapper lordBritish("lb2");
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		fade = palette;
		int16_t frames = flic.frameCount();
		int16_t i = 0;
		for (i = 1; i < 21; i++) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 4);
			flic.nextFrame();
			flic.show(&view);
			CopyView(&view, CurrentView);
			Timer_wait(&frameTimer);
		}
		lordBritish.play();
		for (i = 21; i < frames; i++) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 4);
			flic.nextFrame();
			flic.show(&view);
			if (Jive)
				lordBritish.say("Jump back!     ", 160, JUSTIFY_CENTER);
			else
				lordBritish.say("Stand back!     ", 160, JUSTIFY_CENTER);
			CopyView(&view, CurrentView);
			Timer_wait(&frameTimer);
		}
		lordBritish.wait(71 - frames);
		fade.fadeToColor(&Black, 0, 0, 256);
		FillView(ScreenView, 0);
	}

	/* The Guardian's message to Batlin, mouthed in time with his speech. */
	{
		Flic flic;
		if (!flic.load(&DataFile, "sanctum3"))
			ReadError();
		Yapper guardian(1);
		FIND_CHUNK(DataFile, "SHAP", "guardian");
		MemHandle mouths;
		mouths.allocate(DataFile.chunk.size);
		DataFile.readChunk(mouths.pointer());
		SpeechStream *speaker = 0;
		uint8_t *voice = 0;
		if (Speech) {
			FIND_CHUNK(DataFile, "VOCF", "gdian");
			voice = new uint8_t[DataFile.chunk.size];
			int32_t length = DataFile.readChunk(voice);
			speaker = new SpeechStream(SpeechDriver, voice, length, SPEECH_BUFFER);
		} else
			;
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		int16_t frames = flic.frameCount();
		int16_t i = 1;
		int16_t mouth = 1;
		if (speaker)
			speaker->play(-1);
		if (Speech) {
			while (!speaker->isDone()) {
				CHECK_ESCAPE();
				Timer_set(&frameTimer, 5);
				mouth++;
				speaker->service();
				flic.nextFrame();
				flic.show(&view);
				/* Past the table INTRO.EXE holds "Error reading data file." and other strings, whose
				 * words name no frame of the shape: DrawFrame drew nothing. */
				if (mouth < MOUTH_COUNT)
					DrawShape(&view, MOUTH_X, MOUTH_Y, mouths.linear(), GuardianMouth[mouth] - 1);
				CopyView(&view, CurrentView);
				i++;
				if (i == frames) {
					i = 0;
					flic.rewind();
				}
				Timer_wait(&frameTimer);
			}
		} else {
			for (int16_t count = 0; count < 255; count++) {
				CHECK_ESCAPE();
				Timer_set(&frameTimer, 4);
				flic.nextFrame();
				flic.show(&view);
				DrawShape(&view, MOUTH_X, MOUTH_Y, mouths.linear(), random(6) + 1);
				if (count < 85) {
					if (Jive) {
						guardian.say("Batlin!  Know that my face is most", 174, JUSTIFY_CENTER);
						guardian.say("muppet like!", 187, JUSTIFY_CENTER);
					} else {
						guardian.say("Batlin!  In the event that the", 174, JUSTIFY_CENTER);
						guardian.say("Avatar destroys the Black Gate", 187, JUSTIFY_CENTER);
					}
				} else if (count < 170) {
					if (Jive) {
						guardian.say("You must go to the Serpent Isle, there", 174, JUSTIFY_CENTER);
						guardian.say("to learn the secret of Acne Medication", 187, JUSTIFY_CENTER);
					} else {
						guardian.say("you shall follow the unwitting", 174, JUSTIFY_CENTER);
						guardian.say("human Gwenno to the Serpent Isle", 187, JUSTIFY_CENTER);
					}
				} else {
					if (Jive) {
						guardian.say("Soon I and my horde of muppets will", 174, JUSTIFY_CENTER);
						guardian.say("destroy Britannia!", 187, JUSTIFY_CENTER);
					} else {
						guardian.say("There I shall outline my plan", 174, JUSTIFY_CENTER);
						guardian.say("to destroy Britannia!", 187, JUSTIFY_CENTER);
					}
				}
				CopyView(&view, CurrentView);
				i++;
				if (i == frames) {
					i = 0;
					flic.rewind();
				}
				Timer_wait(&frameTimer);
			}
		}
		fade.fadeToColor(&Black, FadeSpeed, 0, 256);
		FillView(ScreenView, 0);
		delete speaker;
		delete[] voice;
	}

	/* Lord British at the ship. */
	{
		Yapper lordBritish("lb3");
		lordBritish.play();
		Flic flic;
		if (!flic.load(&DataFile, "ship1"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 1, 0, 256);
		fade = palette;
		int16_t frames = flic.frameCount();
		int16_t i = 0;
		for (i = 1; i < frames; i++) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 7);
			flic.nextFrame();
			flic.show(&view);
			if (i < 20)
				lordBritish.say("'Tis my worst fear!", 170, JUSTIFY_CENTER);
			else if (i > 23) {
				lordBritish.say("I must send the Avatar through", 170, JUSTIFY_CENTER);
				lordBritish.say("the pillars to the Serpent Isle", 184, JUSTIFY_CENTER);
			}
			CopyView(&view, CurrentView);
			Timer_wait(&frameTimer);
		}
		fade.fadeToColor(&Black, 0, 0, 256);
		FillView(ScreenView, 0);
	}
	CHECK_ESCAPE();

	/* The ship sets sail. */
	{
		Flic flic;
		if (!flic.load(&DataFile, "ship2"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		fade = palette;
		int16_t frames = flic.frameCount();
		int16_t i = 0;
		for (i = 1; i < frames; i++) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 5);
			flic.nextFrame();
			flic.show(&view);
			CopyView(&view, CurrentView);
			Timer_wait(&frameTimer);
		}
		fade.fadeToColor(&Black, 0, 0, 256);
		FillView(ScreenView, 0);
	}
	CHECK_ESCAPE();

	/* Through the pillars. */
	{
		Flic flic;
		Timer_set(&frameTimer, 200);
		if (!flic.load(&DataFile, "pil1"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		fade = palette;
		int16_t frames = flic.frameCount();
		int16_t i = 0;
		for (i = 1; i < frames; i++) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 4);
			flic.nextFrame();
			flic.show(&view);
			if (Jive && i > frames - 10) {
				Yapper caption(1);
				caption.say("Zot!", 174, JUSTIFY_CENTER);
			}
			CopyView(&view, CurrentView);
			Timer_wait(&frameTimer);
		}
		fade.fadeToColor(&Black, 0, 0, 256);
		FillView(ScreenView, 0);
	}
	CHECK_ESCAPE();

	/* Without music there is no score; DOS read a null driver here and did nothing. */
	if (Score)
		Score->setVolume(0, 4000);

	/* The Serpent Isle title. */
	{
		Flic flic;
		if (!flic.load(&DataFile, "u72_logo"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 1, 0, 256);
		fade = palette;
		Timer_set(&frameTimer, 200);
		Timer_wait(&frameTimer);
		fade.fadeToColor(&Black, 1, 0, 256);
		FillView(ScreenView, 0);
	}
	CHECK_ESCAPE();
	Cleanup(1);
	return EXIT_SELECTOR;
}

}

extern "C" int16_t IntroMain(int16_t argc, char **argv)
{
	return Intro::Run(argc, argv);
}

extern "C" void ResetIntroIntroGlobals(void)
{
	memset((void *) &Intro::Screen, 0, sizeof Intro::Screen);
	memset(&Intro::Options, 0, sizeof Intro::Options);
	memset((void *) &Intro::Config, 0, sizeof Intro::Config);
	memset((void *) &Intro::DataFile, 0, sizeof Intro::DataFile);
	Intro::Score = 0;
	Intro::SpeechDriver = 0;
	Intro::MusicDriver = 0;
	memset((void *) &Intro::Black, 0, sizeof Intro::Black);
	Intro::FontData = 0;
	Intro::Speech = 0;
	Intro::TextFont = 0;
	Intro::Window = 0;
	Intro::Jive = 0;
	Intro::FadeSpeed = 1;
	memset(Intro::WorkBuffer, 0, sizeof Intro::WorkBuffer);
}

extern "C" void ConstructIntroIntroGlobals(void)
{
	new (&Intro::Screen) ::View();
	new (&Intro::Config) SoundConfig();
	new (&Intro::DataFile) Intro::IffFile();
	new (&Intro::Black) Intro::Rgb(0, 0, 0);
}
