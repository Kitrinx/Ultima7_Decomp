#ifndef MAINMENU_CURSOR_H
#define MAINMENU_CURSOR_H

struct View;
struct MouseHandler;

namespace MainMenu {

/* The menu's mouse pointer: one fixed frame of a shape from a Flex entry. */
void LoadPointerShapes(void *self, char *flexName, int16_t entry, int16_t x, int16_t y);
void MoveCursorHook(int16_t events, int16_t buttons, int16_t x, int16_t y);
void EraseCursor(void);
void DrawCursorAt(int16_t x, int16_t y);
void InstallCursorHook(MouseHandler *handler);
void ShowCursor(void);
void HideCursor(void);
void SetCursorTarget(View *buffer);
void DrawCursorInto(View *buffer);
void EraseCursorFrom(View *buffer);

extern View *CursorTarget;
extern int16_t ArrowCenterX;
extern int16_t ArrowCenterY;
extern int16_t CursorFrame;
extern int32_t PointerShapes;
extern int32_t CursorSaveBuffer;
extern Rect CursorRect;
extern int16_t CursorDrawFlags;
extern int16_t CursorSaveMode;

}

#endif
