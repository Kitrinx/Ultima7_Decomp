/* Serpent Isle ENDGAME.EXE, resident segment 1 (file offsets 0x005d8b to 0x0082ae, 9507 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 * File name inferred: Black Gate ENDGAME.EXE's main module, which this one closely follows.
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

#include "endview.h"
#include "iff.h"
#include "palette.h"
#include "flic.h"
#include "textwin.h"
#include "sounddrv.h"
#include "xmidi.h"
#include "digital.h"
#include "speech.h"
#include "endgame.h"

namespace Endgame {

/* Size of the linear block the scenes are loaded into. */
#define LINEAR_SIZE     INT32_C(0xFFFFE)

#define ESC             27

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
			strcpy(song, "static\\r_send.xmi");
		} else if (Config.isAdlib() || Config.isSoundBlaster()) {
			driver = "static\\adlib.adv";
			timbres = "static\\xmidi.ad";
			strcpy(song, "static\\a_send.xmi");
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

	Timer frameTimer;
	Timer sceneTimer;
	Timer scoreTimer;
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

	/* The MT-32 sets up its drums first. Without music DOS compared against the empty string at
	 * the start of the data segment. */
	if (driver && !strcmp(driver, "static\\mt32mpu.adv")) {
		XmidiSong *percussion = 0;
		percussion = new XmidiSong(MusicDriver, "static\\percussy.xmi", 0);
		percussion->play();
		while (!percussion->isDone())
			plat_yield();
		delete percussion;
	}
	CHECK_ESCAPE();

	if (music)
		Score = new XmidiSong(MusicDriver, song, 0);
	else
		Score = 0;
	Timer_set(&scoreTimer, 2900);
	if (Score)
		Score->play();

	/* The Avatar floating. */
	{
		Flic flic;
		if (!flic.load(&DataFile, "avfloat"))
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

	/* The Great Earth Serpent: balance restored. */
	{
		Yapper voice("win1");
		Flic flic;
		if (!flic.load(&DataFile, "snake1"))
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
		while (i < 99) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 4);
			flic.nextFrame();
			flic.show(&view);
			CopyView(&view, CurrentView);
			i++;
			Timer_wait(&frameTimer);
		}
		voice.play();
		while (i < frames - 1) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 6);
			flic.nextFrame();
			flic.show(&view);
			if (i > 110) {
				voice.say("There, we are done.", 176, JUSTIFY_CENTER);
				voice.say("Balance is restored.", 188, JUSTIFY_CENTER);
			}
			CopyView(&view, CurrentView);
			i++;
			Timer_wait(&frameTimer);
		}
		fade.fadeToColor(&Black, 0, 0, 256);
		FillView(ScreenView, 0);
	}

	/* The Avatar floating again: the worlds are saved. */
	{
		Yapper voice("win2");
		Flic flic;
		if (!flic.load(&DataFile, "avfloat"))
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
		int16_t count = 0;
		Timer firstLines;
		Timer scene;
		Timer_set(&firstLines, 500);
		Timer_set(&scene, 1100);
		voice.play();
		while (!Timer_hasFinished(&scene)) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 5);
			flic.nextFrame();
			flic.show(&view);
			if (!Timer_hasFinished(&firstLines)) {
				voice.say("Serpent Isle, Britannia, your Earth,", 174, JUSTIFY_CENTER);
				voice.say("the entire universe, all are saved.", 187, JUSTIFY_CENTER);
			} else {
				voice.say("Worry not about your friend Dupre.", 174, JUSTIFY_CENTER);
				voice.say("He is one with us, and content.", 187, JUSTIFY_CENTER);
			}
			CopyView(&view, CurrentView);
			count++;
			i++;
			if (i == frames) {
				i = 0;
				flic.rewind();
				flic.nextFrame();
			}
			Timer_wait(&frameTimer);
		}
		fade.fadeToColor(&Black, 0, 0, 256);
		FillView(ScreenView, 0);
	}
	CHECK_ESCAPE();

	/* The Serpent's farewell, held until the music and the voice are done. */
	{
		Yapper voice("win3");
		Flic flic;
		if (!flic.load(&DataFile, "snake2"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		fade = palette;
		int16_t frames = flic.frameCount();
		int16_t i = 1;
		voice.play();
		while (!Timer_hasFinished(&scoreTimer) || !voice.isDone()) {
			Timer_set(&frameTimer, 4);
			flic.nextFrame();
			flic.show(&view);
			voice.say("Goodbye, Avatar.", 174, JUSTIFY_CENTER);
			voice.say("We thank you.", 187, JUSTIFY_CENTER);
			CopyView(&view, CurrentView);
			i++;
			if (i == frames) {
				i = 0;
				flic.rewind();
			}
			Timer_wait(&frameTimer);
		}
		fade.fadeToColor(&Black, 0, 0, 256);
		FillView(ScreenView, 0);
	}
	Timer_set(&scoreTimer, 1300);

	/* The Guardian's taunt. */
	{
		Yapper taunt("win4");
		Yapper recap("win5");
		Flic flic;
		if (!flic.load(&DataFile, "avgrab"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		fade = palette;
		int16_t frames = flic.frameCount();
		int16_t i = 1;
		taunt.play();
		Timer_set(&sceneTimer, 420);
		while (!Timer_hasFinished(&sceneTimer)) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 4);
			flic.nextFrame();
			flic.show(&view);
			taunt.say("Well well well, Avatar.  You ", 174, JUSTIFY_CENTER);
			taunt.say("have managed to thwart me once again.", 187, JUSTIFY_CENTER);
			CopyView(&view, CurrentView);
			i++;
			if (i == 60) {
				i = 0;
				flic.rewind();
			}
			Timer_wait(&frameTimer);
		}
		recap.play();
		Timer_set(&sceneTimer, 400);
		while (!Timer_hasFinished(&sceneTimer)) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 4);
			flic.nextFrame();
			flic.show(&view);
			taunt.say("By restoring balance where", 161, JUSTIFY_CENTER);
			taunt.say("once chaos reigned, you have", 174, JUSTIFY_CENTER);
			taunt.say("saved your accursed world.", 187, JUSTIFY_CENTER);
			CopyView(&view, CurrentView);
			i++;
			if (i == 60) {
				i = 0;
				flic.rewind();
			}
			Timer_wait(&frameTimer);
		}
		fade.fadeToColor(&Black, 0, 0, 256);
		FillView(ScreenView, 0);
	}

	/* The Avatar at the edge of Eternity. */
	{
		Yapper voice("win6");
		voice.play();
		Flic flic;
		if (!flic.load(&DataFile, "avfar"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		fade = palette;
		int16_t frames = flic.frameCount();
		int16_t i = 1;
		while (!Timer_hasFinished(&scoreTimer) || !voice.isDone()) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 4);
			flic.nextFrame();
			flic.show(&view);
			voice.say("But now here you are, poised", 161, JUSTIFY_CENTER);
			voice.say("at the edge of Eternity.", 174, JUSTIFY_CENTER);
			voice.say("Where would you go?", 187, JUSTIFY_CENTER);
			CopyView(&view, CurrentView);
			i++;
			if (i == frames) {
				i = 0;
				flic.rewind();
			}
			Timer_wait(&frameTimer);
		}
		fade.fadeToColor(&Black, 0, 0, 256);
		FillView(ScreenView, 0);
	}

	/* The Guardian reaches for the Avatar. */
	{
		Yapper question("win7");
		Yapper offer("win8");
		Flic flic;
		if (!flic.load(&DataFile, "avgrab"))
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
		question.play();
		for (i = 1; i < frames; i++) {
			CHECK_ESCAPE();
			Timer_set(&frameTimer, 4);
			flic.nextFrame();
			flic.show(&view);
			if (i < 50) {
				question.say("Back to Britannia?", 174, JUSTIFY_CENTER);
				question.say("To Earth?", 187, JUSTIFY_CENTER);
			}
			if (i == 55)
				offer.play();
			if (i > 55) {
				question.say("Perhaps you would join me in", 161, JUSTIFY_CENTER);
				question.say("another world alltogether?", 174, JUSTIFY_CENTER);
				question.say("We do have a score to settle!", 187, JUSTIFY_CENTER);
			}
			CopyView(&view, CurrentView);
			Timer_wait(&frameTimer);
		}
		Timer_set(&frameTimer, 60);
		Timer_wait(&frameTimer);
		flic.show(&view);
		CopyView(&view, CurrentView);
		Timer_set(&frameTimer, 40);
		Timer_wait(&frameTimer);
		fade.fadeToColor(&Black, 5, 0, 256);
		FillView(ScreenView, 0);
	}
	Cleanup(1);
	return EXIT_SELECTOR;
}

}

extern "C" int16_t EndgameMain(int16_t argc, char **argv)
{
	return Endgame::Run(argc, argv);
}

extern "C" void ResetEndgameEndgameGlobals(void)
{
	memset((void *) &Endgame::Screen, 0, sizeof Endgame::Screen);
	memset(&Endgame::Options, 0, sizeof Endgame::Options);
	memset((void *) &Endgame::Config, 0, sizeof Endgame::Config);
	memset((void *) &Endgame::DataFile, 0, sizeof Endgame::DataFile);
	Endgame::Score = 0;
	Endgame::SpeechDriver = 0;
	Endgame::MusicDriver = 0;
	memset((void *) &Endgame::Black, 0, sizeof Endgame::Black);
	Endgame::FontData = 0;
	Endgame::Speech = 0;
	Endgame::TextFont = 0;
	Endgame::Window = 0;
	Endgame::Jive = 0;
	memset(Endgame::WorkBuffer, 0, sizeof Endgame::WorkBuffer);
}

extern "C" void ConstructEndgameEndgameGlobals(void)
{
	new (&Endgame::Screen) ::View();
	new (&Endgame::Config) SoundConfig();
	new (&Endgame::DataFile) Endgame::IffFile();
	new (&Endgame::Black) Endgame::Rgb(0, 0, 0);
}
