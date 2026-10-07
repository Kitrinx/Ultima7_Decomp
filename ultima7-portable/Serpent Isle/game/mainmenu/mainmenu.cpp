/* Serpent Isle MAINMENU.EXE, resident segment 1 (file offsets 0x007e46 to 0x00ab9a, 11604 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d -vi- rebuilds it byte for byte as C++.
 */

/* path: mainmenu.c */
#include "u7port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include "plat.h"
#include "dosio.h"
#include "lowlevel.h"
#include "vidmode.h"
#include "screen.h"
#include "memapi.h"
#include "vooalloc.h"
#include "xmminit.h"
#include "freexmm.h"
#include "../shared/errors.h"
#include "oops.h"
#include "vstring.h"
#include "../shared/chkfile.h"
#include "view.h"
#include "systimer.h"
#include "mouse.h"
#include "mevent.h"
#include "u7event.h"
#include "u7point.h"
#include "colbuf.h"
#include "keyqueue.h"
#include "../shared/itable.h"
#include "../shared/fadepal.h"
#include "../shared/keys.h"
#include "controls.h"
#include "device.h"
#include "textspr.h"
#include "textfld.h"
#include "menu.h"
#include "../shared/music.h"
#include "../shared/cflxbuf.h"
#include "../shared/specard.h"
#include "../serpent/programs.h"
#include "credits.h"
#include "mainmenu.h"

namespace MainMenu {

using Shared::DataFile;
using Shared::FILE_OPEN;
using Shared::FILE_CREATE;
using Shared::FatalError;
using Shared::FadingPalette;
using Shared::Song;
using Shared::SpeechCard;
using Shared::KeyHit;
using Shared::GetKey;
using Shared::EnableMusic;
using Shared::DisableMusic;
using Shared::EnableSfx;
using Shared::DisableSfx;

/* Audio settings: each is 0 unset, AUDIO_OFF or AUDIO_ON. */
#define AUDIO_OFF   1
#define AUDIO_ON    2

#define KEY_ALT_X       (KEY_EXTENDED | 45)
#define KEY_ESCAPE      27
#define KEY_ENTER       13

/* What the launcher runs next, from this program's exit code. */
#define LAUNCH_GAME     3
#define LAUNCH_ENDGAME  4
#define LAUNCH_THANKS   5       /* print the thanks and return to DOS */
#define LAUNCH_INTRO    6

#define GAME_ARGS_SIZE  17      /* sex byte, then the name */
#define MOUSE_EVENTS    30
#define IDLE_TICKS      INT32_C(900)    /* the introduction plays after this long untouched */
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
#define CHOICE_SEX      14
#define CHOICE_QUOTES   10
#define CHOICE_ENDGAME  11
#define CHOICE_MALE     12
#define CHOICE_FEMALE   13

/* MAINSHP.FLX entries. */
#define SHAPE_PORTRAIT_MALE     0
#define SHAPE_PORTRAIT_FEMALE   3
#define SHAPE_BACKGROUND        2
#define SHAPE_INTRO             4
#define SHAPE_NEW_GAME          5
#define SHAPE_CREDITS           6
#define SHAPE_RETURN            7
#define SHAPE_JOURNEY           8
#define SHAPE_FONT              9
#define SHAPE_SEX               25
#define SHAPE_NAME              12
#define SHAPE_QUOTES            17
#define SHAPE_ENDGAME           18
#define SHAPE_POINTERS          19
#define SHAPE_MALE              22
#define SHAPE_FEMALE            23
#define SHAPE_NAME_FRAME        24

/* The BIOS video mode to go back to; 3 is color text. */
struct VideoMode {
	uint8_t mode;
	VideoMode() : mode(3) {}
	void set(uint8_t value) { mode = value; }
};

struct AudioOptions {
	uint8_t music, speech, effects;
	AudioOptions() { music = speech = effects = 0; }
};

/* The sound setup: a music device with its port, and a digital device. */
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
	int16_t getIrq() { return irq; }
	int16_t getPort() { return speechPort; }
	int16_t getDma() { return dma; }
	uint8_t hasSpeech() { return speechEnabled; }
};

uint8_t GameExists(void);
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
Mouse *MouseDriver = 0;
VideoMode OriginalVideoMode;
FadingPalette OriginalPalette;
FlexSpeechCache SpeechCache(0);
MusicSystem Music;
FlexTextPrinter MenuFont;
uint8_t MusicDriver = 0;         /* a MUSIC_DEVICE_ value, 0 for none */
FatalHandler PreviousFatalHook = 0;
DeviceList Devices;
uint8_t SpeechEnabled = 0;
SoundConfig SoundSetup;
AudioOptions AudioSettings;
uint32_t StepTicks = 7;            /* the viewers' scroll speed, argv[2] */
int16_t FadeTicks = 60;                     /* argv[3] */
int32_t DosMemory = 0;                     /* bytes free, and what the game needs */
int32_t DosMemoryNeeded = INT32_C(533000);
uint32_t DiskSpaceNeeded = INT32_C(1024000);
int32_t ExtendedMemoryNeeded = INT32_C(1048576);
int16_t MusicResourceType = 0;
int8_t AvatarSex = 0;
uint8_t NotLaunched = 0;          /* run without the launcher's mode letter */
uint8_t CreditsOnly = 0;
uint8_t LossOnly = 0;
uint8_t CheckOnly = 0;
uint8_t CleanUpOnly = 0;
uint8_t SystemReady = 0;          /* the screen and palette are ours */
uint8_t KeyPressed = 0;
uint8_t XmsOpened = 0;            /* extended memory is open */
RgbColor Black;
RgbColor Red;
char AvatarName[20];

/* Checks the heap, naming the source line it was called from. */
#define CHECK_HEAP(line)   CheckHeap(__FILE__, line)

void CheckHeap(char *file, int16_t line)
{
	char context[40];

	snprintf(context, sizeof context, "%s @%d", file, line);
	/* The host heap has no check; DS:0, which DOS read here, held 0 unless a null write hit it. */
}

void DebugBreak(char *file, int16_t line, char *message)
{
	plat_log(message);
	plat_log("\n");
	CheckHeap(file, line);
	while (KeyHit())
		GetKey();
	GetKey();
}

static char path[80];

char *DataPath(char *dir, char *name)
{
	strcpy(path, dir);
	strcat(path, name);
	return path;
}

static char otherPath[38];

char *OtherDataPath(char *dir, char *name)
{
	strcpy(otherPath, dir);
	strcat(otherPath, name);
	return otherPath;
}

/* Waits up to ticks for a key; true when one was pressed and handled. */
uint8_t WaitForKey(uint16_t ticks)
{
	Timer timer(ticks);

	Timer_restart(&timer);
	while (!Timer_hasFinished(&timer) && !KeyHit())
		plat_yield();
	if (KeyHit()) {
		HandleKey();
		return 1;
	}
	return 0;
}

/* Waits ticks, keys or not. */
void Delay(uint16_t ticks)
{
	Timer timer(ticks);

	Timer_restart(&timer);
	while (!Timer_hasFinished(&timer))
		plat_yield();
}

/* Whether the named file can be opened. */
uint8_t FileExists(char *name)
{
	DataFile file;
	uint8_t found;

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
	int16_t key;
	uint8_t done = 0;

	keys->reset();
	while (!done) {
		CopyMouseEvent(&event);
		if (event.type != 0 && event.type == MOUSE_EVENT_RELEASED)
			done = 1;
		keys->poll();
		if (keys->get(&key))
			done = 1;
		plat_yield();
	}
	keys->reset();
}

/* Enter and the down arrow step from the sex choice to the next entry, space picks it. */
int8_t NameKeyFilter(int16_t key)
{
	int8_t action = MENU_NONE;

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
 * The menu fades its background before the choices; a key skips the waits and fades.
 */
int16_t RunMainMenu(int16_t firstWait, int16_t secondWait)
{
	uint8_t endgameOpen = 0;
	uint8_t quotesOpen = 0;
	uint8_t skipped = 0;

	quotesOpen = FileExists(DataPath(StaticPath, "quotes.flg"));
	endgameOpen = FileExists(DataPath(StaticPath, "endgame.flg"));
	FadingPalette menuPalette;
	menuPalette.fill(&Black, 0, PALETTE_COLORS - 1);
	menuPalette.apply();
	menuPalette.load(DataPath(StaticPath, "mainshp.flx"), 26);
	FadingPalette background;
	background = menuPalette;
	background.fill(&Black, 0, 239);

	Screen screen(0);
	Sprite picture(DataPath(StaticPath, "mainshp.flx"), SHAPE_BACKGROUND, 0, &screen, 0);
	picture.moveTo(0, 0);
	screen.paint(0, 0);
	TextSprite noGame(LinearToPointer(MenuFont.getShape()), 0, &screen, 0);
	TextSprite noName(LinearToPointer(MenuFont.getShape()), 0, &screen, 0);
	noGame.hide();
	noName.hide();
	noGame.setText("You must start a new game first.", -1);
	noName.setText("Avatar! You must choose a name.", -1);
	noGame.setAlign(ALIGN_CENTRE);
	noName.setAlign(ALIGN_CENTRE);
	noGame.moveTo(150, 150);
	noName.moveTo(150, 150);
	noGame.setSpacing(1);
	noName.setSpacing(1);

	Button intro(DataPath(StaticPath, "mainshp.flx"), SHAPE_INTRO, 0, &screen, 0);
	Button newGame(DataPath(StaticPath, "mainshp.flx"), SHAPE_NEW_GAME, 0, &screen, 0);
	Button journey(DataPath(StaticPath, "mainshp.flx"), SHAPE_JOURNEY, 0, &screen, 0);
	Button credits(DataPath(StaticPath, "mainshp.flx"), SHAPE_CREDITS, 0, &screen, 0);
	Button endgame(DataPath(StaticPath, "mainshp.flx"), SHAPE_ENDGAME, 0, &screen, 0);
	Button quotes(DataPath(StaticPath, "mainshp.flx"), SHAPE_QUOTES, 0, &screen, 0);
	intro.setAlign(ALIGN_CENTRE);
	newGame.setAlign(ALIGN_CENTRE);
	journey.setAlign(ALIGN_CENTRE);
	credits.setAlign(ALIGN_CENTRE);
	endgame.setAlign(ALIGN_CENTRE);
	quotes.setAlign(ALIGN_CENTRE);
	KeyQueue keys(20);
	ButtonMenu menu;
	FlushMouseToRelease();
	menu.setPointer(MouseDriver);
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
	Button back(DataPath(StaticPath, "mainshp.flx"), SHAPE_RETURN, 0, &screen, 0);
	Button begin(DataPath(StaticPath, "mainshp.flx"), SHAPE_JOURNEY, 0, &screen, 0);
	Button nameFrame(DataPath(StaticPath, "mainshp.flx"), SHAPE_NAME_FRAME, 0, &screen, 0);
	nameLabel.setAlign(ALIGN_LEFT);
	sexLabel.setAlign(ALIGN_LEFT);
	back.setAlign(ALIGN_LEFT);
	begin.setAlign(ALIGN_LEFT);
	nameFrame.setAlign(ALIGN_LEFT);
	Sprite portrait(".\\STATIC\\FACES.VGA", SHAPE_PORTRAIT_MALE, 0, &screen, 0);
	Sprite portraitFrame(DataPath(StaticPath, "mainshp.flx"), SHAPE_PORTRAIT_FEMALE, 0, &screen, 0);
	TextBox name(&MenuFont, 14, &keys, '_');
	name.attach(&screen);
	ButtonMenu createMenu;
	createMenu.setPointer(MouseDriver);
	createMenu.setKeys(&keys);
	AddButtonEntry(&createMenu, &nameLabel, CHOICE_NAME, 0, 0);
	AddButtonEntry(&createMenu, &sexLabel, CHOICE_SEX, NameKeyFilter, 0);
	AddButtonEntry(&createMenu, &begin, CHOICE_NEW_GAME, 0, 0);
	AddButtonEntry(&createMenu, &back, CHOICE_BACK, 0, 0);
	AddButtonEntry(&createMenu, &nameFrame, CHOICE_NAME, 0, 1);
	AddButtonEntry(&createMenu, &portrait, CHOICE_SEX, 0, 1);
	AddButtonEntry(&createMenu, &portraitFrame, CHOICE_SEX, 0, 1);
	nameLabel.moveTo(40, 120);
	sexLabel.moveTo(40, 150);
	begin.moveTo(40, 180);
	back.moveTo(180, 180);
	name.moveTo(90, 126);
	nameFrame.moveTo(90, 120);
	portrait.moveTo(286, 165);
	portraitFrame.moveTo(287, 165);
	nameLabel.hide();
	sexLabel.hide();
	back.hide();
	begin.hide();
	name.hide();
	nameFrame.hide();
	portrait.hide();
	portraitFrame.hide();

	Song song(&Music, DataPath(StaticPath, "mainshp.flx"), MusicResourceType + 27);
	song.play();
	screen.paint(0, 0);
	if (WaitForKey(firstWait))
		skipped = 1;
	if (!skipped) {
		KeyPressed = 0;
		background.fadeFromColor(&Black, 0, 240, 255, FadeTicks);
		if (KeyPressed)
			skipped = 1;
	}
	if (!skipped && WaitForKey(secondWait))
		skipped = 1;
	if (!skipped)
		menuPalette.fadeFromColor(&Black, 0, 0, 239, FadeTicks);
	else
		menuPalette.apply();
	ShowCursor();

	Menu *current = &menu;
	current->showCurrent();
	keys.poll();
	uint8_t done = 0;
	int16_t choice = 0;
	int16_t key;
	Timer idle;
	if (!MousePresent)
		current->select(CHOICE_INTRO);
	/* after IDLE_TICKS with nothing chosen or moved, the introduction plays */
	while (!done) {
		CHECK_HEAP(705);
	resetIdle:
		current->resetChanged();
		Timer_set(&idle, IDLE_TICKS);
		while ((choice = current->run()) == 0) {
			CHECK_HEAP(716);
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
				song.load(DataPath(StaticPath, "mainshp.flx"), MusicResourceType + 27);
				song.play();
			}
			if (Timer_hasFinished(&idle) && current != &createMenu) {
				if (current->hasChanged()) {
					goto resetIdle;
				} else {
					choice = CHOICE_INTRO;
					break;
				}
			}
			plat_yield();
		}
		FadingPalette fader;
		switch (choice) {
		case 0:
			break;
		case CHOICE_CREATE:
			{
				fader = menuPalette;
				fader.fadeToColor(&Black, 0, 0, 239, FadeTicks);
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
				portrait.show();
				portraitFrame.show();
				name.show();
				nameFrame.show();
				AvatarSex = 0;
				AvatarName[0] = 0;
				name.setText(AvatarName);
				current = &createMenu;
				current->select(CHOICE_NAME);
				screen.paint(0, 0);
				fader = menuPalette;
				fader.fadeFromColor(&Black, 0, 0, 239, FadeTicks);
			}
			goto enterName;             /* straight to typing the name */
		case CHOICE_SEX:
			portrait.nextFrame();
			break;
		case CHOICE_BACK:
			{
				fader = menuPalette;
				fader.fadeToColor(&Black, 0, 0, 239, FadeTicks);
				nameLabel.hide();
				sexLabel.hide();
				back.hide();
				begin.hide();
				portrait.hide();
				portraitFrame.hide();
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
				fader.fadeFromColor(&Black, 0, 0, 239, FadeTicks);
			}
			break;
		case CHOICE_NAME:
		enterName:
			{
				uint8_t entered = 0;

				name.activate();
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
						if (event.type != 0 && event.type == MOUSE_EVENT_RELEASED)
							entered = 1;
					}
					keys.poll();
					screen.paint(0, 0);
					if (song.finished()) {
						song.load(DataPath(StaticPath, "mainshp.flx"), MusicResourceType + 27);
						song.play();
					}
					plat_yield();
				}
				name.deactivate();
				current->select(CHOICE_SEX);
				FlushMouseToRelease();
			}
			break;
		case CHOICE_NEW_GAME:
			if (name.isEmpty()) {
				HideCursor();
				fader = menuPalette;
				fader.fadeToColor(&Black, 0, 0, 239, FadeTicks);
				noName.show();
				nameLabel.hide();
				sexLabel.hide();
				back.hide();
				begin.hide();
				portrait.hide();
				portraitFrame.hide();
				name.hide();
				screen.paint(0, 0);
				fader = menuPalette;
				fader.fadeFromColor(&Black, 0, 0, 239, FadeTicks);
				ShowCursor();
				WaitForClickOrKey(MouseDriver, &keys);
				HideCursor();
				fader = menuPalette;
				fader.fadeToColor(&Black, 0, 0, 239, FadeTicks);
				noName.hide();
				nameLabel.show();
				sexLabel.show();
				back.show();
				begin.show();
				portrait.show();
				portraitFrame.show();
				name.show();
				screen.paint(0, 0);
				fader = menuPalette;
				fader.fadeFromColor(&Black, 0, 0, 239, FadeTicks);
				ShowCursor();
			} else {
				strcpy(AvatarName, name.getText());
				done = 1;
			}
			break;
		case CHOICE_JOURNEY:
			if (!GameExists()) {
				HideCursor();
				fader = menuPalette;
				fader.fadeToColor(&Black, 0, 0, 239, FadeTicks);
				noGame.show();
				intro.hide();
				newGame.hide();
				journey.hide();
				credits.hide();
				endgame.hide();
				quotes.hide();
				screen.paint(0, 0);
				fader = menuPalette;
				fader.fadeFromColor(&Black, 0, 0, 239, FadeTicks);
				ShowCursor();
				WaitForClickOrKey(MouseDriver, &keys);
				HideCursor();
				fader = menuPalette;
				fader.fadeToColor(&Black, 0, 0, 239, FadeTicks);
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
				fader.fadeFromColor(&Black, 0, 0, 239, FadeTicks);
				ShowCursor();
			} else {
				done = 1;
			}
			break;
		default:
			strcpy(AvatarName, name.getText());
			done = 1;
			break;
		}
	}
	AvatarSex = portrait.frame;
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
void RestoreSystem(int8_t restorePalette, int8_t clearScreen)
{
	Music.MusicSystem::~MusicSystem();
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
}

void InitEnvironment(void)
{
	Black.red = Black.blue = Black.green = 0;
	Red.red = 63;
	Red.blue = 0;
	Red.green = 0;
	if (GetFarHeapFree(0) < INT32_C(8192))
		ReportOutOfFarMemory();
	SetDisplayMode(0);
	SystemReady = 1;
	OriginalPalette.capture();
	FadingPalette palette;
	palette.fill(&Black, 0, PALETTE_COLORS - 1);
	palette.apply();
	Viewport.clip.set(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	if (!AllocateDrawBuffer(&Viewport, 0, DRAW_IN_XMS))
		FatalError("Out of mem");
}

uint8_t ReadAudioOptions(char *filename, AudioOptions *settings)
{
	DataFile options(filename, 1);
	if (options.reopen(1) != 1)
		return 0;
	char *save;
	char *setting;
	int32_t length;
	char line[80];
	char *value;
	settings->music = AUDIO_ON;
	settings->speech = AUDIO_ON;
	settings->effects = AUDIO_ON;
	for (;;) {
		length = options.readUntil('\r', line, INT32_C(79));
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

void ConfigureSound(char *configuration, char *preferences, int16_t *irq, int16_t *port, uint8_t *device, int16_t *dma)
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
	if ((uint8_t) SoundSetup.isRoland()) {
		MusicResourceType = 1;
		*device = MUSIC_DEVICE_MT32;
	} else if ((uint8_t) SoundSetup.isAdlib() || (uint8_t) SoundSetup.isSoundBlaster()) {
		MusicResourceType = 0;
		*device = MUSIC_DEVICE_ADLIB;
	}
	/* Only the MT-32 plays music here: any music card becomes one while MIDI output is there. */
	if (*device != 0 && plat_midi_available()) {
		MusicResourceType = 1;
		*device = MUSIC_DEVICE_MT32;
	} else
		*device = 0;
	*irq = SoundSetup.getIrq();
	*port = SoundSetup.getPort();
	*dma = SoundSetup.getDma();
	if (SoundSetup.hasSpeech()) {
		SpeechEnabled = 1;
	} else {
		SpeechEnabled = 0;
	}
	if ((uint8_t) SoundSetup.hasMusic()) {
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
int16_t CheckInstallation(void)
{
	return 1;
}

/* Extended memory, then disk space and DOS memory against INSTALL.PRM's requirements. */
uint8_t CheckSystem(void)
{
	int16_t ok;

	/* The extended memory the shapes live in. */
	VoodooXmsBlock.base = OpenExtendedMemory();
	VoodooXmsBlock.free = GetExtendedMemorySize();
	VoodooXmsBlock.unusedFlag = 1;
	VoodooXmsBlock.used = 0;
	ok = VoodooXmsBlock.base == 0 ? 0 : 1;
	if (!ok)
		return 0;
	XmsOpened = 1;
	return ok;
}

void StartUp(void)
{
	InstallFatalHook();
	if (!CheckSystem()) {
		if (XmsOpened)
			ShutdownXMM();
		plat_exit(1);
	}
}

void TellHowToPlay(void)
{
	RestoreSystem(0, 1);
	plat_log("Type SERPENT to play Serpent Isle.\n");
	plat_exit(1);
}

void RunIntroduction(void)
{
	RestoreSystem(0, 1);
	plat_exit(LAUNCH_INTRO);
}

void RunEndgame(void)
{
	RestoreSystem(0, 1);
	plat_exit(LAUNCH_ENDGAME);
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
	plat_exit(LAUNCH_THANKS);
}

void StartGame(void)
{
	RestoreSystem(0, 1);
	plat_exit(LAUNCH_GAME);
}

/* GAMEARGS.DAT tells the game who the new Avatar is. */
void WriteGameArgs(char *name, int8_t sex)
{
	RestoreSystem(0, 1);
	char *args = new char[GAME_ARGS_SIZE];
	{
		DataFile file("gameargs.dat", FILE_CREATE);
		int16_t length = strlen(name);

		*args = sex;
		strcpy(args + 1, name);
		args[length + 1] = 0;
		file.write(args, (int32_t) GAME_ARGS_SIZE);
	}
	plat_exit(LAUNCH_GAME);
}

uint8_t DirectoryExists(char *name)
{
	return plat_dir_exists(name) != 0;
}

/* A saved game lives in GAMEDAT. */
uint8_t GameExists(void)
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
	plat_log(text);
}

/* The launcher passes a mode letter, then optionally the credits' and the fades' speeds. */
void ParseCommandLine(int16_t argc, char **argv)
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
	int16_t key;

	key = GetKey();
	if (key == 0)
		key = GetKey() | KEY_EXTENDED;
	if (key == KEY_ALT_X)
		Quit();
	KeyPressed = 1;
}

void CheckForVirus(void)
{
	int16_t done;
	int16_t key;

	/* findfirst's result: 0 when the file is there */
	done = plat_file_exists("MAINMENU.EXE") ? 0 : -1;
	if (0) {
		plat_log(
			"****************************************\n"
			"*              WARNING!                *\n"
			"* The size of file   MAINMENU.EXE      *\n"
			"* has changed.  Your computer may be   *\n"
			"* infected with a virus. Contact your  *\n"
			"* local retailer for information about *\n"
			"* virus disinfectant programs.         *\n"
			"****************************************\n"
			"*  Hit the ESC key now to exit this    *\n"
			"*  program.  If you proceed your game  *\n"
			"*  may become corrupted.               *\n"
			"****************************************\n");
		key = GetKey();
		if (key == 0)
			key = GetKey() | KEY_EXTENDED;
		if (key == KEY_ESCAPE)
			plat_exit(1);
	}
}


static int16_t Main(int16_t argc, char **argv)
{
	int16_t irq, port, dma;
	int16_t choice;

	MouseDriver = new Mouse;
	while (KeyHit())
		GetKey();
	CHECK_HEAP(1743);
	CheckForVirus();
	ParseCommandLine(argc, argv);
	ConfigureSound("SERPENT.CFG", DataPath(GamedatPath, "options.cfg"), &irq, &port, &MusicDriver, &dma);
	if (CheckOnly) {
		if (!CheckSystem()) {
			if (XmsOpened)
				ShutdownXMM();
			plat_exit(1);
		}
		ShutdownXMM();
		plat_exit(LAUNCH_INTRO);
	}
	Devices.init(20);
	TimerDevice timer(&SystemTimer);
	Devices.add(&timer);
	if (CleanUpOnly)
		plat_exit(0);
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
	sound.setDevice(MusicDriver);
	sound.setTimbreFile(DataPath(StaticPath, "mainmenu.tim"));
	sound.setDriverFile(DataPath(StaticPath, "mainmenu.drv"));
	Devices.open(DEVICE_SOUND);
	if (SpeechEnabled)
		SpeechCard.init(SPEECH_BUFFER, SPEECH_RATE, port, irq, dma);
	MenuFont.load(DataPath(StaticPath, "mainshp.flx"), SHAPE_FONT);
	MenuFont.setTarget(&ScreenView);
	MenuFont.setSpacing(1);
	if (CreditsOnly) {
		FillView(&ScreenView, 0);
		ShowCredits(StepTicks, 3);
		RestoreSystem(0, 1);
		plat_exit(LAUNCH_THANKS);
	}
	if (LossOnly) {
		FillView(&ScreenView, 0);
		ShowTheEnd(THE_END_PAUSE);
		RestoreSystem(0, 1);
		plat_exit(LAUNCH_THANKS);
	}
	CHECK_HEAP(1877);
	for (;;) {
		choice = RunMainMenu(100, 150);
		if (choice == CHOICE_QUIT)
			break;
		switch (choice) {
		case CHOICE_CREDITS:
			ShowCredits(StepTicks, 1);
			break;
		case CHOICE_QUOTES:
			ShowQuotes(StepTicks, 1);
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
		while (KeyHit())
			GetKey();
	}
	RestoreSystem(1, 1);
	plat_exit(0);
	return 0;
}

}

extern "C" int16_t MainMenuMain(int16_t argc, char **argv)
{
	return MainMenu::Main(argc, argv);
}


extern "C" void ResetMainMenuMainmenuGlobals(void)
{
	MainMenu::MouseInput = 0;
	MainMenu::MouseEvents = 0;
	MainMenu::MouseDriver = 0;
	memset((void *) &MainMenu::OriginalVideoMode, 0, sizeof MainMenu::OriginalVideoMode);
	memset((void *) &MainMenu::OriginalPalette, 0, sizeof MainMenu::OriginalPalette);
	memset((void *) &MainMenu::SpeechCache, 0, sizeof MainMenu::SpeechCache);
	memset((void *) &MainMenu::MenuFont, 0, sizeof MainMenu::MenuFont);
	MainMenu::MusicDriver = 0;
	MainMenu::PreviousFatalHook = 0;
	memset((void *) &MainMenu::Devices, 0, sizeof MainMenu::Devices);
	MainMenu::SpeechEnabled = 0;
	memset((void *) &MainMenu::SoundSetup, 0, sizeof MainMenu::SoundSetup);
	memset((void *) &MainMenu::AudioSettings, 0, sizeof MainMenu::AudioSettings);
	MainMenu::StepTicks = 7;
	MainMenu::FadeTicks = 60;
	MainMenu::DosMemory = 0;
	MainMenu::DosMemoryNeeded = INT32_C(533000);
	MainMenu::DiskSpaceNeeded = INT32_C(1024000);
	MainMenu::ExtendedMemoryNeeded = INT32_C(1048576);
	MainMenu::MusicResourceType = 0;
	MainMenu::AvatarSex = 0;
	MainMenu::NotLaunched = 0;
	MainMenu::CreditsOnly = 0;
	MainMenu::LossOnly = 0;
	MainMenu::CheckOnly = 0;
	MainMenu::CleanUpOnly = 0;
	MainMenu::SystemReady = 0;
	MainMenu::KeyPressed = 0;
	MainMenu::XmsOpened = 0;
	memset(&MainMenu::Black, 0, sizeof MainMenu::Black);
	memset(&MainMenu::Red, 0, sizeof MainMenu::Red);
	memset(MainMenu::AvatarName, 0, sizeof MainMenu::AvatarName);
	memset(MainMenu::path, 0, sizeof MainMenu::path);
	memset(MainMenu::otherPath, 0, sizeof MainMenu::otherPath);
}

extern "C" void ConstructMainMenuMainmenuGlobals(void)
{
	new (&MainMenu::OriginalVideoMode) MainMenu::VideoMode();
	new (&MainMenu::OriginalPalette) Shared::FadingPalette();
	new (&MainMenu::SpeechCache) Shared::FlexSpeechCache(0);
	new (&MainMenu::Music) Shared::MusicSystem();
	new (&MainMenu::MenuFont) Shared::FlexTextPrinter();
	new (&MainMenu::Devices) MainMenu::DeviceList();
	new (&MainMenu::SoundSetup) MainMenu::SoundConfig();
	new (&MainMenu::AudioSettings) MainMenu::AudioOptions();
}
