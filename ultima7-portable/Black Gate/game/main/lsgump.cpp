/* Black Gate U7.EXE, overlay segment 344 (file offsets 0x0a4ff0 to 0x0a67b6, 6086 bytes).
 * Borland C++ 2.0 -mm -O -P -Y rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include <new>
#include "plat.h"
#include <string.h>
#include "u7event.h"
#include "u7manage.h"
#include "savegame.h"
#include "colbuf.h"
#include "objref.h"
#include "gumps.h"
#include "lsgump.h"
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
	uint8_t occupied[10];
	int16_t selected;
	SizedSprite load, save, quit, close;
	CounterSprite music, speech, effects;
	int16_t shape, cursorImage;
	Rectangle bounds;
	Timer cursorTimer;
	char savedText[80];
	uint8_t cursorVisible, replaceText;
	uint8_t musicState, speechState, effectsState;
	SaveGump();
	~SaveGump();
	void syncAudio();
	void moveTo(int16_t, int16_t);
	void draw(View *);
	void blink(uint8_t);
	uint8_t saveGame();
	uint8_t loadGame();
	void select(int16_t);
	uint8_t handle(MouseState *);
};

extern View Viewport;
extern "C" View ScreenView;

ProportionalTextPrinter SaveSlotTextPrinter;

void SaveSlot::draw(View *target)
{
	if (visible) {
		Sprite::draw(target);
		View *saved = SaveSlotTextPrinter.target;
		SaveSlotTextPrinter.target = target;
		int16_t textX = x - 197;
		int16_t textY = y - 3;
		SaveSlotTextPrinter.move(textX, textY);
		SaveSlotTextPrinter.printString(text);
		SaveSlotTextPrinter.target = saved;
	}
}

void SaveSlot::edit(uint16_t key, uint8_t replace)
{
	int16_t length = strlen(text);

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

YesNoGump::YesNoGump(int16_t messageFrame) : Sprite(69), yes(70), no(71), message(52)
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

uint8_t YesNoGump::handle(MouseState *state)
{
	uint8_t result = 0;

	if (yes.handle(state) == BUTTON_CLICKED) {
		result = GUMP_YES;
	} else if (no.handle(state) == BUTTON_CLICKED) {
		result = GUMP_NO;
	} else if (PollKeyToGlobalDiscarding()) {
		int16_t key = GetPolledKey();
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

void YesNoGump::moveTo(int16_t nx, int16_t ny)
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

uint8_t DoYesNoDialog(int16_t message)
{
	YesNoGump dialog(message);
	uint8_t action = 0;
	MouseState state;
	uint8_t oldCursor;
	uint8_t answer = 0;

	oldCursor = CursorBase;
	SelectMouseCursor(0);
	PlaySoundSimple(14);
	dialog.paint(&Viewport);
	SetFixedPalette(0);
	CopyFrameBuffer();
	while (action == 0) {
		plat_yield();
		UpdateAndCopyMouseState(&state);
		CyclePalette();
		ContinuePlayingSpeech();
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

SaveGump::SaveGump() : load(5), save(6), quit(56), close(2), music(29), speech(30), effects(31)
{
	int16_t w, h;
	int16_t count;
	int16_t i;

	SelectMouseCursor(7);
	shape = 1362;
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
	int16_t cols = SaveSlotTextPrinter.charWidth(95);
	int16_t rows = SaveSlotTextPrinter.charHeight(95);
	Timer_set(&cursorTimer, INT32_C(16));
	cursorImage = gShapeManager.allocateBlock((int32_t) ((cols + 1) * (rows + 1)), 0x7fff, 0);
	show();
	save.hide();
	load.hide();
	SelectMouseCursor(0);
}

SaveGump::~SaveGump()
{
	for (int16_t i = 0; i < 10; i++) {
		delete slots[i];
	}
	if (!GameRestored) {
		SetAudioState(musicState, speechState, effectsState);
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

void SaveGump::moveTo(int16_t nx, int16_t ny)
{
	bounds.moveTo(nx, ny);
	load.moveTo(nx + 163, ny + 143);
	save.moveTo(nx + 99, ny + 143);
	quit.moveTo(nx + 227, ny + 143);
	close.moveTo(nx + 22, ny + 139);
	for (int16_t i = 0; i < 10; i++) {
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

void SaveGump::blink(uint8_t force)
{
	if (selected >= 0 && selected <= 10) {
		int16_t x, y;
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
			Timer_set(&cursorTimer, INT32_C(32));
			cursorVisible = 1;
		}
		if (Timer_hasFinished(&cursorTimer) && cursorVisible) {
			HideCursor();
			gShapeManager.restoreUnderShape(&ScreenView, cursorImage, x, y, SaveSlotTextPrinter.font, 95);
			ShowCursor();
			Timer_set(&cursorTimer, INT32_C(16));
			cursorVisible = 0;
		}
	}
}

uint8_t SaveGump::saveGame()
{
	uint8_t result = 0;

	if (selected > -1) {
		if (slots[selected]->text[0]) {
			uint8_t confirmed = 1;
			if (occupied[selected]) {
				confirmed = DoYesNoDialog(2);
			}
			paint(&Viewport);
			CopyFrameBuffer();
			result = GUMP_HANDLED;
			if (confirmed) {
				CheckItemBuffer();
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

uint8_t SaveGump::loadGame()
{
	uint8_t result = 0;

	if (!occupied[selected]) {
		strncpy(slots[selected]->text, savedText, 80);
	} else {
		if (selected > -1 && slots[selected]->text[0]) {
			uint8_t confirmed = DoYesNoDialog(1);
			paint(&Viewport);
			CopyFrameBuffer();
			if (confirmed) {
				SelectMouseCursor(7);
				SaveGameFiles.restoreGame(selected);
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
inline void SaveGump::select(int16_t slot)
{
	if (selected != slot && selected > -1) {
		slots[selected]->advance();
		slots[selected]->draw(&Viewport);
	}
	selected = slot;
}

uint8_t SaveGump::handle(MouseState *state)
{
	int16_t mouseX, mouseY;
	uint8_t result = 0;
	uint8_t keyPending, hasMouse;
	uint16_t key;

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
				int16_t nameWidth = SaveSlotTextPrinter.textWidth(slots[selected]->text);
				int16_t width, height;
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
					while (!PollKeyToGlobalDiscarding()) {
						blink(0);
						plat_yield();
					}
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
		for (int16_t i = 0; i < 10; i++) {
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

SaveGump *NewSaveGump()
{
	return new SaveGump;
}

extern "C" void ResetLsgumpGlobals(void)
{
	memset((void *)&SaveSlotTextPrinter, 0, sizeof(SaveSlotTextPrinter));
}

extern "C" void ConstructLsgumpGlobals(void)
{
	new (&SaveSlotTextPrinter) ProportionalTextPrinter();
}
