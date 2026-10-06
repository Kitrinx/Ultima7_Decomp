#ifndef U7POINT_H
#define U7POINT_H

extern int CursorX, CursorY;
extern unsigned char CursorDrawn, CursorTracking;

inline unsigned char IsCursorDrawn() { return CursorDrawn; }
inline void DisableCursorTracking() { CursorTracking = 0; }
inline void EnableCursorTracking() { CursorTracking = 1; }
inline void SetCursorDrawn() { CursorDrawn = 1; }
inline void ClearCursorDrawn() { CursorDrawn = 0; }

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
