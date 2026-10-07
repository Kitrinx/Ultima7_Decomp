#ifndef COLORMAP_H
#define COLORMAP_H

#define BASE_COLORS 16
#define MAX_COLOR_BYTES 16

struct ColorMap {
	uint8_t *map;
};

#ifdef __cplusplus
extern "C" {
#endif
uint8_t MapColor(struct ColorMap *colorMap, int16_t color);
extern int16_t ColorByteCount;
extern char *ColorBytes[MAX_COLOR_BYTES];
extern char IdentityColorBytes[BASE_COLORS];
extern struct ColorMap ModeColorMaps[5];
#ifdef __cplusplus
}
#endif

void RegisterColorBytes(char *first, ...);
char * GetColorByte(int16_t n);

#endif
