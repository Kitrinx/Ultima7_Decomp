#ifndef U7POINT_H
#define U7POINT_H

struct Rect;

struct MouseHandler;

void LoadPointerShapes(void *self, int16_t x, int16_t y);
void InstallCursorHook(MouseHandler *handler);
void MoveCursorHook(int16_t events, int16_t buttons, int16_t x, int16_t y);
void ShowCursor(void);

extern int16_t CursorFrame;
extern int16_t PointerFrameCount;
extern uint8_t CursorBase;
extern uint16_t CursorCenterZ;
extern int32_t PointerShapes;
extern int32_t CursorSaveBuffer;
extern Rect CursorRect;
extern int16_t CursorDrawFlags;
extern int16_t CursorSaveMode;
extern uint8_t CardinalArrowsOnly;
extern int16_t ArrowLength;
void HideCursor(void);
void AdjustCursorZ(int16_t amount);

struct View;

void DrawCursorInto(View *buffer);
void SetCursorTarget(View *buffer);
void EraseCursorFrom(View *buffer);

extern uint8_t CursorFrozen;
extern int16_t CursorSaveSize;
extern uint8_t unused_global_3;
int16_t AbsoluteInt(int16_t value);
int8_t CheckCursorGuard(void);
int16_t GetArrowLength(int16_t x, int16_t y);
int16_t GetCursorFrame(int16_t x, int16_t y);
void EraseCursor(void);
void DrawCursorAt(int16_t x, int16_t y);
void SelectMouseCursor(uint8_t cursor);
void RedrawCursor(void);
void ShowCursorAt(int16_t x, int16_t y);

extern View *CursorTarget;

void AllocateHostCursor(int16_t width, int16_t height);
void SetHostCursorFrame(int32_t shapes, int16_t frame, int16_t flags);
extern int16_t ArrowCenterX;
extern int16_t ArrowCenterY;

#endif
