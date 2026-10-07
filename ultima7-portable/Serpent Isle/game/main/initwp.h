#ifndef INITWP_H
#define INITWP_H

void InitWorldPhysics(void);
uint8_t InitSound(void);
void ShutDownSound(void);

extern char *const WihhFileName;
void InitRolandVoices(void);
void UploadRolandPatches(void);
uint8_t AllocMusicBuffers(void);
uint8_t AllocSfxBuffers(void);

extern char *const TfaFileName;
extern char *const WgtVolFileName;
extern uint16_t *SoundDriverImage;
extern void *TimbreBank;
extern uint16_t TimbreCacheSize;
extern int32_t MusicStateTableSize;

#endif
