#ifndef FREEXMM_H
#define FREEXMM_H

#ifdef __cplusplus
extern "C" {
#endif
int ShutdownXMM(void);
void FreeXMM(void);
void ReleaseExtendedMemory(void);
void FreeXMSBlock(void);
#ifdef __cplusplus
}
#endif

#endif
