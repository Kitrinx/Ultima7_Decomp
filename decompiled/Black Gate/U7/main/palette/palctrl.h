#ifndef PALCTRL_H
#define PALCTRL_H

#ifdef __cplusplus
extern "C" {
#endif
void FlashDamagePalette(void);
void StartPaletteFadeIn(void);
void StartPaletteFadeOut(void);
void SetLightLevel(int level);
void FlashLightning(void);
void MarkPaletteFadedOut(void);
void MarkPaletteFadedIn(void);
void SetLightSpellTime(int duration);
void SetTimePalette(void);
void SetFixedPalette(char daylightOnly);
void RestoreGamePalette(void);
#ifdef __cplusplus
}
#endif

#endif
