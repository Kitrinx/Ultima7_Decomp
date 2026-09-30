/* Black Gate ENDGAME.EXE, resident segment 1 (file offsets 0x005fd1 to 0x008ad2, 11009 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shutdown.h"
#include "display.h"
#include "vooalloc.h"
#include "systimer.h"
#include "options.h"
#include "gsound.h"
#include "memsys.h"
#include "strbuf.h"
#include "view.h"
#include "palette.h"
#include "iff.h"
#include "chunknam.h"
#include "font.h"
#include "flic.h"
#include "xmidi.h"
#include "digital.h"
#include "speech.h"
#include "textwin.h"
#include "endstats.h"
#include "endgame.h"

/* Size of the linear block the scenes and their speech are loaded into. */
#define LINEAR_SIZE     0xFFFFEL

#define FRAME_TICKS     4
#define SPEECH_BUFFER   0xC000
#define SHAPE_CYCLE     8       /* overlay selectors 0-7; 0 draws nothing */

#define LEAD_IN_FRAMES  171     /* flic1's first frame, held before it plays */
#define NO_FRAME        155     /* where the Guardian cries "No! You must not!" */
#define DAMN_FRAME      34      /* flic2 draws the overlay only after this frame */

#define PAGE_TICKS      1000    /* each epilogue page, unless a key is pressed */
#define LAST_TICKS      36000L

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

VoodooBlock VoodooXmsBlock;
AudioOptions Options;
SoundConfig Config;

int FadeSpeed = 1;
int SceneDelay = 90;

void Quit(char *message)
{
	ShutdownXMM();
	FatalMessage(message);
}

void ReadError()
{
	Quit("Error reading data file.");
}

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

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Type ULTIMA7 to play Ultima VII\n");
		exit(1);
	}
	if (stricmp(argv[1], "EREIAMJH")) {
		printf("Type ULTIMA7 to play Ultima VII\n");
		exit(1);
	}
	/* argv[1] again, not argv[2]: a fade speed given here becomes 0. */
	if (argc >= 3)
		FadeSpeed = atol(argv[1]);
	if (argc >= 4)
		SceneDelay = atol(argv[2]);

	int song = 0;
	Screen.open();
	VoodooXmsBlock.open();
	long voodoo = AllocateVoodooMemory(&VoodooXmsBlock, LINEAR_SIZE);
	SystemTimer.install();

	unsigned char speech = 0;
	unsigned char music = 0;
	unsigned char sfx = 0;
	Options.load("gamedat\\options.cfg");
	speech = Options.speechOn();
	music = Options.musicOn();
	sfx = Options.sfxOn();

	Config.readConfig("u7.cfg");
	SoundDriver *speechDriver = 0;
	SoundDriver *musicDriver = 0;
	if (Config.getSpeechEnabled())
		speechDriver = new SoundDriver("static\\sbdig.adv", 0, Config.getSpeechPort(), Config.getIrq(),
			Config.getDma(), 0);
	else
		speech = 0;
	if (Config.hasMusic()) {
		char *driver = 0;
		char *timbres = 0;
		if (Config.isRoland()) {
			driver = "static\\mt32mpu.adv";
			timbres = "static\\xmidi.mt";
			song = 1;
		} else if (Config.isAdlib() || Config.isSoundBlaster()) {
			driver = "static\\adlib.adv";
			timbres = "static\\xmidi.ad";
			song = 3;
		}
		if (driver)
			musicDriver = new SoundDriver(driver, timbres, 0, 0, 0, 0);
	} else
		music = sfx = 0;

	Timer timer;
	View view(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	IffFile file;
	file.open("static\\endgame.dat", FILE_READ);
	file.enterForm();

	/* Without speech the Guardian's lines are printed. */
	MemHandle fontData;
	ShapeFont *font = 0;
	TextWindow *window = 0;
	if (!speech) {
		FIND_CHUNK(file, "FONT", "font2");
		void far *data = Memory.allocate(file.chunk.size, FAR_MEMORY, 0, 1);
		file.readChunk(data);
		fontData.set(data, FAR_MEMORY, 1);
		font = new ShapeFont(fontData);
		font->setSpacing(-1, -1, 0);
		window = new TextWindow(&view, font);
	}

	file.findChunk("SHAP");
	MemHandle shapes;
	shapes.allocate(file.chunk.size, FAR_MEMORY, 0, 1);
	file.readChunk(shapes.pointer());

	XmidiPlayer *score = 0;
	if (music) {
		score = new XmidiPlayer(musicDriver, "static\\endscore.xmi", 0);
		if (Config.isRoland()) {
			score->play(0);
			while (!score->isDone(0))
				;
		}
	}
	long unusedFree1 = Memory.available(FAR_MEMORY);

	/* The Guardian at the Gate. */
	{
		VocSound *voice = 0;
		if (speech) {
			FIND_CHUNK(file, "VOCF", "voc1");
			void far *data = Memory.allocate(file.chunk.size, FAR_MEMORY, 0, 1);
			file.readChunk(data);
			voice = new VocSound(speechDriver, data);
		}
		Flic flic(&voodoo);
		if (!flic.load(&file, "flic1"))
			ReadError();
		if (music)
			score->play(song);
		int frames = flic.frameCount();
		for (int i = 0; i < LEAD_IN_FRAMES; i++) {
			timer.set(FRAME_TICKS);
			flic.rewind();
			flic.nextFrame();
			flic.show(&view);
			int shape = i < SHAPE_CYCLE ? i : i % SHAPE_CYCLE;
			if (shape)
				DrawFrame(&view, 0, 0, shapes.pointer(), shape);
			view.copyTo(CurrentView);
			Timer_wait(&timer);
		}
		flic.rewind();
		char sayNo = 0;
		for (i = 0; i < frames; i++) {
			timer.set(FRAME_TICKS);
			if (i == NO_FRAME) {
				if (speech)
					voice->play(-1);
				else
					sayNo++;
			}
			flic.nextFrame();
			flic.show(&view);
			int shape = i < SHAPE_CYCLE ? i : i % SHAPE_CYCLE;
			if (shape)
				DrawFrame(&view, 0, 0, shapes.pointer(), shape);
			if (sayNo)
				window->print("%Y%JNo!  You must not!", 175, JUSTIFY_CENTER);
			view.copyTo(CurrentView);
			Timer_wait(&timer);
		}
		if (voice) {
			voice->stopPlayback();
			delete voice;
		}
	}
	long unusedFree2 = Memory.available(FAR_MEMORY);

	/* The Gate destroyed. */
	{
		Flic flic(&voodoo);
		if (!flic.load(&file, "flic2"))
			ReadError();
		SpeechStream *speaker = 0;
		if (speech) {
			long start = voodoo + flic.length();
			FIND_CHUNK(file, "VOCF", "voc2");
			long length = file.loadChunk(start);
			speaker = new SpeechStream(speechDriver, start, length, SPEECH_BUFFER);
			speaker->play(-1);
		}
		if (music) {
			score->play(song + 1);
			score->stop(song);
		}
		int frames = flic.frameCount();
		for (int i = 0; i < frames; i++) {
			timer.set(FRAME_TICKS);
			flic.nextFrame();
			flic.show(&view);
			if (i > DAMN_FRAME) {
				int shape = i % SHAPE_CYCLE;
				if (shape)
					DrawFrame(&view, 0, 0, shapes.pointer(), shape);
			}
			if (speaker)
				speaker->service();
			else
				window->print("%Y%JDamn you Avatar!  Damn you!", 175, JUSTIFY_CENTER);
			view.copyTo(CurrentView);
			Timer_wait(&timer);
		}
		if (speech) {
			while (!speaker->isDone()) {
				for (i = 0; i < SHAPE_CYCLE; i++) {
					timer.set(FRAME_TICKS);
					speaker->service();
					int shape = i % SHAPE_CYCLE;
					if (shape)
						DrawFrame(ScreenView, 0, 0, shapes.pointer(), shape);
					Timer_wait(&timer);
				}
			}
		} else {
			timer.set(120);
			Timer_wait(&timer);
		}
		delete speaker;
		shapes.release(0);

		Palette palette;
		palette.load(0, 256, (Rgb far *)flic.getPalette());
		palette.write(0, 256, 0);
		Rgb black(0, 0, 0);
		FadePalette fade(palette);
		fade.fadeToColor(&black, FadeSpeed, 0, 256);
		timer.set(SceneDelay);
		Timer_wait(&timer);

		FIND_CHUNK(file, "FONT", "font1");
		MemHandle fontData;
		fontData.allocate(file.chunk.size, FAR_MEMORY, 0, 1);
		file.readChunk(fontData.pointer());
		ShapeFont font(fontData);
		font.setSpacing(-1, -1, 0);
		Rect screen(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
		TextBox box(screen, ScreenView, &font);
		ScreenView->clear(0);
		box.print("%Y%JThe Black Gate is destroyed.", 80, JUSTIFY_CENTER);
		fade = palette;
		fade.fadeFromColor(&black, 2, 0, 256);
		timer.set(100);
		Timer_wait(&timer);
		fade = palette;
		fade.fadeToColor(&black, 2, 0, 256);
		ScreenView->clear(0);
		box.print("%Y%JThe Guardian has been stopped.", 80, JUSTIFY_CENTER);
		fade = palette;
		fade.fadeFromColor(&black, 2, 0, 256);
		timer.set(100);
		Timer_wait(&timer);
		fade.fadeToColor(&black, 2, 0, 256);
	}

	if (!speech) {
		delete window;
		delete font;
		FIND_CHUNK(file, "FONT", "font3");
		void far *data = Memory.allocate(file.chunk.size, FAR_MEMORY, 0, 1);
		file.readChunk(data);
		fontData.set(data, FAR_MEMORY, 1);
		font = new ShapeFont(fontData);
		font->setSpacing(-1, -1, 0);
		window = new TextWindow(&view, font);
	}
	long unusedFree3 = Memory.available(FAR_MEMORY);

	/* The Guardian's parting threat, then the epilogue. */
	{
		Flic flic(&voodoo);
		if (!flic.load(&file, "flic3"))
			ReadError();
		SpeechStream *speaker = 0;
		if (speech) {
			long start = voodoo + flic.length();
			FIND_CHUNK(file, "VOCF", "voc3");
			long length = file.loadChunk(start);
			speaker = new SpeechStream(speechDriver, start, length, SPEECH_BUFFER);
		}
		Rgb black(0, 0, 0);
		flic.nextFrame();
		flic.showInColor(ScreenView, &black);
		Palette palette;
		palette.load(0, 256, (Rgb far *)flic.getPalette());
		FadePalette fade(palette);
		fade.fadeFromColor(&black, FadeSpeed, 0, 256);
		int frames = flic.frameCount();
		Timer hold;
		hold.set(SceneDelay + 100);
		while (!hold.hasFinished()) {
			for (int i = 0; i < frames; i++) {
				timer.set(FRAME_TICKS);
				flic.nextFrame();
				flic.show(&view);
				view.copyTo(CurrentView);
				Timer_wait(&timer);
			}
			flic.rewind();
		}
		if (speaker)
			speaker->play(-1);
		if (speech) {
			while (!speaker->isDone() || score->isPlaying(song + 1)) {
				for (int i = 0; i < frames; i++) {
					timer.set(FRAME_TICKS);
					speaker->service();
					flic.nextFrame();
					flic.show(&view);
					view.copyTo(CurrentView);
					Timer_wait(&timer);
				}
				flic.rewind();
			}
		} else {
			hold.set(1200);
			while (!hold.hasFinished() || score->isPlaying(song + 1)) {
				for (int i = 0; i < frames; i++) {
					timer.set(FRAME_TICKS);
					flic.nextFrame();
					flic.show(&view);
					window->print("%Y%JAvatar!  You think you have won?", 40, JUSTIFY_CENTER);
					window->print("%Y%JThink again!  You are unable to", 55, JUSTIFY_CENTER);
					window->print("%Y%Jleave Britannia, whereas I am free", 70, JUSTIFY_CENTER);
					window->print("%Y%Jto enter other worlds!  Hmmm...", 85, JUSTIFY_CENTER);
					window->print("%Y%JPerhaps your puny Earth shall be", 100, JUSTIFY_CENTER);
					window->print("%Y%Jmy NEXT target!", 115, JUSTIFY_CENTER);
					view.copyTo(CurrentView);
					Timer_wait(&timer);
				}
				flic.rewind();
			}
		}
		fade.fadeToColor(&black, FadeSpeed, 0, 256);
		delete speaker;

		FIND_CHUNK(file, "FONT", "font4");
		{
			MemHandle fontData;
			fontData.allocate(file.chunk.size, FAR_MEMORY, 0, 1);
			file.readChunk(fontData.pointer());
			ShapeFont font(fontData);
			font.setSpacing(-1, -1, 0);
			Rect screen(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
			TextBox box(screen, ScreenView, &font);
			while (kbhit())
				getch();

			ScreenView->clear(0);
			box.print("%Y%JIn the months following the climactic", 25, JUSTIFY_CENTER);
			box.print("%Y%Jbattle at The Black Gate, Britannia", 40, JUSTIFY_CENTER);
			box.print("%Y%Jis set upon the long road to recovery", 55, JUSTIFY_CENTER);
			box.print("%Y%Jfrom its various plights.", 70, JUSTIFY_CENTER);
			box.print("%Y%JUpon your return to Britain,", 100, JUSTIFY_CENTER);
			box.print("%Y%JLord British decreed that", 115, JUSTIFY_CENTER);
			box.print("%Y%JThe Fellowship be outlawed,", 130, JUSTIFY_CENTER);
			box.print("%Y%Jand all of the branches were", 145, JUSTIFY_CENTER);
			box.print("%Y%Jsoon destroyed.", 160, JUSTIFY_CENTER);
			fade = palette;
			fade.fadeFromColor(&black, FadeSpeed, 0, 256);
			WaitOrKey(PAGE_TICKS);
			fade = palette;
			fade.fadeToColor(&black, FadeSpeed, 0, 256);

			ScreenView->clear(0);
			box.print("%Y%JThe frustration you feel at having been", 55, JUSTIFY_CENTER);
			box.print("%Y%Jstranded in Britannia is somewhat", 70, JUSTIFY_CENTER);
			box.print("%Y%Jalleviated by the satisfaction that you", 85, JUSTIFY_CENTER);
			box.print("%Y%Jsolved the gruesome murders committed", 100, JUSTIFY_CENTER);
			box.print("%Y%Jby The Fellowship and even avenged the", 115, JUSTIFY_CENTER);
			box.print("%Y%Jdeath of Spark's father.", 130, JUSTIFY_CENTER);
			fade = palette;
			fade.fadeFromColor(&black, FadeSpeed, 0, 256);
			WaitOrKey(PAGE_TICKS);
			fade = palette;
			fade.fadeToColor(&black, FadeSpeed, 0, 256);

			ScreenView->clear(0);
			box.print("%Y%JAnd although you are, at the moment, ", 55, JUSTIFY_CENTER);
			box.print("%Y%Jhelpless to do anything about", 70, JUSTIFY_CENTER);
			box.print("%Y%JThe Guardian's final threat,", 85, JUSTIFY_CENTER);
			box.print("%Y%Janother thought nags at you...", 100, JUSTIFY_CENTER);
			box.print("%Y%Jwhat became of Batlin, the fiend", 115, JUSTIFY_CENTER);
			box.print("%Y%Jwho got away?", 130, JUSTIFY_CENTER);
			fade = palette;
			fade.fadeFromColor(&black, FadeSpeed, 0, 256);
			WaitOrKey(PAGE_TICKS);
			fade = palette;
			fade.fadeToColor(&black, FadeSpeed, 0, 256);

			ScreenView->clear(0);
			box.print("%Y%JThat is another story...", 70, JUSTIFY_CENTER);
			box.print("%Y%Jone that will take you", 85, JUSTIFY_CENTER);
			box.print("%Y%Jto a place called", 100, JUSTIFY_CENTER);
			box.print("%Y%JThe Serpent Isle...", 115, JUSTIFY_CENTER);
			fade = palette;
			fade.fadeFromColor(&black, FadeSpeed, 0, 256);
			WaitOrKey(PAGE_TICKS);
			fade = palette;
			fade.fadeToColor(&black, FadeSpeed, 0, 256);
		}

		/* How long the game took, when the game left a record of it. */
		GameDate date;
		if (date.load("static\\endstats.dat")) {
			FIND_CHUNK(file, "FONT", "font4");
			{
				MemHandle fontData;
				fontData.allocate(file.chunk.size, FAR_MEMORY, 0, 1);
				file.readChunk(fontData.pointer());
				ShapeFont font(fontData);
				font.setSpacing(-1, -1, 0);
				Rect screen(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
				TextBox box(screen, ScreenView, &font);
				ScreenView->clear(0);
				box.print("%Y%JCongratulations!", 40, JUSTIFY_CENTER);
				box.print("%Y%JYou have completed Ultima VII in", 55, JUSTIFY_CENTER);
				{
					char comma = 0;
					Message text;
					Message part;
					text.assign("%Y%J");
					if (date.year) {
						part.format("%d year", date.year);
						text.append(part);
						if (date.year > 1)
							text.append("s");
						comma = 1;
					}
					if (date.month) {
						if (comma)
							text.append(", ");
						part.format("%d month", date.month);
						text.append(part);
						if (date.month > 1)
							text.append("s");
						comma = 1;
					}
					if (date.day) {
						if (comma)
							text.append(", & ");
						part.format("%d day", date.day);
						text.append(part);
						if (date.day > 1)
							text.append("s");
						text.append(".");
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
			}
		}
	}
	file.close();

	/* Tells the game the ending has been seen. */
	{
		DiskFile seen;
		if (seen.open("static\\endgame.flg", FILE_CREATE))
			seen.write("Congratulations!", 17);
	}

	if (score)
		delete score;
	delete speechDriver;
	delete musicDriver;
	VoodooXmsBlock.close();
	Screen.close();
	return EXIT_CREDITS;
}
