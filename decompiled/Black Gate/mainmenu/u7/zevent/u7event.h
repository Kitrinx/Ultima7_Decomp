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
	unsigned char type;         /* a MouseEvent type */
	unsigned char button;       /* 1 left, 2 right */
	int x;
	int y;
	unsigned char state;        /* buttons held */
	unsigned char action;
	unsigned long time;
	MouseState() { type = 0; action = 0; }
	MouseState(char action, char button, int x, int y);
	unsigned char valid() { return action != 0; }
	unsigned char moving() { return action == MOUSE_MOVE; }
	unsigned char pressed(unsigned char b)
	{
		return (action == MOUSE_CLICK || action == MOUSE_DOUBLE_CLICK) && button == b;
	}
	unsigned char clicked() { return (action == MOUSE_CLICK || action == MOUSE_DOUBLE_CLICK) && button == 1; }
	unsigned char released() { return action == MOUSE_RELEASE && button == 1; }
};

extern unsigned char CursorDrawn, CursorTracking;
extern int CursorX, CursorY, DoubleClickDelay;

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
	EventQueue(char *, int);
	EventQueue(char *, int, int);
	void install();
};
struct MouseQueue : EventQueue {
	MouseQueue(char *, int);
	MouseQueue(char *, int, int);
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
	unsigned char isButtonPressed(unsigned char button);
	unsigned char isButtonReleased(unsigned char button);
	unsigned char isGumpMode();
	void enableKeyboardMouse();
};

extern Input GameInput;

void far PollKeyAndTranslateWithMouse(unsigned *, int *, int *);
void far FlushKeyboard(void);
unsigned PollKey(int discard);
unsigned char far PollKeyToGlobalDiscarding(void);
unsigned char PollEscapeKey(void);
void GetCtrlStatus(void);
int GetLeftAndRightShiftStatus(void);
void SetKeyMouseEvent(unsigned char event);
MouseState *PollKeyboardMouse(void);
MouseState *far GetLastMouseState(void);
MouseState *far UpdateAndGetMouseState(void);
MouseState *far UpdateAndCopyMouseState(MouseState *);
unsigned char PollPointerMoved(void);
int far MouseState_getX(MouseState *);
MouseState *GetDriverMouseState(void);
int far GetPolledKey(void);
void SetMouseCursorPosition(int x, int y);
unsigned char far InGumpMode(void);
unsigned char far MouseExists(void);

void WaitForClickOrKey(void);
void SelectNoArrowCursor(void);
void SelectArrowCursor(void);
extern int PolledKey;
extern unsigned char HeldMouseButton;

extern unsigned char GumpMode, KeyMouseEnabled;
extern int KeyMouseX;
extern int KeyMouseY;
extern unsigned char PreviousCtrlStatus;
extern unsigned char CtrlStatus;
extern unsigned char KeyMouseReleasePending;

#ifdef __cplusplus
extern "C" {
#endif
void far InterceptCtrlC(void);
extern void far *far PrevKeyIntercept;
#ifdef __cplusplus
}
#endif

void RunKeyboardMouse(void);

#endif
