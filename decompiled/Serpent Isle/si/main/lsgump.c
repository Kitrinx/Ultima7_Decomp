/* Serpent Isle SI.EXE, overlay segment 333 (file offsets 0x09b1f0 to 0x09c9da, 6122 bytes).
 * Borland C++ 2.0 -mm -O -P -Y rebuilds it byte for byte as C++.
 */

/* path: lsgump.c */
#include <string.h>
#include <mem.h>
#include "u7event.h"
#include "u7manage.h"
#include "savegame.h"
#include "colbuf.h"
#include "objref.h"
#include "gumps.h"
#include "lsgump.h"
#include "npcshape.h"
#include "itable.h"
#include "mevent.h"
#include "palfade.h"
#include "voice.h"
#include "mouse.h"
#include "itemcmd.h"
#include "palctrl.h"
#include "preload.h"
#include "oops.h"
#include "bltshape.h"
#include "text.h"
#include "sprite.h"
#include "tools.h"
#include "u7point.h"
#include "camera.h"
#include "gumpmgr.h"
#include "systimer.h"

/* the save, load and options dialog */
struct SaveGump : Control {
	SaveSlot *slots[10];
	unsigned char occupied[10];
	int selected;
	SizedSprite load, save, quit, close;
	CounterSprite music, speech, effects;
	int shape, cursorImage;
	Rectangle bounds;
	Timer cursorTimer;
	char savedText[80];
	unsigned char cursorVisible, replaceText;
	unsigned char musicState, speechState, effectsState;
	SaveGump();
	~SaveGump();
	void syncAudio();
	void moveTo(int, int);
	void draw(View *);
	void blink(unsigned char);
	unsigned char saveGame();
	unsigned char loadGame();
	void select(int);
	unsigned char handle(MouseState *);
};

extern View Viewport;
extern View ScreenView;

ProportionalTextPrinter SaveSlotTextPrinter;

void SaveSlot::draw(View *target)
{
	if (visible) {
		Sprite::draw(target);
		View *saved = SaveSlotTextPrinter.target;
		SaveSlotTextPrinter.target = target;
		int textX = x - 197;
		int textY = y - 3;
		SaveSlotTextPrinter.move(textX, textY);
		SaveSlotTextPrinter.printString(text);
		SaveSlotTextPrinter.target = saved;
	}
}

void SaveSlot::edit(unsigned key, unsigned char replace)
{
	int length = strlen(text);

	switch (key) {
	case 8:     /* backspace */
		if (length) {
			text[length - 1] = 0;
		}
		break;
	case 0x14d: /* right arrow */
		break;
	default:
		if (replace) {
			strncpy(text, "", 80);
			length = 0;
		}
		if (key >= 32 && key <= 126) {
			text[length] = key;
			length++;
			text[length] = 0;
		}
	}
}

YesNoGump::YesNoGump(int messageFrame) : Sprite(51), yes(52), no(53), message(47)
{
	add(&yes);
	add(&no);
	add(&message);
	yes.show();
	no.show();
	message.setFrame(messageFrame);
	message.show();
	show();
	moveTo(95, 68);
}

unsigned char YesNoGump::handle(MouseState *state)
{
	unsigned char result = 0;

	if (yes.handle(state) == BUTTON_CLICKED) {
		result = GUMP_YES;
	} else if (no.handle(state) == BUTTON_CLICKED) {
		result = GUMP_NO;
	} else if (PollKeyToGlobalDiscarding()) {
		int key = GetPolledKey();
		switch (key) {
		case 'Y':
		case 'y':
			result = GUMP_YES;
			break;
		case 27:    /* Esc */
		case 'N':
		case 'n':
			result = GUMP_NO;
			break;
		}
		SkipToMouseRelease();
	}
	return result;
}

void YesNoGump::moveTo(int nx, int ny)
{
	x = nx;
	y = ny;
	yes.moveTo(x + 63, y + 45);
	no.moveTo(x + 84, y + 45);
	message.moveTo(x + 125, y + 33);
}

void YesNoGump::draw(View *target)
{
	Sprite::draw(target);
}

unsigned char far DoYesNoDialog(int message)
{
	YesNoGump dialog(message);
	unsigned char action = 0;
	MouseState state;
	unsigned char oldCursor;
	unsigned char answer = 0;

	oldCursor = CursorBase;
	SelectMouseCursor(0);
	PlaySoundSimple(94);
	dialog.paint(&Viewport);
	SetFixedPalette(0);
	CopyFrameBuffer();
	while (action == 0) {
		UpdateAndCopyMouseState(&state);
		CyclePalette();
		action = dialog.handle(&state);
		if (action) {
			switch (action) {
			case GUMP_YES:
				answer = 1;
				break;
			case GUMP_NO:
				answer = 0;
				break;
			}
		}
	}
	SelectMouseCursor(oldCursor);
	return answer;
}

SaveGump::SaveGump() : load(5), save(6), quit(50), close(2), music(24), speech(25), effects(26)
{
	int w, h;
	int count;
	int i;

	SelectMouseCursor(7);
	shape = 1426;
	bounds.x = 0;
	bounds.y = 0;
	gShapeManager.getShapeSize(&w, &h, shape);
	bounds.x1 = w;
	bounds.y1 = h;
	add(&load);
	add(&save);
	add(&close);
	close.show();
	add(&quit);
	quit.show();
	count = SaveGameFiles.getFreeSaveSpace();
	for (i = 0; i < 10; i++) {
		slots[i] = new SaveSlot(4);
		if (slots[i] == 0) {
			ReportOutOfNearMemory();
		}
		add(slots[i]);
		slots[i]->show();
		SaveGameFiles.getSaveTitle(i, slots[i]->text);
		if (slots[i]->text[0]) {
			occupied[i] = 1;
		} else {
			count--;
			if (count < 0) {
				slots[i]->hide();
				slots[i]->lock();
			}
			occupied[i] = 0;
		}
	}
	selected = -1;
	add(&music);
	add(&speech);
	add(&effects);
	syncAudio();
	moveTo(30, 19);
	memset(savedText, 0, 80);
	cursorVisible = 0;
	SaveSlotTextPrinter.setFont(2);
	int cols = SaveSlotTextPrinter.charWidth(95);
	int rows = SaveSlotTextPrinter.charHeight(95);
	Timer_set(&cursorTimer, 16L);
	cursorImage = gShapeManager.allocateBlock((long) ((cols + 1) * (rows + 1)), 0x7fff, 0);
	show();
	save.hide();
	load.hide();
	SelectMouseCursor(0);
}

SaveGump::~SaveGump()
{
	for (int i = 0; i < 10; i++) {
		delete slots[i];
	}
	if (!GameRestored) {
		SetAudioState(musicState, speechState, effectsState);
	} else {
		GetAudioOptions(&musicState, &speechState, &effectsState);
	}
	gShapeManager.releaseBlock(cursorImage);
}

void SaveGump::syncAudio()
{
	GetAudioOptions(&musicState, &speechState, &effectsState);
	if (musicState == 0) {
		music.hide();
		music.lock();
	} else {
		music.show();
		music.setFrame(musicState == 1 ? 0 : 1);
	}
	if (speechState == 0) {
		speech.hide();
		speech.lock();
	} else {
		speech.show();
		speech.setFrame(speechState == 1 ? 0 : 1);
	}
	if (effectsState == 0) {
		effects.hide();
		effects.lock();
	} else {
		effects.show();
		effects.setFrame(effectsState == 1 ? 0 : 1);
	}
}

void SaveGump::moveTo(int nx, int ny)
{
	bounds.moveTo(nx, ny);
	load.moveTo(nx + 163, ny + 143);
	save.moveTo(nx + 99, ny + 143);
	quit.moveTo(nx + 227, ny + 143);
	close.moveTo(nx + 22, ny + 139);
	for (int i = 0; i < 10; i++) {
		slots[i]->moveTo(nx + 237, ny + i * 13 + 14);
	}
	music.moveTo(nx + 99, ny + 156);
	speech.moveTo(nx + 163, ny + 156);
	effects.moveTo(nx + 227, ny + 156);
}

void SaveGump::draw(View *target)
{
	ShapeManager_draw(&gShapeManager, target, bounds.x, bounds.y, shape, 0, 0, 0);
}

void SaveGump::blink(unsigned char force)
{
	if (selected >= 0 && selected <= 10) {
		int x, y;
		char *text = slots[selected]->text;
		x = slots[selected]->x + SaveSlotTextPrinter.textWidth(text) - 197;
		y = slots[selected]->y - 3;
		if ((Timer_hasFinished(&cursorTimer) && !cursorVisible) || force) {
			HideCursor();
			gShapeManager.saveUnderShape(&ScreenView, cursorImage, x, y, SaveSlotTextPrinter.font, 95);
			View *saved = SaveSlotTextPrinter.target;
			SaveSlotTextPrinter.target = &ScreenView;
			SaveSlotTextPrinter.move(x, y);
			SaveSlotTextPrinter.printString("_");
			SaveSlotTextPrinter.target = saved;
			ShowCursor();
			Timer_set(&cursorTimer, 32L);
			cursorVisible = 1;
		}
		if (Timer_hasFinished(&cursorTimer) && cursorVisible) {
			HideCursor();
			gShapeManager.restoreUnderShape(&ScreenView, cursorImage, x, y, SaveSlotTextPrinter.font, 95);
			ShowCursor();
			Timer_set(&cursorTimer, 16L);
			cursorVisible = 0;
		}
	}
}

unsigned char SaveGump::saveGame()
{
	unsigned char result = 0;

	if (selected > -1) {
		if (slots[selected]->text[0]) {
			unsigned char confirmed = 1;
			if (occupied[selected]) {
				confirmed = DoYesNoDialog(2);
			}
			paint(&Viewport);
			CopyFrameBuffer();
			result = GUMP_HANDLED;
			if (confirmed) {
				SelectMouseCursor(7);
				char name[80];
				strncpy(name, slots[selected]->text, 80);
				SetAudioState(musicState, speechState, effectsState);
				SaveGameFiles.saveGame(selected, name, 0);
				BarkAfterSaving();
				SelectMouseCursor(0);
				occupied[selected] = 1;
			} else {
				strncpy(slots[selected]->text, savedText, 80);
			}
			slots[selected]->advance();
			selected = -1;
			cursorVisible = 0;
			load.hide();
			save.hide();
			paint(&Viewport);
			CopyFrameBuffer();
		} else {
			ReportNoCanDo(1);
			result = GUMP_HANDLED;
		}
	}
	return result;
}

unsigned char SaveGump::loadGame()
{
	unsigned char result = 0;

	if (!occupied[selected]) {
		strncpy(slots[selected]->text, savedText, 80);
	} else {
		if (selected > -1 && slots[selected]->text[0]) {
			unsigned char confirmed = DoYesNoDialog(1);
			paint(&Viewport);
			CopyFrameBuffer();
			if (confirmed) {
				SelectMouseCursor(7);
				SaveGameFiles.restoreGame(selected);
				RestorePolymorphs();
				CheckMoonshadeRavaged();
				CenterOnAvatar();
				SetTimePalette();
				GameRestored = 1;
				SelectMouseCursor(0);
				result = GUMP_RESTORED;
			} else {
				result = GUMP_HANDLED;
			}
		} else {
			ReportNoCanDo(1);
			result = GUMP_HANDLED;
		}
	}
	return result;
}

/* Deselect the highlighted slot, if another one is taking its place. */
inline void SaveGump::select(int slot)
{
	if (selected != slot && selected > -1) {
		slots[selected]->advance();
		slots[selected]->draw(&Viewport);
	}
	selected = slot;
}

unsigned char SaveGump::handle(MouseState *state)
{
	int mouseX, mouseY;
	unsigned char result = 0;
	unsigned char keyPending, hasMouse;
	unsigned key;

	mouseX = MouseState_getX(state);
	mouseY = state->y;
	hasMouse = MousePresent;
	if (hasMouse && selected != -1)
		blink(0);
	keyPending = PollKeyToGlobalDiscarding();
	key = GetPolledKey();
	/* without a mouse, the number and keypad keys move the pointer instead of typing */
	if (!hasMouse) {
		switch (key) {
		case '1': case '2': case '3': case '4':
		case '6': case '7': case '8': case '9':
		case 0x147: case 0x148: case 0x149: case 0x14b:
		case 0x14d: case 0x14f: case 0x150: case 0x151:
		case 0x173: case 0x174: case 0x175: case 0x176:
		case 0x177: case 0x184: case 0x18d: case 0x191:
			keyPending = 0;
			break;
		}
	}
	if (keyPending && selected != -1) {
		if (!hasMouse) {
			blink(1);
			KeyMouseEnabled = 0;
			paint(&Viewport);
			CopyFrameBuffer();
			HideCursor();
			CursorFrozen = 1;
			key = 0;
		}
		do {
			if (key == 27) {
				if (!hasMouse) {
					strncpy(slots[selected]->text, savedText, 80);
					slots[selected]->advance();
					selected = -1;
					cursorVisible = 0;
					load.hide();
					save.hide();
				}
				result = GUMP_CLOSE;
			} else if (key == 13 && !load.isVisible() && save.isVisible()) {
				result = saveGame();
			} else {
				if (load.isVisible() && strcmp(savedText, slots[selected]->text)) {
					load.hide();
				}
				int nameWidth = SaveSlotTextPrinter.textWidth(slots[selected]->text);
				int width, height;
				slots[selected]->size(&width, &height);
				if (key == 8 || width + (nameWidth + SaveSlotTextPrinter.charWidth(key) +
					SaveSlotTextPrinter.charWidth(95)) - 197 <= width) {
					slots[selected]->edit(key, replaceText);
					if (replaceText)
						replaceText = 0;
					if (slots[selected]->text[0])
						save.show();
					else
						save.hide();
				}
				paint(&Viewport);
				CopyFrameBuffer();
				if (!hasMouse) {
					HideCursor();
					while (!PollKeyToGlobalDiscarding())
						blink(0);
					key = GetPolledKey();
				}
			}
		} while (!hasMouse && key != 13 && key != 27);
		if (!hasMouse) {
			KeyMouseEnabled = 1;
			CursorFrozen = 0;
			ShowCursor();
		}
		if (result == 0)
			result = GUMP_HANDLED;
	} else if (keyPending && key == 27) {
		if (selected != -1) {
			strncpy(slots[selected]->text, savedText, 80);
			selected = -1;
			cursorVisible = 0;
			load.hide();
			save.hide();
		} else {
			result = GUMP_CLOSE;
		}
	} else if (!bounds.contains(mouseX, mouseY)) {
		result = 0;
	} else if ((result = close.handle(state)) != 0) {
		switch (result) {
		case BUTTON_CLICKED: return GUMP_CLOSE;
		default: return result;
		}
	} else if (music.isVisible() && (result = music.handle(state))) {
		musicState = result == BUTTON_OFF ? 1 : 2;
		result = GUMP_HANDLED;
	} else if (speech.isVisible() && (result = speech.handle(state))) {
		speechState = result == BUTTON_OFF ? 1 : 2;
		result = GUMP_HANDLED;
	} else if (effects.isVisible() && (result = effects.handle(state))) {
		effectsState = result == BUTTON_OFF ? 1 : 2;
		result = GUMP_HANDLED;
	} else if (save.isVisible() && save.handle(state) == BUTTON_CLICKED) {
		result = saveGame();
	} else if (load.isVisible() && load.handle(state) == BUTTON_CLICKED) {
		result = loadGame();
	} else if (quit.isVisible() && quit.handle(state) == BUTTON_CLICKED) {
		if (DoYesNoDialog(0))
			result = GUMP_QUIT;
		else
			result = GUMP_HANDLED;
	} else {
		for (int i = 0; i < 10; i++) {
			if (slots[i]->isVisible() && slots[i]->handle(state)) {
				if (selected == i) {
					strncpy(slots[selected]->text, savedText, 80);
					selected = -1;
					cursorVisible = 0;
					load.hide();
					save.hide();
				} else {
					if (selected != -1)
						strncpy(slots[selected]->text, savedText, 80);
					select(i);
					strncpy(savedText, slots[selected]->text, 80);
				}
				if (selected != -1) {
					Timer_restart(&cursorTimer);
					strncpy(savedText, slots[selected]->text, 80);
					if (savedText)
						replaceText = 1;
					else
						replaceText = 0;
					if (occupied[selected]) {
						save.show();
						load.show();
					} else {
						load.hide();
						save.hide();
					}
				}
				result = GUMP_HANDLED;
				break;
			}
		}
	}
	if (result == GUMP_HANDLED) {
		paint(&Viewport);
		CopyFrameBuffer();
	}
	return result;
}
