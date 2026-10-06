#ifndef MOUSE_H
#define MOUSE_H

/* Called by the mouse driver with the event mask, buttons and position. */
typedef void (far *MouseFunc)(int, int, int, int);

/* The driver's event mask. */
#define MOUSE_MOVED         0x01
#define MOUSE_LEFT          0x06    /* left button pressed or released */
#define MOUSE_RIGHT         0x18
#define MOUSE_PRESSED       0x2a    /* any button pressed */
#define MOUSE_RELEASED      0x54
#define MOUSE_BUTTONS       0xfe    /* every button event */

#define MOUSE_HANDLER_SLOTS 4

/* Resets the driver and centers the pointer. */
struct Mouse {
	int unusedField1;
	Mouse();
};

/* One installed driver callback; slot is -1 when none. */
struct MouseHandler {
	int slot;
	MouseHandler();
	~MouseHandler() { detach(); }
	void attach(MouseFunc, int);
	void detach();
	char installed() { return slot != -1; }
};

void far InitMouse(char);
int far ReadMouseButtons(void);
void far SetMousePosition(int, int);
int far GetMouseX(void);
int far GetMouseY(void);
void far PinMouseHere(void);
void far UnpinMouse(void);

void far SelectMouseCursor(unsigned char);

extern int MouseX;
extern int MouseY;
extern MouseFunc MouseHandlers[MOUSE_HANDLER_SLOTS];
extern int MouseHandlerMasks[MOUSE_HANDLER_SLOTS];
void ResetMouseDriver(void);
void SetMouseXRange(int lo, int hi);
void SetMouseYRange(int lo, int hi);
void SetMouseMickeys(int h, int v);
void PinMouseAt(int x, int y);

#ifdef __cplusplus
extern "C" {
#endif
void far DispatchMouseEvents(void);
#ifdef __cplusplus
}
#endif

extern int MouseStackOverrun;

extern unsigned char MousePresent;
inline unsigned char MouseReady() { return MousePresent; }
extern int MouseButtonCount;
extern char MouseCallbackOff;
inline char MouseCallbacksDisabled() { return MouseCallbackOff; }
extern int SavedMouseMask;
extern void far *SavedMouseCallback;
extern int MouseHandlerCount;

#endif
