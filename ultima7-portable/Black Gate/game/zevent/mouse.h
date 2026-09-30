#ifndef MOUSE_H
#define MOUSE_H

/* Called by the mouse driver with the event mask, buttons and position. */
typedef void ( *MouseFunc)(int16_t, int16_t, int16_t, int16_t);

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
	int16_t unusedField1;
	Mouse();
};

/* One installed driver callback; slot is -1 when none. */
struct MouseHandler {
	int16_t slot;
	MouseHandler();
	~MouseHandler() { detach(); }
	void attach(MouseFunc, int16_t);
	void detach();
	int8_t installed() { return slot != -1; }
};

void InitMouse(int8_t);
int16_t ReadMouseButtons(void);
void SetMousePosition(int16_t, int16_t);
int16_t GetMouseX(void);
int16_t GetMouseY(void);
void PinMouseHere(void);
void UnpinMouse(void);

void SelectMouseCursor(uint8_t);

extern int16_t MouseX;
extern int16_t MouseY;
extern MouseFunc MouseHandlers[MOUSE_HANDLER_SLOTS];
extern int16_t MouseHandlerMasks[MOUSE_HANDLER_SLOTS];
void ResetMouseDriver(void);
void SetMouseXRange(int16_t lo, int16_t hi);
void SetMouseYRange(int16_t lo, int16_t hi);
void SetMouseMickeys(int16_t h, int16_t v);
void PinMouseAt(int16_t x, int16_t y);

extern int16_t MouseStackOverrun;

extern uint8_t MousePresent;
extern int16_t MouseButtonCount;
extern int8_t MouseCallbackOff;
extern int16_t MouseHandlerCount;

#endif
