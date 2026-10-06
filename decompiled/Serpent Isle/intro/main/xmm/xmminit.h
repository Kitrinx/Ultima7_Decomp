#ifndef XMMINIT_H
#define XMMINIT_H

/* How extended memory was obtained, in ExtendedMemoryMethod. */
#define XMM_METHOD_XMS      1
#define XMM_METHOD_UNUSED   2
#define XMM_METHOD_INT15    3

#ifdef __cplusplus
extern "C" {
#endif
extern int XMSPresent;
extern int ExtendedMemoryMethod;
extern int XMSAlreadyAllocated;
int DetectEmsDriver(void);
int IsXMSPresent(void);
char FreeStaleXMSBlock(void);
int LeaveFlatMode(void);
int IsFlatModeBlocked(void);
#ifdef __cplusplus
}
#endif

#endif
