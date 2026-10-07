#ifndef VIDMODE_H
#define VIDMODE_H

/* Display modes, their colors and the CRT. */
extern int8_t DisplayMode;
extern int8_t UnusedModeByte;
extern const char BiosVideoModes[6];
extern int16_t SlotRunLengths[256];

#ifdef __cplusplus
extern "C" {
#endif
void SetDisplayMode(int8_t mode);
void SetBiosVideoMode(int8_t mode);
int16_t AllocateSlotRun(int16_t size);
void FreeSlotRun(int16_t first);
void ApplyModeColors(int8_t mode);
void FindCrtStatusPort(void);
void WaitForRetrace(void);
void GetVideoMode(uint8_t *mode);
void SetVideoMode(uint8_t *mode);
#ifdef __cplusplus
}
#endif

#endif
