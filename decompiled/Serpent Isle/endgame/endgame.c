/* Serpent Isle ENDGAME.EXE, resident segment 1 (file offsets 0x005d8b to 0x0082ae, 9507 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 * File name inferred: Black Gate ENDGAME.EXE's main module, which this one closely follows.
 */

#include <alloc.h>
#include <conio.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "display.h"
#include "vooalloc.h"
#include "systimer.h"
#include "options.h"
#include "gsound.h"
#include "memsys.h"
#include "strbuf.h"
#include "errors.h"
#include "view.h"
#include "palette.h"
#include "iff.h"
#include "chunknam.h"
#include "font.h"
#include "flic.h"
#include "xmidi.h"
#include "digital.h"
#include "textwin.h"
#include "endgame.h"

/* Size of the linear block the scenes are loaded into. */
#define LINEAR_SIZE     0xFFFFEL

#define ESC             27

/* Finds the chunk of type id called name in the current form. */
#define FIND_CHUNK(file, id, name)                  \
	file.findChunk(id);                             \
	while (file.isId(id)) {                         \
		ChunkName chunkName(&file);                 \
		if (strcmp(chunkName, name)) {              \
			file.skipChunk();                       \
			file.readHeader();                      \
		} else                                      \
			break;                                  \
	}

/* Escape ends the program at once. */
#define CHECK_ESCAPE()                              \
	if (kbhit() && getch() == ESC)                  \
		Cleanup(1)

VoodooBlock VoodooXmsBlock;
AudioOptions Options;
SoundConfig Config;
IffFile DataFile;
XmidiSong *Score = 0;
SoundDriver *SpeechDriver = 0;
SoundDriver *MusicDriver = 0;
Rgb Black(0, 0, 0);
void far *FontData = 0;
unsigned char Speech = 0;
MemHandle FontHandle;
ShapeFont *TextFont = 0;
TextWindow *Window = 0;
int Jive = 0;

/* Waits ticks, or less if a key is pressed; says whether one was. */
unsigned char WaitOrKey(unsigned long ticks)
{
	Timer timer;

	timer.set(ticks);
	while (!timer.hasFinished() && !kbhit())
		;
	if (kbhit()) {
		getch();
		return 1;
	}
	return 0;
}

/* Blacks out the screen, or scrambles the palette after an error, then puts everything back and exits. */
void Cleanup(int normal)
{
	FadePalette palette;

	if (normal)
		palette.fill(&Black);
	else
		for (int i = 0; i < 255; i++) {
			Rgb color;
			color.red = random(63);
			color.green = random(63);
			color.blue = random(63);
			palette.writeOne(&color, i);
		}
	palette.write(0);
	if (DataFile.isOpen())
		DataFile.close();
	if (FontData)
		delete FontData;
	if (Score)
		delete Score;
	if (SpeechDriver)
		delete SpeechDriver;
	if (MusicDriver)
		delete MusicDriver;
	VoodooXmsBlock.close();
	Screen.close(0);
	while (kbhit())
		getch();
	exit(EXIT_SELECTOR);
}

void Error(char *format, ...)
{
	if (format != WorkBuffer) {
		va_list args;
		va_start(args, format);
		vsprintf(WorkBuffer, format, args);
	}
	Cleanup(0);
}

void Quit(char *message)
{
	ShutdownXMM();
	Error(message);
}

void ReadError()
{
	Quit("Error reading data file.");
}

void CheckHeap(char *file, int line)
{
	char where[40];

	sprintf(where, "%s @%d", file, line);
	if (heapcheck() < 0)
		FatalError("Heap failure: %s", where);
	if (*(long *)0)
		FatalError("Near ptr asn: %s", where);
}

/* Beeps a message, checks the heap, then waits for a key. */
void Checkpoint(char *file, int line, char *message)
{
	printf("\a%s\a\n", message);
	CheckHeap(file, line);
	while (kbhit())
		getch();
	getch();
}

void LoadFont(View *view)
{
	FIND_CHUNK(DataFile, "FONT", "font1");
	FontData = Memory.allocate(DataFile.chunk.size, FAR_MEMORY, 0, 1);
	DataFile.readChunk(FontData);
	FontHandle.set(FontData, FAR_MEMORY, 1);
	TextFont = new ShapeFont(FontHandle);
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
		Memory.release(&data, FAR_MEMORY);
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
	data = Memory.allocate(DataFile.chunk.size, FAR_MEMORY, 0, 1);
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
void Yapper::say(char *text, int y, int justify)
{
	if (Speech && sound && !printerOnly)
		return;
	char line[256];
	sprintf(line, "*Y*J%s", text);
	line[0] = '%';
	line[2] = '%';
	Window->print(line, y, justify);
}

void Yapper::wait(int ticks)
{
	if (printerOnly)
		Error("Cannot wait for printer-only yappers");
	if (sound) {
		while (!sound->isDone())
			;
	} else if (ticks >= 1) {
		Timer timer;
		timer.set(ticks * 4);
		Timer_wait(&timer);
	}
}

unsigned char Yapper::isDone()
{
	if (printerOnly)
		return 1;
	if (sound)
		return sound->isDone();
	else
		return 1;
}

int main(int argc, char *argv[])
{
	while (kbhit())
		getch();
	if (argc < 2) {
		printf("Type SERPENT to play Serpent Isle.\n");
		return 1;
	}
	if (stricmp(argv[1], "hisss")) {
		printf("Type SERPENT to play Serpent Isle.\n");
		return 1;
	} else if (argc >= 3)
		Jive = stricmp(argv[2], "jive") == 0;

	Screen.open();
	SystemTimer.install();
	VoodooXmsBlock.open();
	long voodoo = AllocateVoodooMemory(&VoodooXmsBlock, LINEAR_SIZE);

	unsigned char music = 0;
	unsigned char sfx = 0;
	Options.load("gamedat\\options.cfg");
	Speech = Options.speechOn();
	music = Options.musicOn();
	sfx = Options.sfxOn();

	Config.readConfig("serpent.cfg");
	if (Config.getSpeechEnabled()) {
		char *driver = 0;
		if (Config.getOptionEnabled())
			driver = "static\\sbpdig.adv";
		else
			driver = "static\\sbdig.adv";
		SpeechDriver = new SoundDriver(driver, 0, Config.getSpeechPort(), Config.getIrq(), Config.getDma(), 0, 0);
	} else
		Speech = 0;
	char *driver = 0;
	char *timbres = 0;
	char song[26];
	if (Config.hasMusic()) {
		if (Config.isRoland()) {
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
	}
	if (!MusicDriver)
		music = sfx = 0;

	Timer frameTimer;
	Timer sceneTimer;
	Timer scoreTimer;
	View view(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	DataFile.open("static\\intro.dat", FILE_READ);
	DataFile.enterForm();
	LoadFont(&view);

	/* The MT-32 sets up its drums first. */
	if (!strcmp(driver, "static\\mt32mpu.adv")) {
		XmidiSong *percussion = 0;
		percussion = new XmidiSong(MusicDriver, "static\\percussy.xmi", 0);
		percussion->play();
		while (!percussion->isDone())
			;
		delete percussion;
	}
	CHECK_ESCAPE();

	if (music)
		Score = new XmidiSong(MusicDriver, song, 0);
	else
		Score = 0;
	scoreTimer.set(2900);
	if (Score)
		Score->play();

	/* The Avatar floating. */
	{
		Flic flic(&voodoo);
		if (!flic.load(&DataFile, "avfloat"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb far *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		fade = palette;
		int frames = flic.frameCount();
		int i = 0;
		for (i = 1; i < frames; i++) {
			CHECK_ESCAPE();
			frameTimer.set(5);
			flic.nextFrame();
			flic.show(&view);
			view.copyTo(CurrentView);
			Timer_wait(&frameTimer);
		}
		fade.fadeToColor(&Black, 0, 0, 256);
		ScreenView->clear(0);
	}
	CHECK_ESCAPE();

	/* The Great Earth Serpent: balance restored. */
	{
		Yapper voice("win1");
		Flic flic(&voodoo);
		if (!flic.load(&DataFile, "snake1"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb far *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		fade = palette;
		int frames = flic.frameCount();
		int i = 0;
		while (i < 99) {
			CHECK_ESCAPE();
			frameTimer.set(4);
			flic.nextFrame();
			flic.show(&view);
			view.copyTo(CurrentView);
			i++;
			Timer_wait(&frameTimer);
		}
		voice.play();
		while (i < frames - 1) {
			CHECK_ESCAPE();
			frameTimer.set(6);
			flic.nextFrame();
			flic.show(&view);
			if (i > 110) {
				voice.say("There, we are done.", 176, JUSTIFY_CENTER);
				voice.say("Balance is restored.", 188, JUSTIFY_CENTER);
			}
			view.copyTo(CurrentView);
			i++;
			Timer_wait(&frameTimer);
		}
		fade.fadeToColor(&Black, 0, 0, 256);
		ScreenView->clear(0);
	}

	/* The Avatar floating again: the worlds are saved. */
	{
		Yapper voice("win2");
		Flic flic(&voodoo);
		if (!flic.load(&DataFile, "avfloat"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb far *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		fade = palette;
		int frames = flic.frameCount();
		int i = 0;
		int count = 0;
		Timer firstLines;
		Timer scene;
		firstLines.set(500);
		scene.set(1100);
		voice.play();
		while (!scene.hasFinished()) {
			CHECK_ESCAPE();
			frameTimer.set(5);
			flic.nextFrame();
			flic.show(&view);
			if (!firstLines.hasFinished()) {
				voice.say("Serpent Isle, Britannia, your Earth,", 174, JUSTIFY_CENTER);
				voice.say("the entire universe, all are saved.", 187, JUSTIFY_CENTER);
			} else {
				voice.say("Worry not about your friend Dupre.", 174, JUSTIFY_CENTER);
				voice.say("He is one with us, and content.", 187, JUSTIFY_CENTER);
			}
			view.copyTo(CurrentView);
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
		ScreenView->clear(0);
	}
	CHECK_ESCAPE();

	/* The Serpent's farewell, held until the music and the voice are done. */
	{
		Yapper voice("win3");
		Flic flic(&voodoo);
		if (!flic.load(&DataFile, "snake2"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb far *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		fade = palette;
		int frames = flic.frameCount();
		int i = 1;
		voice.play();
		while (!scoreTimer.hasFinished() || !voice.isDone()) {
			frameTimer.set(4);
			flic.nextFrame();
			flic.show(&view);
			voice.say("Goodbye, Avatar.", 174, JUSTIFY_CENTER);
			voice.say("We thank you.", 187, JUSTIFY_CENTER);
			view.copyTo(CurrentView);
			i++;
			if (i == frames) {
				i = 0;
				flic.rewind();
			}
			Timer_wait(&frameTimer);
		}
		fade.fadeToColor(&Black, 0, 0, 256);
		ScreenView->clear(0);
	}
	scoreTimer.set(1300);

	/* The Guardian's taunt. */
	{
		Yapper taunt("win4");
		Yapper recap("win5");
		Flic flic(&voodoo);
		if (!flic.load(&DataFile, "avgrab"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb far *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		fade = palette;
		int frames = flic.frameCount();
		int i = 1;
		taunt.play();
		sceneTimer.set(420);
		while (!sceneTimer.hasFinished()) {
			CHECK_ESCAPE();
			frameTimer.set(4);
			flic.nextFrame();
			flic.show(&view);
			taunt.say("Well well well, Avatar.  You ", 174, JUSTIFY_CENTER);
			taunt.say("have managed to thwart me once again.", 187, JUSTIFY_CENTER);
			view.copyTo(CurrentView);
			i++;
			if (i == 60) {
				i = 0;
				flic.rewind();
			}
			Timer_wait(&frameTimer);
		}
		recap.play();
		sceneTimer.set(400);
		while (!sceneTimer.hasFinished()) {
			CHECK_ESCAPE();
			frameTimer.set(4);
			flic.nextFrame();
			flic.show(&view);
			taunt.say("By restoring balance where", 161, JUSTIFY_CENTER);
			taunt.say("once chaos reigned, you have", 174, JUSTIFY_CENTER);
			taunt.say("saved your accursed world.", 187, JUSTIFY_CENTER);
			view.copyTo(CurrentView);
			i++;
			if (i == 60) {
				i = 0;
				flic.rewind();
			}
			Timer_wait(&frameTimer);
		}
		fade.fadeToColor(&Black, 0, 0, 256);
		ScreenView->clear(0);
	}

	/* The Avatar at the edge of Eternity. */
	{
		Yapper voice("win6");
		voice.play();
		Flic flic(&voodoo);
		if (!flic.load(&DataFile, "avfar"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb far *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		fade = palette;
		int frames = flic.frameCount();
		int i = 1;
		while (!scoreTimer.hasFinished() || !voice.isDone()) {
			CHECK_ESCAPE();
			frameTimer.set(4);
			flic.nextFrame();
			flic.show(&view);
			voice.say("But now here you are, poised", 161, JUSTIFY_CENTER);
			voice.say("at the edge of Eternity.", 174, JUSTIFY_CENTER);
			voice.say("Where would you go?", 187, JUSTIFY_CENTER);
			view.copyTo(CurrentView);
			i++;
			if (i == frames) {
				i = 0;
				flic.rewind();
			}
			Timer_wait(&frameTimer);
		}
		fade.fadeToColor(&Black, 0, 0, 256);
		ScreenView->clear(0);
	}

	/* The Guardian reaches for the Avatar. */
	{
		Yapper question("win7");
		Yapper offer("win8");
		Flic flic(&voodoo);
		if (!flic.load(&DataFile, "avgrab"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb far *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		fade = palette;
		int frames = flic.frameCount();
		int i = 0;
		question.play();
		for (i = 1; i < frames; i++) {
			CHECK_ESCAPE();
			frameTimer.set(4);
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
			view.copyTo(CurrentView);
			Timer_wait(&frameTimer);
		}
		frameTimer.set(60);
		Timer_wait(&frameTimer);
		flic.show(&view);
		view.copyTo(CurrentView);
		frameTimer.set(40);
		Timer_wait(&frameTimer);
		fade.fadeToColor(&Black, 5, 0, 256);
		ScreenView->clear(0);
	}
	Cleanup(1);
}
