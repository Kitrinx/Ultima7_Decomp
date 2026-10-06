#ifndef EMS_H
#define EMS_H

/* Expanded memory through the page frame. EMS pointers carry the page in their segment and are
 * mapped in by MapEmsPointer before use. */
extern "C" {

extern int EmsVersion;
extern int EmsFrame;

int OpenEms(void);
void CloseEms(void);
void far *AllocateEms(long size);
void FreeEms(void far *block);
void far *MapEmsPointer(void far *block);
long EmsAvailable(void);
long EmsLargestBlock(void);

}

#endif
