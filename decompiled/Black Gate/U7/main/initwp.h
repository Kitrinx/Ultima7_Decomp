#ifndef INITWP_H
#define INITWP_H

void InitWorldPhysics(void);
unsigned char InitSound(void);
void ShutDownSound(void);

extern char *WihhFileName;
void InitRolandVoices(void);
void UploadRolandPatches(void);
unsigned char AllocMusicBuffers(void);
unsigned char AllocSfxBuffers(void);

extern char *TfaFileName;
extern char *WgtVolFileName;

#endif
