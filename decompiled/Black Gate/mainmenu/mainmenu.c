/* Black Gate MAINMENU.EXE, resident segment 1 (file offsets 0x007652 to 0x00a021, 10703 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include <conio.h>
#include <dir.h>
#include <dos.h>
#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lowlevel.h"
#include "vidmode.h"
#include "screen.h"
#include "memapi.h"
#include "vooalloc.h"
#include "xmminit.h"
#include "freexmm.h"
#include "errors.h"
#include "oops.h"
#include "vstring.h"
#include "chkfile.h"
#include "view.h"
#include "systimer.h"
#include "mouse.h"
#include "mevent.h"
#include "u7event.h"
#include "u7point.h"
#include "colbuf.h"
#include "keyqueue.h"
#include "itable.h"
#include "device.h"
#include "fadepal.h"
#include "controls.h"
#include "textspr.h"
#include "textfld.h"
#include "menu.h"
#include "music.h"
#include "cflxbuf.h"
#include "specard.h"
#include "install.h"
#include "credits.h"
#include "mainmenu.h"

/* Audio settings: each is 0 unset, AUDIO_OFF or AUDIO_ON. */
#define AUDIO_OFF   1
#define AUDIO_ON    2

#define KEY_ALT_X       (KEY_EXTENDED | 45)
#define KEY_ESCAPE      27
#define KEY_ENTER       13

/* A clean MAINMENU.EXE is exactly this many bytes. */
#define MAINMENU_SIZE   127116L

/* What the launcher runs next, from this program's exit code. */
#define LAUNCH_GAME     3
#define LAUNCH_ENDGAME  4
#define LAUNCH_THANKS   5       /* print the thanks and return to DOS */
#define LAUNCH_INTRO    6

#define GAME_ARGS_SIZE  17      /* sex byte, then the name */
#define MOUSE_EVENTS    30
#define IDLE_TICKS      900L    /* the introduction plays after this long untouched */
#define SPEECH_BUFFER   2596
#define SPEECH_RATE     10667
#define THE_END_PAUSE   960
#define SONG_FADE_TICKS 150

/* Choices; the main menu returns the ones the launcher acts on. */
#define CHOICE_CREDITS  1
#define CHOICE_NEW_GAME 2
#define CHOICE_JOURNEY  3
#define CHOICE_INTRO    4
#define CHOICE_QUIT     5
#define CHOICE_CREATE   6
#define CHOICE_BACK     7
#define CHOICE_NAME     8
#define CHOICE_SEX      9
#define CHOICE_QUOTES   10
#define CHOICE_ENDGAME  11
#define CHOICE_MALE     12
#define CHOICE_FEMALE   13

/* MAINSHP.FLX entries. */
#define SHAPE_PORTRAIT_MALE     0
#define SHAPE_PORTRAIT_FEMALE   1
#define SHAPE_BACKGROUND        2
#define SHAPE_INTRO             4
#define SHAPE_NEW_GAME          5
#define SHAPE_CREDITS           6
#define SHAPE_RETURN            7
#define SHAPE_JOURNEY           8
#define SHAPE_FONT              9
#define SHAPE_SEX               10
#define SHAPE_NAME              12
#define SHAPE_QUOTES            17
#define SHAPE_ENDGAME           18
#define SHAPE_POINTERS          19
#define SHAPE_MALE              22
#define SHAPE_FEMALE            23
#define SHAPE_NAME_FRAME        24

/* The BIOS video mode to go back to; 3 is color text. */
struct VideoMode {
	unsigned char mode;
	VideoMode() : mode(3) {}
};

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

extern "C" long OpenExtendedMemory(void);
extern "C" long GetExtendedMemorySize(void);
extern "C" int far GetProcessorMode(void);
extern "C" void far EnterFlatMode(void);
long far GetFreeDosMemory(void);
extern int FlatModeFlags;

unsigned char GameExists(void);
void ShowMessage(char *text);

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
char *GamedatPath = ".\\gamedat\\";
char *StaticPath = ".\\static\\";
MouseDevice *MouseInput = 0;
EventQueue *MouseEvents = 0;
Mouse MouseDriver;
VideoMode OriginalVideoMode;
FadingPalette OriginalPalette;
FlexSpeechCache SpeechCache(0);
MusicSystem Music;
FlexTextPrinter MenuFont;
unsigned char MusicDriver = 0;         /* a MUSIC_DEVICE_ value, 0 for none */
FatalHandler PreviousFatalHook = 0;
VoodooBlock VoodooXmsBlock = { 0 };
View ScreenView;
View Viewport;
DeviceList Devices;
unsigned char SpeechEnabled = 0;
SoundConfig SoundSetup;
AudioOptions AudioSettings;
unsigned long StepTicks = 7;            /* the viewers' scroll speed, argv[2] */
int FadeTicks = 60;                     /* argv[3] */
long DosMemory = 0;                     /* bytes free, and what the game needs */
long DosMemoryNeeded = 530000L;
unsigned long DiskSpaceNeeded = 1024000L;
long ExtendedMemoryNeeded = 1048576L;
CtrlBreakTrap CtrlBreakHook;
BiosHook SystemServicesHook;
char AvatarSex = 'M';
unsigned char NotLaunched = 0;          /* run without the launcher's mode letter */
unsigned char CreditsOnly = 0;
unsigned char LossOnly = 0;
unsigned char CheckOnly = 0;
unsigned char CleanUpOnly = 0;
unsigned char SystemReady = 0;          /* the screen and palette are ours */
unsigned char KeyPressed = 0;
unsigned char XmsOpened = 0;            /* extended memory is open */
RgbColor Black;
RgbColor Red;
char MusicFlexName[80];
char *MusicFlex = MusicFlexName;
char AvatarName[20];

char *DataPath(char *dir, char *name)
{
	static char path[80];

	strcpy(path, dir);
	strcat(path, name);
	return path;
}

char *OtherDataPath(char *dir, char *name)
{
	static char path[38];

	strcpy(path, dir);
	strcat(path, name);
	return path;
}

/* Waits up to ticks for a key; true when one was pressed and handled. */
unsigned char WaitForKey(unsigned ticks)
{
	Timer timer(ticks);

	Timer_restart(&timer);
	while (!Timer_hasFinished(&timer) && !kbhit())
		;
	if (kbhit()) {
		HandleKey();
		return 1;
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

/* Whether the named file can be opened. */
unsigned char FileExists(char *name)
{
	DataFile file;
	unsigned char found;

	found = file.open(name, FILE_OPEN) == 1;
	if (found)
		file.close();
	return found;
}

/* Creates an empty file, as the viewers do to unlock menu choices. */
void CreateFlag(char *name)
{
	DataFile file;

	file.open(name, FILE_CREATE);
	file.close();
}

void WaitForClickOrKey(Mouse *, KeyQueue *keys)
{
	MouseEvent event;
	int key;
	unsigned char done = 0;

	keys->reset();
	while (!done) {
		CopyMouseEvent(&event);
		if (event.valid() && event.type == MOUSE_EVENT_RELEASED)
			done = 1;
		keys->poll();
		if (keys->get(&key))
			done = 1;
	}
	keys->reset();
}

/* Enter and the down arrow step from the sex choice to the next entry, space picks it. */
char far NameKeyFilter(int key)
{
	char action = MENU_NONE;

	switch (key) {
	case KEY_ENTER:
	case KEY_DOWN:
		action = MENU_NEXT;
		break;
	case KEY_UP:
		action = MENU_PREVIOUS;
		break;
	case ' ':
		action = MENU_SELECT;
		break;
	}
	return action;
}

/*
 * The main menu and the new-game screen behind it. The menu fades in in three stages, a wait
 * before each, and a key skips the rest. Returns the choice the launcher acts on.
 */
int RunMainMenu(int firstWait, int secondWait, int thirdWait)
{
	unsigned char endgameOpen = 0;
	unsigned char quotesOpen = 0;
	unsigned char skipped = 0;
	FadingPalette *palettes[3];

	quotesOpen = FileExists(DataPath(StaticPath, "quotes.flg"));
	endgameOpen = FileExists(DataPath(StaticPath, "endgame.flg"));
	FadingPalette background, buttons, menuPalette;
	background.fill(&Black, 0, PALETTE_COLORS - 1);
	background.apply();
	menuPalette.load(DataPath(StaticPath, "intropal.dat"), 6);
	/* each stage's palette shows one more band of colors */
	background = buttons = menuPalette;
	background.fill(&Black, 48, PALETTE_COLORS - 1);
	buttons.fill(&Black, 96, PALETTE_COLORS - 1);
	palettes[0] = &background;
	palettes[1] = &buttons;
	palettes[2] = &menuPalette;

	Screen screen(0);
	Sprite picture(DataPath(StaticPath, "mainshp.flx"), SHAPE_BACKGROUND, 0, &screen, 0);
	picture.moveTo(0, 0);
	screen.paint(0, 0);
	TextSprite noGame((void far *) MenuFont.shape, 0, &screen, 0);
	TextSprite noName((void far *) MenuFont.shape, 0, &screen, 0);
	noGame.hide();
	noName.hide();
	noGame.setText("You must start a new game first.", -1);
	noName.setText("Avatar! You must choose a name.", -1);
	noGame.setAlign(ALIGN_CENTRE);
	noName.setAlign(ALIGN_CENTRE);
	noGame.moveTo(150, 150);
	noName.moveTo(150, 150);
	noGame.spacing = 1;
	noName.spacing = 1;

	Button intro(DataPath(StaticPath, "mainshp.flx"), SHAPE_INTRO, 0, &screen, 0);
	Button newGame(DataPath(StaticPath, "mainshp.flx"), SHAPE_NEW_GAME, 0, &screen, 0);
	Button journey(DataPath(StaticPath, "mainshp.flx"), SHAPE_JOURNEY, 0, &screen, 0);
	Button credits(DataPath(StaticPath, "mainshp.flx"), SHAPE_CREDITS, 0, &screen, 0);
	Button endgame(DataPath(StaticPath, "mainshp.flx"), SHAPE_ENDGAME, 0, &screen, 0);
	Button quotes(DataPath(StaticPath, "mainshp.flx"), SHAPE_QUOTES, 0, &screen, 0);
	intro.align = ALIGN_CENTRE;
	newGame.align = ALIGN_CENTRE;
	journey.align = ALIGN_CENTRE;
	credits.align = ALIGN_CENTRE;
	endgame.align = ALIGN_CENTRE;
	quotes.align = ALIGN_CENTRE;
	KeyQueue keys(20);
	Menu menu;
	FlushMouseToRelease();
	menu.setPointer(&MouseDriver);
	menu.setKeys(&keys);
	AddButtonEntry(&menu, &endgame, CHOICE_ENDGAME, 0, 0);
	AddButtonEntry(&menu, &intro, CHOICE_INTRO, 0, 0);
	AddButtonEntry(&menu, &newGame, CHOICE_CREATE, 0, 0);
	AddButtonEntry(&menu, &journey, CHOICE_JOURNEY, 0, 0);
	AddButtonEntry(&menu, &credits, CHOICE_CREDITS, 0, 0);
	AddButtonEntry(&menu, &quotes, CHOICE_QUOTES, 0, 0);
	endgame.moveTo(159, 115);
	intro.moveTo(159, 130);
	newGame.moveTo(159, 145);
	journey.moveTo(159, 160);
	credits.moveTo(159, 175);
	quotes.moveTo(159, 190);
	if (!endgameOpen)
		endgame.hide();
	if (!quotesOpen)
		quotes.hide();

	/* the new-game screen, hidden until New Game is chosen */
	Button nameLabel(DataPath(StaticPath, "mainshp.flx"), SHAPE_NAME, 0, &screen, 0);
	Button sexLabel(DataPath(StaticPath, "mainshp.flx"), SHAPE_SEX, 0, &screen, 0);
	Button male(DataPath(StaticPath, "mainshp.flx"), SHAPE_MALE, 0, &screen, 0);
	Button female(DataPath(StaticPath, "mainshp.flx"), SHAPE_FEMALE, 0, &screen, 0);
	Button back(DataPath(StaticPath, "mainshp.flx"), SHAPE_RETURN, 0, &screen, 0);
	Button begin(DataPath(StaticPath, "mainshp.flx"), SHAPE_JOURNEY, 0, &screen, 0);
	Button nameFrame(DataPath(StaticPath, "mainshp.flx"), SHAPE_NAME_FRAME, 0, &screen, 0);
	nameLabel.align = ALIGN_LEFT;
	sexLabel.align = ALIGN_LEFT;
	back.align = ALIGN_LEFT;
	begin.align = ALIGN_LEFT;
	male.align = ALIGN_LEFT;
	female.align = ALIGN_LEFT;
	nameFrame.align = ALIGN_LEFT;
	Sprite malePortrait(DataPath(StaticPath, "mainshp.flx"), SHAPE_PORTRAIT_MALE, 0, &screen, 0);
	Sprite femalePortrait(DataPath(StaticPath, "mainshp.flx"), SHAPE_PORTRAIT_FEMALE, 0, &screen, 0);
	TextBox name(&MenuFont, 14, &keys, '_');
	name.attach(&screen);
	Menu createMenu;
	createMenu.setPointer(&MouseDriver);
	createMenu.setKeys(&keys);
	AddButtonEntry(&createMenu, &nameLabel, CHOICE_NAME, 0, 0);
	AddButtonEntry(&createMenu, &sexLabel, CHOICE_SEX, NameKeyFilter, 0);
	AddButtonEntry(&createMenu, &begin, CHOICE_NEW_GAME, 0, 0);
	AddButtonEntry(&createMenu, &back, CHOICE_BACK, 0, 0);
	AddButtonEntry(&createMenu, &male, CHOICE_MALE, 0, 1);
	AddButtonEntry(&createMenu, &female, CHOICE_FEMALE, 0, 1);
	AddButtonEntry(&createMenu, &nameFrame, CHOICE_NAME, 0, 1);
	nameLabel.moveTo(40, 120);
	sexLabel.moveTo(40, 140);
	begin.moveTo(40, 180);
	back.moveTo(180, 180);
	male.moveTo(90, 140);
	female.moveTo(90, 140);
	name.moveTo(90, 126);
	nameFrame.moveTo(90, 120);
	femalePortrait.moveTo(240, 120);
	malePortrait.moveTo(240, 120);
	nameLabel.hide();
	sexLabel.hide();
	back.hide();
	begin.hide();
	male.hide();
	female.hide();
	name.hide();
	nameFrame.hide();
	femalePortrait.hide();
	malePortrait.hide();

	Song song(&Music, MusicFlex, 3);
	song.play();
	screen.paint(0, 0);
	if (WaitForKey(firstWait))
		skipped = 1;
	if (!skipped) {
		KeyPressed = 0;
		background.fadeFromColor(&Black, 0, 0, 47, FadeTicks);
		if (KeyPressed)
			skipped = 1;
	}
	if (!skipped && WaitForKey(secondWait))
		skipped = 1;
	if (!skipped) {
		KeyPressed = 0;
		buttons.fadeFromColor(&Black, 0, 48, 97, FadeTicks);
		if (KeyPressed)
			skipped = 1;
	}
	if (!skipped && WaitForKey(thirdWait))
		skipped = 1;
	if (!skipped)
		menuPalette.fadeFromColor(&Black, 0, 97, 254, FadeTicks);
	else
		menuPalette.apply();
	ShowCursor();

	Menu *current = &menu;
	current->showCurrent();
	keys.poll();
	unsigned char done = 0;
	int choice = 0;
	int key;
	Timer idle;
	if (!MousePresent)
		current->select(CHOICE_INTRO);
	/* after IDLE_TICKS with nothing chosen or moved, the introduction plays */
	while (!done) {
		current->changed = 0;
		Timer_set(&idle, IDLE_TICKS);
		while ((choice = current->run()) == 0) {
			if (current == &createMenu) {
				name.update();
			} else if (keys.peek(&key)) {
				keys.get(&key);
				if (key == KEY_ALT_X)
					Quit();
			}
			keys.poll();
			screen.paint(0, 0);
			if (song.finished()) {
				song.load(MusicFlex, 3);
				song.play();
			}
			if (Timer_hasFinished(&idle) && current != &createMenu) {
				if (current->changed) {
					current->changed = 0;
					Timer_set(&idle, IDLE_TICKS);
				} else {
					choice = CHOICE_INTRO;
					break;
				}
			}
		}
		switch (choice) {
		case 0:
			break;
		case CHOICE_CREATE:
			{
				FadingPalette fader;
				fader = menuPalette;
				fader.fadeToColor(&Black, 0, 97, 254, FadeTicks);
				intro.hide();
				newGame.hide();
				journey.hide();
				credits.hide();
				endgame.hide();
				quotes.hide();
				nameLabel.show();
				sexLabel.show();
				back.show();
				begin.show();
				male.show();
				malePortrait.show();
				name.show();
				nameFrame.show();
				AvatarSex = 'M';
				AvatarName[0] = 0;
				name.setText(AvatarName);
				current = &createMenu;
				current->select(CHOICE_NAME);
				screen.paint(0, 0);
				fader = menuPalette;
				fader.fadeFromColor(&Black, 0, 97, 254, FadeTicks);
			}
			goto enterName;             /* straight to typing the name */
		case CHOICE_SEX:
		case CHOICE_MALE:
		case CHOICE_FEMALE:
			if (AvatarSex == 'M') {
				AvatarSex = 'F';
				male.hide();
				female.show();
				malePortrait.hide();
				femalePortrait.show();
			} else {
				AvatarSex = 'M';
				male.show();
				female.hide();
				malePortrait.show();
				femalePortrait.hide();
			}
			break;
		case CHOICE_BACK:
			{
				FadingPalette fader;
				fader = menuPalette;
				fader.fadeToColor(&Black, 0, 97, 254, FadeTicks);
				nameLabel.hide();
				sexLabel.hide();
				back.hide();
				begin.hide();
				male.hide();
				female.hide();
				malePortrait.hide();
				femalePortrait.hide();
				name.hide();
				nameFrame.hide();
				intro.show();
				newGame.show();
				journey.show();
				credits.show();
				if (endgameOpen)
					endgame.show();
				if (quotesOpen)
					quotes.show();
				current = &menu;
				current->select(CHOICE_INTRO);
				screen.paint(0, 0);
				fader = menuPalette;
				fader.fadeFromColor(&Black, 0, 97, 254, FadeTicks);
			}
			break;
		case CHOICE_NAME:
		enterName:
			{
				unsigned char entered = 0;

				name.active = 1;
				while (!entered) {
					if (name.update()) {
						entered = 1;
					} else if (keys.peek(&key)) {
						keys.get(&key);
						switch (key) {
						case KEY_ALT_X:
							Quit();
							break;
						case KEY_DOWN:
							entered = 1;
							break;
						}
					} else {
						MouseEvent event;

						CopyMouseEvent(&event);
						if (event.valid() && event.type == MOUSE_EVENT_RELEASED)
							entered = 1;
					}
					keys.poll();
					screen.paint(0, 0);
					if (song.finished()) {
						song.load(MusicFlex, 3);
						song.play();
					}
				}
				name.active = 0;
				current->select(CHOICE_SEX);
				FlushMouseToRelease();
			}
			break;
		case CHOICE_NEW_GAME:
			if ((char) !*name.text) {
				HideCursor();
				FadingPalette fader;
				fader = menuPalette;
				fader.fadeToColor(&Black, 0, 97, 254, FadeTicks);
				noName.show();
				nameLabel.hide();
				sexLabel.hide();
				back.hide();
				begin.hide();
				male.hide();
				female.hide();
				malePortrait.hide();
				femalePortrait.hide();
				name.hide();
				screen.paint(0, 0);
				fader = menuPalette;
				fader.fadeFromColor(&Black, 0, 97, 254, FadeTicks);
				ShowCursor();
				WaitForClickOrKey(&MouseDriver, &keys);
				HideCursor();
				fader.fadeToColor(&Black, 0, 97, 254, FadeTicks);
				noName.hide();
				nameLabel.show();
				sexLabel.show();
				back.show();
				begin.show();
				if (AvatarSex == 'M') {
					male.show();
					malePortrait.show();
				} else {
					female.show();
					femalePortrait.show();
				}
				name.show();
				screen.paint(0, 0);
				fader = menuPalette;
				fader.fadeFromColor(&Black, 0, 97, 254, FadeTicks);
				ShowCursor();
			} else {
				strcpy(AvatarName, name.text);
				done = 1;
			}
			break;
		case CHOICE_JOURNEY:
			if (!GameExists()) {
				HideCursor();
				FadingPalette fader;
				fader = menuPalette;
				fader.fadeToColor(&Black, 0, 97, 254, FadeTicks);
				noGame.show();
				intro.hide();
				newGame.hide();
				journey.hide();
				credits.hide();
				endgame.hide();
				quotes.hide();
				screen.paint(0, 0);
				fader = menuPalette;
				fader.fadeFromColor(&Black, 0, 97, 254, FadeTicks);
				ShowCursor();
				WaitForClickOrKey(&MouseDriver, &keys);
				HideCursor();
				fader.fadeToColor(&Black, 0, 97, 254, FadeTicks);
				noGame.hide();
				intro.show();
				newGame.show();
				journey.show();
				credits.show();
				if (endgameOpen)
					endgame.show();
				if (quotesOpen)
					quotes.show();
				screen.paint(0, 0);
				fader = menuPalette;
				fader.fadeFromColor(&Black, 0, 97, 254, FadeTicks);
				ShowCursor();
			} else {
				done = 1;
			}
			break;
		default:
			strcpy(AvatarName, name.text);
			done = 1;
			break;
		}
	}
	song.fadeOut(SONG_FADE_TICKS);
	HideCursor();
	menuPalette.fadeToColor(&Black, 0, 0, PALETTE_COLORS - 1, FadeTicks);
	FillView(&ScreenView, 0);
	return choice;
}

/* A fatal error puts the screen and palette back before the message. */
void OnFatalError(void)
{
	RestoreSystem(1, 1);
	VideoMode textMode;
	textMode.mode = 3;
	SetVideoMode(&textMode.mode);
	if (PreviousFatalHook)
		PreviousFatalHook();
}

void InstallFatalHook(void)
{
	PreviousFatalHook = SwapFatalHook(OnFatalError);
	StartFarHeap(0);
	Devices.open(DEVICE_TIMER);
}

/* Stops the music and devices; blackens or restores the palette, and with both flags the text mode. */
void RestoreSystem(char restorePalette, char clearScreen)
{
	Music.stop();
	Devices.closeAll();
	if (SystemReady) {
		if (clearScreen)
			FillView(&ScreenView, 0);
		if (restorePalette) {
			OriginalPalette.apply();
		} else {
			FadingPalette palette;

			palette.fill(&Black, 0, PALETTE_COLORS - 1);
			palette.apply();
		}
	}
	if (XmsOpened)
		ShutdownXMM();
	ResetFarHeap(0);
	CloseFarHeap(0);
	if (restorePalette && clearScreen)
		SetVideoMode(&OriginalVideoMode.mode);
}

void InitEnvironment(void)
{
	GetVideoMode(&OriginalVideoMode.mode);
	Black.red = Black.blue = Black.green = 0;
	Red.red = 63;
	Red.blue = 0;
	Red.green = 0;
	SetDisplayMode(0);
	SystemReady = 1;
	OriginalPalette.capture();
	FadingPalette palette;
	palette.fill(&Black, 0, PALETTE_COLORS - 1);
	palette.apply();
	Viewport.clip.x0 = 0;
	Viewport.clip.y0 = 0;
	Viewport.clip.x1 = SCREEN_WIDTH - 1;
	Viewport.clip.y1 = SCREEN_HEIGHT - 1;
	if (!AllocateDrawBuffer(&Viewport, 0, DRAW_IN_XMS))
		ReportOutOfVoodooMemory();
}

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
	if ((unsigned char) SoundSetup.isRoland()) {
		strcpy(MusicFlexName, DataPath(StaticPath, "intrordm.dat"));
		*device = MUSIC_DEVICE_MT32;
		DosMemoryNeeded += InstallInfo.rolandMemory;
	} else if ((unsigned char) SoundSetup.isAdlib() || (unsigned char) SoundSetup.isSoundBlaster()) {
		strcpy(MusicFlexName, DataPath(StaticPath, "introadm.dat"));
		*device = MUSIC_DEVICE_ADLIB;
		DosMemoryNeeded += InstallInfo.adlibMemory;
	}
	*irq = SoundSetup.irq;
	*port = SoundSetup.speechPort;
	*dma = SoundSetup.dma;
	if (SoundSetup.speechEnabled) {
		SpeechEnabled = 1;
		DosMemoryNeeded += InstallInfo.speechMemory;
	} else {
		SpeechEnabled = 0;
	}
	if ((unsigned char) SoundSetup.hasMusic()) {
		EnableMusic();
		EnableSfx();
		if (ReadAudioOptions(preferencesPath, &AudioSettings)) {
			if (AudioSettings.music == AUDIO_OFF)
				DisableMusic();
			if (AudioSettings.effects == AUDIO_OFF)
				DisableSfx();
			if (AudioSettings.speech == AUDIO_OFF)
				SpeechEnabled = 0;
		}
	}
}

/* Always passes. */
int CheckInstallation(void)
{
	return 1;
}

/* Bytes free on the current drive. */
long far GetFreeDiskSpace(void)
{
	int drive;
	struct dfree space;
	long bytes;

	drive = getdisk();
	getdfree(drive + 1, &space);
	if (space.df_sclus == -1)
		FatalError("Error reading free disk space");
	bytes = (long) space.df_avail * space.df_bsec * space.df_sclus;
	return bytes;
}

/* Extended memory, then disk space and DOS memory against INSTALL.PRM's requirements. */
unsigned char CheckSystem(void)
{
	int ok;
	VideoMode textMode;

	textMode.mode = 3;
	VoodooXmsBlock.base = OpenExtendedMemory();
	VoodooXmsBlock.free = GetExtendedMemorySize();
	VoodooXmsBlock.unusedFlag = 1;
	VoodooXmsBlock.used = 0;
	ok = VoodooXmsBlock.valid();
	if (!ok) {
		SetVideoMode(&textMode.mode);
		if (DetectEmsDriver()) {
			cout << "Please remove your expanded memory manager before running Ultima VII.\n";
			cout << "Refer to the Ultima VII reference manual for information concerning\n";
			cout << "EMS memory managers.\n";
		} else if (GetProcessorMode()) {
			cout << "Something has placed your machine in virtual 8086 mode.\n";
			cout << "Ultima VII cannot function in this mode.  Please remove the offending\n";
			cout << "software before running the game.\n";
			cout << "Refer to the Ultima VII reference manual for information concerning\n";
			cout << "system configuration.\n";
		}
		return 0;
	}
	XmsOpened = 1;
	if (VoodooXmsBlock.free < ExtendedMemoryNeeded) {
		SetVideoMode(&textMode.mode);
		cout << "Ultima VII requires " << ExtendedMemoryNeeded << " bytes of extended memory!\n";
		return 0;
	}
	if (GetFreeDiskSpace() < DiskSpaceNeeded) {
		SetVideoMode(&textMode.mode);
		cout << "Ultima VII requires " << DiskSpaceNeeded << " bytes of hard disk space!\n";
		return 0;
	}
	if (DosMemory < DosMemoryNeeded) {
		SetVideoMode(&textMode.mode);
		cout << "Ultima VII requires " << DosMemoryNeeded << " bytes of DOS memory!\n";
		return 0;
	}
	return ok;
}

void StartUp(void)
{
	InstallFatalHook();
	if (!CheckSystem()) {
		if (XmsOpened)
			ShutdownXMM();
		exit(1);
	}
}

void TellHowToPlay(void)
{
	RestoreSystem(0, 1);
	printf("Type ULTIMA7 to play Ultima VII\n");
	exit(1);
}

void RunIntroduction(void)
{
	RestoreSystem(0, 1);
	exit(LAUNCH_INTRO);
}

void RunEndgame(void)
{
	RestoreSystem(0, 1);
	exit(LAUNCH_ENDGAME);
}

/* Takes main's mouse handlers out of the driver, as exit() will not. */
void ReleaseMouse(void)
{
	if (MouseInput) {
		MouseInput->detach();
		MouseInput = 0;
	}
	if (MouseEvents) {
		MouseEvents->detach();
		MouseEvents = 0;
	}
}

void QuitToDos(void)
{
	ReleaseMouse();
	RestoreSystem(0, 1);
	exit(LAUNCH_THANKS);
}

void StartGame(void)
{
	RestoreSystem(0, 1);
	EnterFlatMode();
	exit(LAUNCH_GAME);
}

/* GAMEARGS.DAT tells the game who the new Avatar is. */
void WriteGameArgs(char *name, char sex)
{
	RestoreSystem(0, 1);
	char *args = new char[GAME_ARGS_SIZE];
	{
		DataFile file("gameargs.dat", FILE_CREATE);
		int length = strlen(name);

		*args = sex == 'M' ? 0 : 1;
		strcpy(args + 1, name);
		args[length + 1] = 0;
		file.write(args, (long) GAME_ARGS_SIZE);
	}
	EnterFlatMode();
	exit(LAUNCH_GAME);
}

unsigned char DirectoryExists(char *name)
{
	struct ffblk found;
	int result;

	result = findfirst(name, &found, FA_DIREC);
	return result == 0;
}

/* A saved game lives in GAMEDAT. */
unsigned char GameExists(void)
{
	return DirectoryExists(".\\gamedat");
}

void BeginNewGame(void)
{
	WriteGameArgs(AvatarName, AvatarSex);
}

void JourneyOnward(void)
{
	if (GameExists())
		StartGame();
	else
		ShowMessage("You must create a new game first!");
}

void ShowMessage(char *text)
{
	cout << text;
}

/* The launcher passes a mode letter, then optionally the credits' and the fades' speeds. */
void ParseCommandLine(int argc, char **argv)
{
	NotLaunched = 1;
	if (argc >= 2) {
		NotLaunched = 0;
		switch (argv[1][0]) {
		case 'N':
		case 'n':
			CleanUpOnly = 1;
			break;
		case 'V':
		case 'v':
			CleanUpOnly = 0;
			break;
		case 'C':
		case 'c':
			CreditsOnly = 1;
			break;
		case 'L':
		case 'l':
			LossOnly = 1;
			break;
		case 'M':
		case 'm':
			CheckOnly = 1;
			break;
		default:
			NotLaunched = 1;
			break;
		}
	}
	if (argc >= 3)
		StepTicks = atol(argv[2]);
	if (argc >= 4)
		FadeTicks = atol(argv[3]);
}

void Quit(void)
{
	QuitToDos();
}

void HandleKey(void)
{
	int key;

	key = getch();
	if (key == 0)
		key = getch() | KEY_EXTENDED;
	if (key == KEY_ALT_X)
		Quit();
	KeyPressed = 1;
}

void far CheckForVirus(void)
{
	int done;
	struct ffblk found;
	int key;

	done = findfirst("MAINMENU.EXE", &found, 0);
	if (done == 0 && found.ff_fsize != MAINMENU_SIZE) {
		printf(
			"****************************************\n"
			"* WARNING!  Ultima VII has detected    *\n"
			"* that the size of file MAINMENU.EXE   *\n"
			"* has changed.  Your computer may be   *\n"
			"* infected with a virus. Contact your  *\n"
			"* local retailer for information about *\n"
			"* virus disinfectant programs.         *\n"
			"****************************************\n"
			"* Hit the ESC key now to exit this     *\n"
			"* program.  If you proceed your        *\n"
			"* Ultima VII game may become corrupted.*\n"
			"****************************************\n");
		key = getch();
		if (key == 0)
			key = getch() | KEY_EXTENDED;
		if (key == KEY_ESCAPE)
			exit(1);
	}
}

void main(int argc, char **argv)
{
	int irq, port, dma;
	int choice;

	CheckForVirus();
	DosMemory = GetFreeDosMemory();
	DosMemory += 1648;
	if (FileExists("install.prm"))
		InstallInfo.load("install.prm");
	DosMemoryNeeded = InstallInfo.dosMemory;
	ExtendedMemoryNeeded = InstallInfo.extendedMemory;
	DiskSpaceNeeded = InstallInfo.diskSpace;
	ParseCommandLine(argc, argv);
	ConfigureSound("u7.cfg", DataPath(GamedatPath, "options.cfg"), &irq, &port, &MusicDriver, &dma);
	if (CheckOnly) {
		if (!CheckSystem()) {
			if (XmsOpened)
				ShutdownXMM();
			exit(1);
		}
		ShutdownXMM();
		exit(LAUNCH_INTRO);
	}
	Devices.init(20);
	TimerDevice timer(&SystemTimer);
	Devices.add(&timer);
	FlatModeFlags = 1;
	if (CleanUpOnly) {
		IsFlatModeBlocked();
		LeaveFlatMode();
		exit(0);
	}
	StartUp();
	if (NotLaunched)
		TellHowToPlay();
	InitEnvironment();
	EventQueue queue((char *) new MouseEvent[MOUSE_EVENTS], MOUSE_EVENTS);
	MouseEvents = &queue;
	MouseEvents->install();
	MouseDevice mouse;
	MouseInput = &mouse;
	LoadPointerShapes(MouseInput, DataPath(StaticPath, "mainshp.flx"), SHAPE_POINTERS, 160, 100);
	InstallCursorHook(MouseInput);
	SetCursorTarget(&ScreenView);
	HideCursor();
	if (LossOnly)
		MusicDriver = 0;
	SoundDevice sound(&Music);
	Devices.add(&sound);
	sound.device = MusicDriver;
	sound.setTimbreFile(DataPath(StaticPath, "u7intro.tim"));
	sound.setDriverFile(DataPath(StaticPath, "u7strax.drv"));
	Devices.open(DEVICE_SOUND);
	if (SpeechEnabled)
		SpeechCard.init(SPEECH_BUFFER, SPEECH_RATE, port, irq, dma);
	MenuFont.load(DataPath(StaticPath, "mainshp.flx"), SHAPE_FONT);
	MenuFont.target = &ScreenView;
	MenuFont.spacing = 1;
	if (CreditsOnly) {
		FillView(&ScreenView, 0);
		ShowCredits(StepTicks, 3);
		RestoreSystem(0, 1);
		EnterFlatMode();
		exit(LAUNCH_THANKS);
	}
	if (LossOnly) {
		FillView(&ScreenView, 0);
		ShowTheEnd(THE_END_PAUSE);
		RestoreSystem(0, 1);
		EnterFlatMode();
		exit(LAUNCH_THANKS);
	}
	for (;;) {
		choice = RunMainMenu(100, 250, 230);
		if (choice == CHOICE_QUIT)
			break;
		switch (choice) {
		case CHOICE_CREDITS:
			ShowCredits(StepTicks, 1);
			break;
		case CHOICE_QUOTES:
			ShowQuotes(StepTicks, 1);
			ShowButterfly(3);
			break;
		case CHOICE_NEW_GAME:
			ReleaseMouse();
			BeginNewGame();
			break;
		case CHOICE_JOURNEY:
			ReleaseMouse();
			JourneyOnward();
			break;
		case CHOICE_INTRO:
			ReleaseMouse();
			RunIntroduction();
			break;
		case CHOICE_ENDGAME:
			ReleaseMouse();
			RunEndgame();
			break;
		}
		while (kbhit())
			getch();
	}
	RestoreSystem(1, 1);
	exit(0);
}
