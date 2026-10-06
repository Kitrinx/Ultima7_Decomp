#ifndef VIDMODE_H
#define VIDMODE_H

/* Display modes, their colors and the CRT. */
extern char DisplayMode;
extern char UnusedModeByte;
extern char BiosVideoModes[6];
extern int SlotRunLengths[256];

#ifdef __cplusplus
extern "C" {
#endif
void pascal SetDisplayMode(char mode);
void pascal SetBiosVideoMode(char mode);
int pascal AllocateSlotRun(int size);
void pascal FreeSlotRun(int first);
void ApplyModeColors(char mode);
void far FindCrtStatusPort(void);
void far WaitForRetrace(void);
void far GetVideoMode(unsigned char *mode);
void far SetVideoMode(unsigned char *mode);
#ifdef __cplusplus
}
#endif

#endif
