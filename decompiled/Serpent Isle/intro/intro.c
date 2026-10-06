/* Serpent Isle INTRO.EXE, resident segment 1 (file offsets 0x005f8b to 0x008f7f, 12276 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 * File name inferred: Black Gate INTRO.EXE's main module, which this one closely follows.
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
#include "speech.h"
#include "intro.h"

/* Size of the linear block the scenes are loaded into. */
#define LINEAR_SIZE     0xFFFFEL

#define ESC             27

#define SPEECH_BUFFER   0xC000

/* Where the Guardian's mouth is drawn over his face. */
#define MOUTH_X         124
#define MOUTH_Y         110

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
int FadeSpeed = 1;
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
/* The Guardian's mouth frame for each frame of his speech. */
int GuardianMouth[] = {
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
			strcpy(song, "static\\r_sintro.xmi");
		} else if (Config.isAdlib() || Config.isSoundBlaster()) {
			driver = "static\\adlib.adv";
			timbres = "static\\xmidi.ad";
			strcpy(song, "static\\a_sintro.xmi");
		}
		if (driver)
			MusicDriver = new SoundDriver(driver, timbres, 0, 0, 0, 0, 1);
	}
	if (!MusicDriver)
		music = sfx = 0;

	Timer sceneTimer;
	Timer frameTimer;
	View view(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	DataFile.open("static\\intro.dat", FILE_READ);
	DataFile.enterForm();
	LoadFont(&view);
	CHECK_ESCAPE();

	/* The Origin logo; the MT-32 sets up its drums meanwhile. */
	{
		Flic flic(&voodoo);
		if (!flic.load(&DataFile, "lblogo"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb far *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, FadeSpeed, 0, 256);
		sceneTimer.set(100);
		if (!strcmp(driver, "static\\mt32mpu.adv")) {
			XmidiSong *percussion = 0;
			percussion = new XmidiSong(MusicDriver, "static\\percussy.xmi", 0);
			percussion->play();
			while (!percussion->isDone())
				;
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
		Flic flic(&voodoo);
		if (!flic.load(&DataFile, "castle"))
			ReadError();
		Yapper thunder("thndr");
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		thunder.play();
		if (Score && Speech == 1) {
			Score->play();
			if (driver == "static\\adlib.adv")
				Score->setVolume(1000, 0);
		}
		Palette palette;
		palette.load(0, 256, (Rgb far *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		int frames = flic.frameCount();
		int frame = 0;
		int count = 0;
		sceneTimer.set(50);
		while (!sceneTimer.hasFinished()) {
			CHECK_ESCAPE();
			frameTimer.set(4);
			flic.nextFrame();
			flic.show(&view);
			view.copyTo(CurrentView);
			frame++;
			if (frame == frames) {
				frame = 0;
				flic.rewind();
			}
			Timer_wait(&frameTimer);
		}
		flic.rewind();
		int flash = 0;
		sceneTimer.set(250);
		while (!sceneTimer.hasFinished()) {
			CHECK_ESCAPE();
			frameTimer.set(4);
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
			view.copyTo(CurrentView);
			Timer_wait(&frameTimer);
		}
		thunder.play();
		count = 0;
		sceneTimer.set(300);
		while (!sceneTimer.hasFinished()) {
			CHECK_ESCAPE();
			frameTimer.set(4);
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
			view.copyTo(CurrentView);
			Timer_wait(&frameTimer);
		}
		flic.rewind();
		flic.nextFrame();
		flic.show(&view);
		view.copyTo(CurrentView);
		fade.fadeToColor(&Black, FadeSpeed, 0, 256);
		ScreenView->clear(0);
	}
	CHECK_ESCAPE();

	if (Score && Speech == 0) {
		Score->play();
		if (driver == "static\\adlib.adv")
			Score->setVolume(1000, 0);
	}

	/* The guards bring Lord British what they found among Batlin's belongings. */
	{
		Flic flic(&voodoo);
		if (!flic.load(&DataFile, "sanctum1"))
			ReadError();
		Yapper guard(0);
		Yapper captain(0);
		Yapper lordBritish(0);
		guard.load("guard1");
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb far *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, FadeSpeed, 0, 256);
		fade = palette;
		int frames = flic.frameCount();
		int i = 0;
		for (i = 1; i < 35; i++) {
			CHECK_ESCAPE();
			frameTimer.set(3);
			flic.nextFrame();
			flic.show(&view);
			view.copyTo(CurrentView);
			Timer_wait(&frameTimer);
		}
		guard.play();
		for (i = 35; i < 60; i++) {
			CHECK_ESCAPE();
			frameTimer.set(4);
			flic.nextFrame();
			flic.show(&view);
			if (Jive)
				guard.say("        Yo, homes", 185, JUSTIFY_CENTER);
			else
				guard.say("        My leige", 185, JUSTIFY_CENTER);
			view.copyTo(CurrentView);
			Timer_wait(&frameTimer);
		}
		flic.show(&view);
		view.copyTo(CurrentView);
		guard.wait(3);
		guard.release();
		captain.load("guard2");
		captain.play();
		for (i = 60; i < 76; i++) {
			CHECK_ESCAPE();
			frameTimer.set(4);
			flic.nextFrame();
			flic.show(&view);
			captain.say("     All we found among Batlin's", 174, JUSTIFY_CENTER);
			captain.say("   belongings was this enchanted scroll...", 187, JUSTIFY_CENTER);
			view.copyTo(CurrentView);
			Timer_wait(&frameTimer);
		}
		captain.wait(90);
		flic.show(&view);
		view.copyTo(CurrentView);
		captain.wait(3);
		flic.show(&view);
		captain.say("     and a map showing the way to", 174, JUSTIFY_CENTER);
		captain.say("    a place called the Serpent Isle.", 187, JUSTIFY_CENTER);
		view.copyTo(CurrentView);
		captain.wait(116);
		captain.release();
		flic.show(&view);
		view.copyTo(CurrentView);
		captain.wait(3);
		lordBritish.load("lb1");
		lordBritish.play();
		for (i = 77; i < 81; i++) {
			CHECK_ESCAPE();
			frameTimer.set(4);
			flic.nextFrame();
			flic.show(&view);
			if (Jive)
				lordBritish.say("Iree. Slap it down there!       ", 185, JUSTIFY_CENTER);
			else
				lordBritish.say("Indeed.                 ", 174, JUSTIFY_CENTER);
			lordBritish.say("Put it on the table.       ", 187, JUSTIFY_CENTER);
			view.copyTo(CurrentView);
			Timer_wait(&frameTimer);
		}
		lordBritish.wait(66);
		fade.fadeToColor(&Black, 0, 0, 256);
		ScreenView->clear(0);
	}

	/* Lord British opens the scroll. */
	{
		Flic flic(&voodoo);
		if (!flic.load(&DataFile, "sanctum2"))
			ReadError();
		Yapper lordBritish("lb2");
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb far *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		fade = palette;
		int frames = flic.frameCount();
		int i = 0;
		for (i = 1; i < 21; i++) {
			CHECK_ESCAPE();
			frameTimer.set(4);
			flic.nextFrame();
			flic.show(&view);
			view.copyTo(CurrentView);
			Timer_wait(&frameTimer);
		}
		lordBritish.play();
		for (i = 21; i < frames; i++) {
			CHECK_ESCAPE();
			frameTimer.set(4);
			flic.nextFrame();
			flic.show(&view);
			if (Jive)
				lordBritish.say("Jump back!     ", 160, JUSTIFY_CENTER);
			else
				lordBritish.say("Stand back!     ", 160, JUSTIFY_CENTER);
			view.copyTo(CurrentView);
			Timer_wait(&frameTimer);
		}
		lordBritish.wait(71 - frames);
		fade.fadeToColor(&Black, 0, 0, 256);
		ScreenView->clear(0);
	}

	/* The Guardian's message to Batlin, mouthed in time with his speech. */
	{
		Flic flic(&voodoo);
		if (!flic.load(&DataFile, "sanctum3"))
			ReadError();
		Yapper guardian(1);
		FIND_CHUNK(DataFile, "SHAP", "guardian");
		MemHandle mouths;
		mouths.allocate(DataFile.chunk.size, FAR_MEMORY, 0, 1);
		DataFile.readChunk(mouths.pointer());
		SpeechStream *speaker = 0;
		if (Speech) {
			long start = voodoo + flic.length();
			FIND_CHUNK(DataFile, "VOCF", "gdian");
			long length = DataFile.loadChunk(start);
			speaker = new SpeechStream(SpeechDriver, start, length, SPEECH_BUFFER);
		} else
			;
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb far *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 0, 0, 256);
		int frames = flic.frameCount();
		int i = 1;
		int mouth = 1;
		if (speaker)
			speaker->play(-1);
		if (Speech) {
			while (!speaker->isDone()) {
				CHECK_ESCAPE();
				frameTimer.set(5);
				mouth++;
				speaker->service();
				flic.nextFrame();
				flic.show(&view);
				DrawFrame(&view, MOUTH_X, MOUTH_Y, mouths.pointer(), GuardianMouth[mouth] - 1);
				view.copyTo(CurrentView);
				i++;
				if (i == frames) {
					i = 0;
					flic.rewind();
				}
				Timer_wait(&frameTimer);
			}
		} else {
			for (int count = 0; count < 255; count++) {
				CHECK_ESCAPE();
				frameTimer.set(4);
				flic.nextFrame();
				flic.show(&view);
				DrawFrame(&view, MOUTH_X, MOUTH_Y, mouths.pointer(), random(6) + 1);
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
				view.copyTo(CurrentView);
				i++;
				if (i == frames) {
					i = 0;
					flic.rewind();
				}
				Timer_wait(&frameTimer);
			}
		}
		fade.fadeToColor(&Black, FadeSpeed, 0, 256);
		ScreenView->clear(0);
		delete speaker;
	}

	/* Lord British at the ship. */
	{
		Yapper lordBritish("lb3");
		lordBritish.play();
		Flic flic(&voodoo);
		if (!flic.load(&DataFile, "ship1"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb far *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 1, 0, 256);
		fade = palette;
		int frames = flic.frameCount();
		int i = 0;
		for (i = 1; i < frames; i++) {
			CHECK_ESCAPE();
			frameTimer.set(7);
			flic.nextFrame();
			flic.show(&view);
			if (i < 20)
				lordBritish.say("'Tis my worst fear!", 170, JUSTIFY_CENTER);
			else if (i > 23) {
				lordBritish.say("I must send the Avatar through", 170, JUSTIFY_CENTER);
				lordBritish.say("the pillars to the Serpent Isle", 184, JUSTIFY_CENTER);
			}
			view.copyTo(CurrentView);
			Timer_wait(&frameTimer);
		}
		fade.fadeToColor(&Black, 0, 0, 256);
		ScreenView->clear(0);
	}
	CHECK_ESCAPE();

	/* The ship sets sail. */
	{
		Flic flic(&voodoo);
		if (!flic.load(&DataFile, "ship2"))
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

	/* Through the pillars. */
	{
		Flic flic(&voodoo);
		frameTimer.set(200);
		if (!flic.load(&DataFile, "pil1"))
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
			frameTimer.set(4);
			flic.nextFrame();
			flic.show(&view);
			if (Jive && i > frames - 10) {
				Yapper caption(1);
				caption.say("Zot!", 174, JUSTIFY_CENTER);
			}
			view.copyTo(CurrentView);
			Timer_wait(&frameTimer);
		}
		fade.fadeToColor(&Black, 0, 0, 256);
		ScreenView->clear(0);
	}
	CHECK_ESCAPE();

	Score->setVolume(0, 4000);

	/* The Serpent Isle title. */
	{
		Flic flic(&voodoo);
		if (!flic.load(&DataFile, "u72_logo"))
			ReadError();
		flic.nextFrame();
		flic.showInColor(ScreenView, &Black);
		Palette palette;
		palette.load(0, 256, (Rgb far *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&Black, 1, 0, 256);
		fade = palette;
		frameTimer.set(200);
		Timer_wait(&frameTimer);
		fade.fadeToColor(&Black, 1, 0, 256);
		ScreenView->clear(0);
	}
	CHECK_ESCAPE();
	Cleanup(1);
}
