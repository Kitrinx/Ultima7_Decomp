#ifndef VIDMODE_H
#define VIDMODE_H

/* Display modes, their colors and the CRT. */
#ifdef __cplusplus
extern "C" {
#endif
void SetDisplayMode(int8_t mode);
void ApplyModeColors(int8_t mode);
void WaitForRetrace(void);
#ifdef __cplusplus
}
#endif

#endif
