#ifndef CRGBPAL_H
#define CRGBPAL_H

/* not a PALETTES.FLX record: LoadPalette builds a red ramp instead */
#define RED_RAMP_PALETTE 999

void LoadPalette(long *block, int index);
void AllocatePaletteBlock(long *block);
void BuildRedPalette(long *block);

extern char *PaletteFileName;

#endif
