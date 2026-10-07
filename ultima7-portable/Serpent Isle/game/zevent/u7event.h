#ifndef U7EVENT_H
#define U7EVENT_H

#include "mouse.h"

/* Keyboard and mouse events. */

/* MouseState actions; 0 is none. */
#define MOUSE_CLICK         1
#define MOUSE_DOUBLE_CLICK  2
#define MOUSE_RELEASE       3
#define MOUSE_MOVE          4

/* Keys PollKey returns for the extended keys. */
#define KEY_HOME            0x147
#define KEY_UP              0x148
#define KEY_PAGE_UP         0x149
#define KEY_LEFT            0x14b
#define KEY_RIGHT           0x14d
#define KEY_END             0x14f
#define KEY_DOWN            0x150
#define KEY_PAGE_DOWN       0x151
#define KEY_CTRL_LEFT       0x173
#define KEY_CTRL_RIGHT      0x174
#define KEY_CTRL_END        0x175
#define KEY_CTRL_PAGE_DOWN  0x176
#define KEY_CTRL_HOME       0x177
#define KEY_CTRL_PAGE_UP    0x184
#define KEY_CTRL_UP         0x18d
#define KEY_CTRL_DOWN       0x191

/* Keys made from mouse events: add the button. */
#define KEY_MOUSE_CLICK     0x200
#define KEY_MOUSE_DOUBLE    0x202
#define KEY_MOUSE_HELD      0x204
#define KEY_MOUSE_RELEASE   0x206

/* One mouse event: what the driver reported, and what the queue made of it. */
struct MouseState {
	uint8_t type;         /* a MouseEvent type */
	uint8_t button;       /* 1 left, 2 right */
	int16_t x;
	int16_t y;
	uint8_t state;        /* buttons held */
	uint8_t action;
	uint32_t time;
	MouseState() { type = 0; action = 0; }
	MouseState(int8_t action, int8_t button, int16_t x, int16_t y);
	uint8_t valid() { return action != 0; }
	uint8_t moving() { return action == MOUSE_MOVE; }
	uint8_t pressed(uint8_t b)
	{
		return (action == MOUSE_CLICK || action == MOUSE_DOUBLE_CLICK) && button == b;
	}
	uint8_t clicked() { return (action == MOUSE_CLICK || action == MOUSE_DOUBLE_CLICK) && button == 1; }
	uint8_t released() { return action == MOUSE_RELEASE && button == 1; }
};

extern uint8_t CursorDrawn, CursorTracking;
extern int16_t CursorX, CursorY, DoubleClickDelay;

/* Starts with the cursor hidden and redrawn as the mouse moves. */
struct MouseDevice : MouseHandler {
	MouseDevice() {
		CursorDrawn = 0;
		CursorX = 0;
		CursorY = 0;
		CursorTracking = 1;
	}
};
/* The ring of fixed-size events the mouse driver fills. */
struct EventQueue : MouseHandler {
	EventQueue(char *, int16_t);
	EventQueue(char *, int16_t, int16_t);
	void install();
};
struct MouseQueue : EventQueue {
	MouseQueue(char *, int16_t);
	MouseQueue(char *, int16_t, int16_t);
	void install();
};
struct MouseInterface : Mouse {
	MouseInterface() {
	}
};
struct MouseControl : MouseInterface {
	MouseControl() {
	}
};
struct Input : MouseDevice {
	MouseQueue queue;
	MouseControl interface;
	Input();
	void initialize();
	void enterGumpMode();
	void leaveGumpMode();
	uint8_t isButtonPressed(uint8_t button);
	uint8_t isButtonReleased(uint8_t button);
	uint8_t isGumpMode();
	void enableKeyboardMouse();
};

extern Input GameInput;

void PollKeyAndTranslateWithMouse(uint16_t *, int16_t *, int16_t *);
int16_t KeyPressed(void);
int16_t ReadKey(void);
void UnreadKey(int16_t key);
void FlushKeyboard(void);
uint16_t PollKey(int16_t discard);
uint8_t PollKeyToGlobalDiscarding(void);
uint8_t PollEscapeKey(void);
void GetCtrlStatus(void);
int16_t GetLeftAndRightShiftStatus(void);
void SetKeyMouseEvent(uint8_t event);
MouseState *PollKeyboardMouse(void);
MouseState * GetLastMouseState(void);
MouseState * UpdateAndGetMouseState(void);
MouseState * UpdateAndCopyMouseState(MouseState *);
uint8_t PollPointerMoved(void);
int16_t MouseState_getX(MouseState *);
MouseState *GetDriverMouseState(void);
int16_t GetPolledKey(void);
void SetMouseCursorPosition(int16_t x, int16_t y);
uint8_t InGumpMode(void);
uint8_t MouseExists(void);

void WaitForClickOrKey(void);
void SelectNoArrowCursor(void);
void SelectArrowCursor(void);
extern int16_t PolledKey;
extern uint8_t HeldMouseButton;

extern uint8_t GumpMode, KeyMouseEnabled;
extern int16_t KeyMouseX;
extern int16_t KeyMouseY;
extern uint8_t PreviousCtrlStatus;
extern uint8_t CtrlStatus;
extern uint8_t KeyMouseReleasePending;

void RunKeyboardMouse(void);

#endif
