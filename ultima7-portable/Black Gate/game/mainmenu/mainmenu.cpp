/* Black Gate MAINMENU.EXE module MAINMENU: the main menu, the new-game screen, and the start-up
 * the launcher runs before and after the game.
 *
 *   m  set up and check the system, then exit 6 (introduction)
 *   v  the menu: exits 3 (play), 4 (endgame), 6 (introduction) or 5 (Alt-X)
 *   n  clean up after the game, exit 0
 *   c  the credits, exit 5
 *   l  the ending screen, exit 5
 *
 * A fatal error exits 1. The DOS-only checks (memory, disk, EMS, CPU mode, virus, INSTALL.PRM)
 * are gone.
 */

#include "u7port.h"
#include "plat.h"
#include "dosio.h"
#include "lowlevel.h"
#include "vidmode.h"
#include "screen.h"
#include "memapi.h"
#include "vooalloc.h"
#include "xmminit.h"
#include "freexmm.h"
#include "oops.h"
#include "chkfile.h"
#include "view.h"
#include "systimer.h"
#include "mouse.h"
#include "mevent.h"
#include "u7event.h"
#include "preload.h"
#include "colbuf.h"
#include "cflxbuf.h"
#include "specard.h"
#include "midiplay.h"
#include "../ultima7/programs.h"
#include "../shared/errors.h"
#include "../shared/fadepal.h"
#include "../shared/music.h"
#include "../shared/itable.h"
#include "../shared/keys.h"
#include "keyqueue.h"
#include "device.h"
#include "controls.h"
#include "cursor.h"
#include "textspr.h"
#include "textfld.h"
#include "menu.h"
#include "credits.h"
#include "mainmenu.h"
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

namespace MainMenu {

using Shared::FadingPalette;
using Shared::Song;
using Shared::KeyHit;
using Shared::GetKey;
using Shared::FatalError;

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
#define IDLE_TICKS      900     /* the introduction plays after this long untouched */
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

uint8_t GameExists(void);
void ShowMessage(char *text);

char *const GamedatPath = ".\\gamedat\\";
char *const StaticPath = ".\\static\\";
static MouseDevice *MouseInput = 0;
static EventQueue *MouseEvents = 0;
static Mouse *MouseDriver = 0;
static FadingPalette OriginalPalette;
FlexSpeechCache MenuSpeech(0);
MusicSystem Music;
FlexTextPrinter MenuFont;
static uint8_t MusicDriver = 0;         /* a MUSIC_DEVICE_ value, 0 for none */
static FatalHandler PreviousFatalHook = 0;
static DeviceList Devices;
uint8_t SpeechEnabled = 0;
static SoundConfig SoundSetup;
static AudioOptions AudioSettings;
static uint32_t StepTicks = 7;          /* the viewers' scroll speed, argv[2] */
int16_t FadeTicks = 60;                 /* argv[3] */
char AvatarSex = 'M';
static uint8_t NotLaunched = 0;         /* run without the launcher's mode letter */
static uint8_t CreditsOnly = 0;
static uint8_t LossOnly = 0;
static uint8_t CheckOnly = 0;
static uint8_t CleanUpOnly = 0;
static uint8_t SystemReady = 0;         /* the screen and palette are ours */
uint8_t KeyPressed = 0;
static uint8_t XmsOpened = 0;           /* extended memory is open */
RgbColor Black;
RgbColor Red;
static char MusicFlexName[80];
char *const MusicFlex = MusicFlexName;
char AvatarName[20];

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

static void WaitForClickOrKey(Mouse *, KeyQueue *keys)
{
	MouseEvent event;
	int16_t key;
	uint8_t done = 0;

	keys->reset();
	while (!done) {
		CopyMouseEvent(&event);
		if (event.type == MOUSE_EVENT_RELEASED)
			done = 1;
		keys->poll();
		if (keys->get(&key))
			done = 1;
		plat_yield();
	}
	keys->reset();
}

/* Enter and the down arrow step from the sex choice to the next entry, space picks it. */
static char NameKeyFilter(int16_t key)
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
static int16_t RunMainMenu(int16_t firstWait, int16_t secondWait, int16_t thirdWait)
{
	uint8_t endgameOpen = 0;
	uint8_t quotesOpen = 0;
	uint8_t skipped = 0;
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
	(void) palettes;

	Screen screen(0);
	Sprite picture(DataPath(StaticPath, "mainshp.flx"), SHAPE_BACKGROUND, 0, &screen, 0);
	picture.moveTo(0, 0);
	screen.paint(0, 0);
	TextSprite noGame(LinearToPointer(MenuFont.shape), 0, &screen, 0);
	TextSprite noName(LinearToPointer(MenuFont.shape), 0, &screen, 0);
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
	createMenu.setPointer(MouseDriver);
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
	uint8_t done = 0;
	int16_t choice = 0;
	int16_t key;
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
			plat_yield();
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
				uint8_t entered = 0;

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
						if (event.type == MOUSE_EVENT_RELEASED)
							entered = 1;
					}
					keys.poll();
					screen.paint(0, 0);
					if (song.finished()) {
						song.load(MusicFlex, 3);
						song.play();
					}
					plat_yield();
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
				WaitForClickOrKey(MouseDriver, &keys);
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
				WaitForClickOrKey(MouseDriver, &keys);
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
static void OnFatalError(void)
{
	RestoreSystem(1, 1);
	if (PreviousFatalHook)
		PreviousFatalHook();
}

static void InstallFatalHook(void)
{
	PreviousFatalHook = SwapFatalHook(OnFatalError);
	StartFarHeap(0);
	Devices.open(DEVICE_TIMER);
}

/* Stops the music and devices, and blackens or restores the palette. */
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
}

static void InitEnvironment(void)
{
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

/* Picks the song file for the music card. Only the MT-32 plays music here, so any music card
 * becomes one while MIDI output is there, as in the game. */
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
	if ((uint8_t) SoundSetup.isRoland())
		*device = MUSIC_DEVICE_MT32;
	else if ((uint8_t) SoundSetup.isAdlib() || (uint8_t) SoundSetup.isSoundBlaster())
		*device = MUSIC_DEVICE_ADLIB;
	if (*device != 0 && plat_midi_available()) {
		strcpy(MusicFlexName, DataPath(StaticPath, "intrordm.dat"));
		*device = MUSIC_DEVICE_MT32;
	} else
		*device = 0;
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
		if (ReadAudioOptions(preferencesPath, &AudioSettings)) {
			if (AudioSettings.music == AUDIO_OFF)
				Shared::DisableMusic();
			if (AudioSettings.effects == AUDIO_OFF)
				Shared::DisableSfx();
			if (AudioSettings.speech == AUDIO_OFF)
				SpeechEnabled = 0;
		}
	}
	delete[] preferencesPath;
}

/* Opens extended memory, which the menu's shapes and screen come from. */
static uint8_t CheckSystem(void)
{
	uint8_t ok;

	VoodooXmsBlock.base = OpenExtendedMemory();
	VoodooXmsBlock.free = GetExtendedMemorySize();
	VoodooXmsBlock.unusedFlag = 1;
	VoodooXmsBlock.used = 0;
	ok = VoodooXmsBlock.valid();
	if (ok)
		XmsOpened = 1;
	return ok;
}

static void StartUp(void)
{
	InstallFatalHook();
	if (!CheckSystem()) {
		if (XmsOpened)
			ShutdownXMM();
		plat_exit(1);
	}
}

static void TellHowToPlay(void)
{
	RestoreSystem(0, 1);
	plat_log("Type ULTIMA7 to play Ultima VII\n");
	plat_exit(1);
}

static void RunIntroduction(void)
{
	RestoreSystem(0, 1);
	plat_exit(LAUNCH_INTRO);
}

static void RunEndgame(void)
{
	RestoreSystem(0, 1);
	plat_exit(LAUNCH_ENDGAME);
}

/* Takes main's mouse handlers out of the driver. */
static void ReleaseMouse(void)
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

static void QuitToDos(void)
{
	ReleaseMouse();
	RestoreSystem(0, 1);
	plat_exit(LAUNCH_THANKS);
}

static void StartGame(void)
{
	RestoreSystem(0, 1);
	plat_exit(LAUNCH_GAME);
}

/* GAMEARGS.DAT tells the game who the new Avatar is. */
static void WriteGameArgs(char *name, char sex)
{
	RestoreSystem(0, 1);
	char args[GAME_ARGS_SIZE];
	{
		DataFile file("gameargs.dat", FILE_CREATE);

		memset(args, 0, sizeof args);
		*args = sex == 'M' ? 0 : 1;
		strncpy(args + 1, name, GAME_ARGS_SIZE - 2);
		file.write(args, (int32_t) GAME_ARGS_SIZE);
	}
	plat_exit(LAUNCH_GAME);
}

/* A saved game lives in GAMEDAT. */
uint8_t GameExists(void)
{
	return plat_dir_exists(".\\gamedat") != 0;
}

static void BeginNewGame(void)
{
	WriteGameArgs(AvatarName, AvatarSex);
}

static void JourneyOnward(void)
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
static void ParseCommandLine(int16_t argc, char **argv)
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

static int16_t Main(int16_t argc, char **argv)
{
	int16_t irq, port, dma;
	int16_t choice;

	MouseDriver = new Mouse;
	Shared::SetKeyHandler(HandleKey);
	ParseCommandLine(argc, argv);
	ConfigureSound("u7.cfg", DataPath(GamedatPath, "options.cfg"), &irq, &port, &MusicDriver, &dma);
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
		plat_exit(LAUNCH_THANKS);
	}
	if (LossOnly) {
		FillView(&ScreenView, 0);
		ShowTheEnd(THE_END_PAUSE);
		RestoreSystem(0, 1);
		plat_exit(LAUNCH_THANKS);
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
	memset((void *) &MainMenu::OriginalPalette, 0, sizeof MainMenu::OriginalPalette);
	memset((void *) &MainMenu::MenuSpeech, 0, sizeof MainMenu::MenuSpeech);
	memset((void *) &MainMenu::MenuFont, 0, sizeof MainMenu::MenuFont);
	MainMenu::MusicDriver = 0;
	MainMenu::PreviousFatalHook = 0;
	memset((void *) &MainMenu::Devices, 0, sizeof MainMenu::Devices);
	MainMenu::SpeechEnabled = 0;
	memset((void *) &MainMenu::SoundSetup, 0, sizeof MainMenu::SoundSetup);
	memset(&MainMenu::AudioSettings, 0, sizeof MainMenu::AudioSettings);
	MainMenu::StepTicks = 7;
	MainMenu::FadeTicks = 60;
	MainMenu::AvatarSex = 'M';
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
	memset(MainMenu::MusicFlexName, 0, sizeof MainMenu::MusicFlexName);
	memset(MainMenu::AvatarName, 0, sizeof MainMenu::AvatarName);
	memset(MainMenu::path, 0, sizeof MainMenu::path);
	memset(MainMenu::otherPath, 0, sizeof MainMenu::otherPath);
}

extern "C" void ConstructMainMenuMainmenuGlobals(void)
{
	new (&MainMenu::OriginalPalette) Shared::FadingPalette();
	new (&MainMenu::MenuSpeech) FlexSpeechCache(0);
	new (&MainMenu::Music) Shared::MusicSystem();
	new (&MainMenu::MenuFont) Shared::FlexTextPrinter();
	new (&MainMenu::Devices) MainMenu::DeviceList();
	new (&MainMenu::SoundSetup) SoundConfig();
}
