#ifndef XMMINIT_H
#define XMMINIT_H

/* How extended memory was obtained, in ExtendedMemoryMethod. */
#define XMM_METHOD_XMS      1
#define XMM_METHOD_UNUSED   2
#define XMM_METHOD_INT15    3

#ifdef __cplusplus
extern "C" {
#endif
extern int16_t XMSPresent;
extern int16_t ExtendedMemoryMethod;
extern int16_t XMSAlreadyAllocated;
extern int32_t OverlayMemoryKb;
void *OpenXMSBlock(void);
int32_t OpenExtendedMemory(void);
int32_t GetXMSBlockSize(void);
int32_t GetExtendedMemorySize(void);
int16_t DetectEmsDriver(void);
int16_t IsXMSPresent(void);
int8_t FreeStaleXMSBlock(void);
int16_t LeaveFlatMode(void);
int16_t IsFlatModeBlocked(void);
#ifdef __cplusplus
}
#endif

#endif
