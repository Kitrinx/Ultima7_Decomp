#ifndef XMMBLOCK_H
#define XMMBLOCK_H

#ifdef __cplusplus
extern "C" {
#endif
int UnlockXMS(void);
int FreeXMS(void);
extern int XMSHandle;
int FindXMSDriver(void);
int UnhookInt15(void);
int DisableA20Global(void);
extern int XMSLargestKilobytes;
extern void far *XMSBlockAddress;
void far *ClaimExtendedMemory(void);
int QueryXMSFree(void);
int AllocateXMS(int kilobytes);
int LockXMS(void);
int EnableA20Global(void);
int SetA20Gate(int enable);
void far SetFlatBit(long linear, int bit);
#ifdef __cplusplus
}
#endif

#endif
