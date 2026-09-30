#ifndef MSCLICK_H
#define MSCLICK_H

struct RomFontLoader;

struct MouseState;

MouseState *GetMouseAction(void);
MouseState *CopyMouseAction(MouseState *e);
uint8_t CheckPointerMoved(void);

extern int16_t MouseHand;
extern MouseState LastMouseAction;
extern int16_t DoubleClickDelay;
extern RomFontLoader RomFont;
void ApplyMouseHand(MouseState *e);
uint8_t ClassifyMouseClick(MouseState *e, int16_t delay);

#endif
