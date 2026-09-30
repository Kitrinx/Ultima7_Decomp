#ifndef INITWP_H
#define INITWP_H

void InitWorldPhysics(void);
uint8_t InitSound(void);
void ShutDownSound(void);

extern char *WihhFileName;
void InitRolandVoices(void);
void UploadRolandPatches(void);
uint8_t AllocMusicBuffers(void);
uint8_t AllocSfxBuffers(void);

extern char *TfaFileName;
extern char *WgtVolFileName;

#endif
