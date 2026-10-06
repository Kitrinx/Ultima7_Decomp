#ifndef BIOSTEXT_H
#define BIOSTEXT_H

/* Text through the BIOS and DOS, usable in any video mode. */

/* A text cursor position. */
struct CursorPos {
	unsigned char row;
	unsigned char column;
};

#define DISPLAY_CGA     0               /* what DetectDisplay finds */
#define DISPLAY_EGA     1
#define DISPLAY_VGA     2

void PutBiosString(char *s);
unsigned GetBiosKey();
void GetCursor(CursorPos *pos, unsigned char page);
void SetCursor(CursorPos *pos, unsigned char page);
unsigned char DetectDisplay();
void SetScanLines(unsigned char lines);
void PutDosString(char *s);

#endif
