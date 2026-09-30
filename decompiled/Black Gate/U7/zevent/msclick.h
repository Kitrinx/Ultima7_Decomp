#ifndef MSCLICK_H
#define MSCLICK_H

struct RomFontLoader;

struct MouseState;

MouseState *GetMouseAction(void);
MouseState *CopyMouseAction(MouseState *e);
unsigned char CheckPointerMoved(void);

extern int MouseHand;
extern MouseState LastMouseAction;
extern int DoubleClickDelay;
extern RomFontLoader RomFont;
void ApplyMouseHand(MouseState *e);
unsigned char ClassifyMouseClick(MouseState *e, int delay);

#endif
