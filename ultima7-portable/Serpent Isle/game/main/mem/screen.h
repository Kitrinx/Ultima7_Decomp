#ifndef SCREEN_H
#define SCREEN_H

struct View;

extern struct View *VgaScreen;
extern struct View *BackScreen;
extern int32_t VgaRowTable;
extern int16_t UnreadScreenWord;
extern struct View *ActiveScreen;
extern int16_t UnusedScreenWord;
extern int16_t *UnreadScreenWordPointer;

/* AllocateDrawBuffer flag: take the pixels from the Voodoo XMS block. */
#define DRAW_IN_XMS 0x10

#ifdef __cplusplus
extern "C" {
#endif
int16_t InitVgaScreen(struct View *view, uint8_t color);
int16_t InitBackScreen(struct View *view, uint8_t color);
uint8_t AllocateDrawBuffer(struct View *view, uint8_t color, uint16_t flags);
void RequireDrawBuffer(struct View *view, uint8_t color, uint16_t flags);
#ifdef __cplusplus
}
#endif

#endif
