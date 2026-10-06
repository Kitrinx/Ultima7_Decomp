#ifndef COLORMAP_H
#define COLORMAP_H

#define BASE_COLORS 16
#define MAX_COLOR_BYTES 16

struct ColorMap {
	unsigned char *map;
};

#ifdef __cplusplus
extern "C" {
#endif
unsigned char far MapColor(struct ColorMap *colorMap, int color);
extern int ColorByteCount;
extern char *ColorBytes[MAX_COLOR_BYTES];
extern char IdentityColorBytes[BASE_COLORS];
extern struct ColorMap ModeColorMaps[5];
#ifdef __cplusplus
}
#endif

void far RegisterColorBytes(char *first, ...);
char *far GetColorByte(int n);

#endif
