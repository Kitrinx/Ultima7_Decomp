#ifndef SCREEN_H
#define SCREEN_H

struct View;

/* AllocateDrawBuffer flag: take the pixels from the Voodoo XMS block. */
#define DRAW_IN_XMS 0x10

#ifdef __cplusplus
extern "C" {
#endif
int far pascal InitVgaScreen(struct View *view, unsigned char color);
unsigned char far pascal AllocateDrawBuffer(struct View *view, unsigned char color, unsigned flags);
void far RequireDrawBuffer(struct View *view, unsigned char color, unsigned flags);
#ifdef __cplusplus
}
#endif

#endif
