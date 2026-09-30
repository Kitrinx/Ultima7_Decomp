#ifndef COLORREG_H
#define COLORREG_H

/* Color operands: 0-15 stand for themselves; from 16 on, bytes registered by RegisterColorBytes. */
#define BASE_COLORS 16
#define MAX_COLOR_BYTES 16

/* Maps color operands to palette indexes; with no table, each maps to itself. */
struct ColorMap {
	uint8_t *map;
};

extern int16_t ColorByteCount;
extern char *ColorBytes[MAX_COLOR_BYTES];
extern char IdentityColorBytes[BASE_COLORS];
extern struct ColorMap ModeColorMaps[5];       /* one per display mode */

void RegisterColorBytes(char *first, ...);
char * GetColorByte(int16_t n);

#ifdef __cplusplus
extern "C"
#endif
uint8_t MapColor(struct ColorMap *colorMap, int16_t color);

#endif
