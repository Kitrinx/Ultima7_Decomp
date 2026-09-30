#ifndef DBGFONT_H
#define DBGFONT_H

/* The debug line, drawn straight to the screen in the video BIOS's ROM font. */
extern uint8_t *RomFontGlyphs;
extern char *DebugAreaValue;
extern char *DebugAreaLine;
extern uint8_t RomTextColor;
extern uint8_t RomTextBackground;
extern uint8_t *RomTextScreen;
extern int16_t RomTextPitch;
extern void (*DebugHook)(void);
extern char DebugAreaBuffer[80];

#ifdef __cplusplus
/* Fetches the ROM font at startup. */
struct RomFontLoader {
	int16_t unusedField1;
	RomFontLoader();
};
#endif

void SetDebugArea(int32_t value);
char *GetDebugAreaText(void);
void DrawRomChar(int8_t ch, int16_t x, int16_t y);

#ifdef __cplusplus
extern "C" {
#endif
void DrawRomText(char *s, int16_t x, int16_t row);
#ifdef __cplusplus
}
#endif

void DrawDebugArea(void);
void SetDebugHook(void (*hook)(void));
void CallDebugHook(void);

#endif
