#ifndef FREEXMM_H
#define FREEXMM_H

#ifdef __cplusplus
extern "C" {
#endif
void FreeXMM(void);
void ReleaseExtendedMemory(void);
void FreeXMSBlock(void);
int ShutdownXMM(void);
#ifdef __cplusplus
}
#endif

#endif
