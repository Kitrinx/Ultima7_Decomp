#ifndef DBGFONT_H
#define DBGFONT_H

/* The debug line, drawn straight to the screen in the video BIOS's ROM font. */
extern unsigned char far *RomFontGlyphs;
extern char *DebugAreaValue;
extern char far *DebugAreaLine;
extern unsigned char RomTextColor;
extern unsigned char RomTextBackground;
extern unsigned char far *RomTextScreen;
extern int RomTextPitch;
extern void (*DebugHook)(void);
extern char DebugAreaBuffer[80];

#ifdef __cplusplus
/* Fetches the ROM font at startup. */
struct RomFontLoader {
	int unusedField1;
	RomFontLoader();
};
#endif

void SetDebugArea(long value);
char *GetDebugAreaText(void);
void DrawRomChar(char ch, int x, int y);

#ifdef __cplusplus
extern "C" {
#endif
void DrawRomText(char far *s, int x, int row);
unsigned char far *GetRomFont(int);
#ifdef __cplusplus
}
#endif

void DrawDebugArea(void);
void SetDebugHook(void (*hook)(void));
void CallDebugHook(void);

#endif
