#ifndef LOWLEVEL_H
#define LOWLEVEL_H

struct View;

/* Assembly helpers: flat (32-bit linear) memory and screen buffers. */
#ifdef __cplusplus
extern "C" {
#endif

/* Flat memory reads and writes. */
int32_t PeekLong(int32_t);
void PokeLong(int32_t, int32_t);
int16_t PeekWord(uint32_t);
void PokeWord(int32_t, int16_t);
uint8_t PeekByte(int32_t);
void PokeByte(int32_t, int8_t);
void FillLinear(int32_t, int8_t, int32_t, int16_t);
int16_t MoveLinear(int32_t, int32_t, uint32_t, int16_t);
int16_t CopyFarToLinear(int32_t, void *, int32_t);
int16_t CopyLinearToFar(void *, int32_t, int32_t);
void MoveLinearFlat(int32_t, int32_t, int32_t);
int32_t SumLinearBytes(int32_t, int32_t);
int16_t TestLinearBit(int32_t, uint16_t);
void SetLinearBit(int32_t, uint16_t);
void ClearLinearBit(int32_t, uint16_t);
void FillLinearRect(int32_t, uint8_t, uint16_t, uint16_t, uint16_t);
void FillScreen(uint8_t);
void CopyScreenWords(int32_t, int32_t);
void RemapScreen(int32_t, int16_t);
void FillScreenBuffer(int32_t, uint8_t);

/* The depth buffer: rows of 128 signed depth bytes. */
void RaiseDepth(int32_t, int16_t);
void RaiseDepthRun(int32_t, int16_t, uint16_t);
int16_t IsBelowDepth(int32_t, int16_t);
int16_t IsBelowDepthCorners(int32_t, int16_t, uint16_t, uint16_t);
int16_t IsDepthBlockSet(int32_t);

/* Palettes, buffers and shapes. */
void SetPaletteRange(int32_t, int16_t *, int16_t, int16_t);
void DrawTile(int32_t, int16_t, int16_t);
void FillView(void *, uint8_t);
void SetRowAddress(uint16_t, int32_t, int32_t);
int32_t GetRowAddress(int16_t, int32_t);
int16_t GetFrameBounds(void *, int16_t, int16_t, int32_t, int16_t, int16_t);
int16_t GetShapeFrameCount(int32_t, int16_t);
void RestoreRect(struct View *, int32_t, void *, int16_t);
void SaveRect(struct View *, int32_t, void *, int16_t);
void DrawFrame(void *, int16_t, int16_t, int32_t, int16_t, int16_t);

void RaiseDepthRect(int32_t, int16_t, int16_t, int16_t);
uint8_t IsShapeHidden(void);
void CopyLinearStringN(uint32_t, uint32_t, int16_t, int16_t);
void CopyLinearString(uint32_t, uint32_t, int16_t);
/* The same copy into ordinary memory. */
void CopyLinearStringOut(char *, uint32_t, int16_t);
void DrawFrameFlippedTranslated(struct View *, int16_t, int16_t, int32_t, int16_t, int32_t, int16_t);
void DrawFrameFlippedTranslucent(struct View *, int16_t, int16_t, int32_t, int16_t, int32_t, int16_t);
void DrawFrameFlipped(struct View *, int16_t, int16_t, int32_t, int16_t, int16_t);
void DrawFrameTranslated(struct View *, int16_t, int16_t, int32_t, int16_t, int32_t, int16_t);
void DrawFrameTranslucent(struct View *, int16_t, int16_t, int32_t, int16_t, int32_t, int16_t);
void CopyScreen(int32_t, int32_t);
void RemapView(struct View, int32_t);
void CopyView(struct View *from, struct View *to);
void SaveUnderFrame(struct View *, int32_t, int16_t, int16_t, int32_t, int16_t, int16_t);
void RestoreUnderFrame(struct View *, int32_t, int16_t, int16_t, int32_t, int16_t, int16_t);
void PaletteBytesToWords(int32_t, int32_t);
int16_t PreparePaletteFade(int32_t, int32_t, int32_t, int32_t, int16_t);
void StepPaletteFade(int32_t, int32_t, int32_t);
void SetPaletteRangeBytes(const uint8_t *, int16_t *, int16_t, int16_t);
void PaletteWordsToBytes(int32_t, int32_t);
void ClearScreen(int16_t);
uint16_t EncodeFrame(struct View *, int32_t, int32_t, int32_t, int32_t, int16_t, int16_t);
void * GetFrameAddress(char *, int16_t, int16_t);
int8_t IsPointInFrame(void *, void *, void *, int16_t);
#ifdef __cplusplus
}
#endif

#endif
