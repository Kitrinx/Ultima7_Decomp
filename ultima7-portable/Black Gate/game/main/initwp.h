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

#endif
