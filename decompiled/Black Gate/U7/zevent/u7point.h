#ifndef U7POINT_H
#define U7POINT_H

struct Rect;

struct MouseHandler;

void LoadPointerShapes(void *self, int x, int y);
void InstallCursorHook(MouseHandler *handler);
void MoveCursorHook(int events, int buttons, int x, int y);
void ShowCursor(void);

extern int CursorFrame;
extern int PointerFrameCount;
extern unsigned char CursorBase;
extern unsigned CursorCenterZ;
extern long PointerShapes;
extern long CursorSaveBuffer;
extern Rect CursorRect;
extern int CursorDrawFlags;
extern int CursorSaveMode;
extern unsigned char CardinalArrowsOnly;
extern int ArrowLength;
void HideCursor(void);
void AdjustCursorZ(int amount);

struct View;

void DrawCursorInto(View *buffer);
void SetCursorTarget(View *buffer);
void EraseCursorFrom(View *buffer);

extern unsigned char CursorFrozen;
extern int CursorSaveSize;
extern unsigned char unused_global_3;
int AbsoluteInt(int value);
char CheckCursorGuard(void);
int GetArrowLength(int x, int y);
int GetCursorFrame(int x, int y);
void EraseCursor(void);
void DrawCursorAt(int x, int y);
void SelectMouseCursor(unsigned char cursor);
void RedrawCursor(void);
void ShowCursorAt(int x, int y);

extern int CursorTarget;
extern int ArrowCenterX;
extern int ArrowCenterY;

#endif
