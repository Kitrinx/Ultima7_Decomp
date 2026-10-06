#ifndef VIDMODE_H
#define VIDMODE_H

/* Display modes, their colors and the CRT. */
#ifdef __cplusplus
extern "C" {
#endif
void pascal SetDisplayMode(char mode);
void ApplyModeColors(char mode);
void far FindCrtStatusPort(void);
void far WaitForRetrace(void);
void far GetVideoMode(unsigned char *mode);
void far SetVideoMode(unsigned char *mode);
#ifdef __cplusplus
}
#endif

#endif
