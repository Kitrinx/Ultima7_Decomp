#ifndef CRGBPAL_H
#define CRGBPAL_H

/* not a PALETTES.FLX record: LoadPalette builds a red ramp instead */
#define RED_RAMP_PALETTE 999

void LoadPalette(int32_t *block, int16_t index);
void AllocatePaletteBlock(int32_t *block);
void BuildRedPalette(int32_t *block);

extern char *PaletteFileName;

#endif
