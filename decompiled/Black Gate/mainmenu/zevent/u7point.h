#ifndef U7POINT_H
#define U7POINT_H

struct Rect;
struct View;
struct MouseHandler;

void LoadPointerShapes(void *self, char *flexName, int entry, int x, int y);
void MoveCursorHook(int events, int buttons, int x, int y);
void EraseCursor(void);
void DrawCursorAt(int x, int y);
void InstallCursorHook(MouseHandler *handler);
void ShowCursor(void);
void HideCursor(void);
void SetCursorTarget(View *buffer);
void DrawCursorInto(View *buffer);
void EraseCursorFrom(View *buffer);

extern int CursorTarget;
extern int ArrowCenterX;
extern int ArrowCenterY;
extern int CursorFrame;
extern long PointerShapes;
extern long CursorSaveBuffer;
extern Rect CursorRect;
extern int CursorDrawFlags;
extern int CursorSaveMode;

#endif
