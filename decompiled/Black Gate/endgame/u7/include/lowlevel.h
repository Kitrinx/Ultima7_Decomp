#ifndef LOWLEVEL_H
#define LOWLEVEL_H

struct View;

/* Assembly helpers: flat (32-bit linear) memory and screen buffers. */
#ifdef __cplusplus
extern "C" {
#endif

/* Flat memory reads and writes. */
long far PeekLong(long);
void far PokeLong(long, long);
int far PeekWord(unsigned long);
void far PokeWord(long, int);
unsigned char far PeekByte(long);
void far PokeByte(long, char);
void far FillLinear(long, char, long, int);
int far MoveLinear(long, long, unsigned long, int);
int far CopyFarToLinear(long, void far *, long);
int far CopyLinearToFar(void far *, long, long);
void far MoveLinearFlat(long, long, long);

/* Palettes, buffers and shapes. */
void far SetPaletteRange(long, int *, int, int);
void far DrawTile(long, int, int);
void far FillView(void *, unsigned char);
void far SetRowAddress(unsigned, long, long);
long far GetRowAddress(int, long);
int far GetFrameBounds(void far *, int, int, long, int, int);
int far GetShapeFrameCount(long, int);
void far RestoreRect(int, long, void *, int);
void far SaveRect(int, long, void *, int);
void far DrawFrame(void *, int, int, long, int, int);

void far RaiseDepthRect(long, int, int, int);
unsigned char far IsShapeHidden(void);
void far CopyLinearStringN(unsigned long, unsigned long, int, int);
void far CopyLinearString(unsigned long, unsigned long, int);
void far DrawFrameFlippedTranslated(struct View *, int, int, long, int, long, int);
void far DrawFrameFlippedTranslucent(struct View *, int, int, long, int, long, int);
void far DrawFrameFlipped(struct View *, int, int, long, int, int);
void far DrawFrameTranslated(struct View *, int, int, long, int, long, int);
void far DrawFrameTranslucent(struct View *, int, int, long, int, long, int);
void far CopyScreen(long, long);
void far RemapView(struct View, long);
void CopyView(struct View *from, struct View *to);
void far SaveUnderFrame(struct View *, long, int, int, long, int, int);
void far RestoreUnderFrame(struct View *, long, int, int, long, int, int);
void far PaletteBytesToWords(long, long);
int far PreparePaletteFade(long, long, long, long, int);
void far StepPaletteFade(long, long, long);
void far SetPaletteRangeBytes(long, int *, int, int);
void far PaletteWordsToBytes(long, long);
void far ClearScreen(int);
unsigned EncodeFrame(struct View *, long, long, long, long, int, int);
void far * far GetFrameAddress(char far *, int, int);
char far IsPointInFrame(void far *, void far *, void far *, int);
#ifdef __cplusplus
}
#endif

#endif
