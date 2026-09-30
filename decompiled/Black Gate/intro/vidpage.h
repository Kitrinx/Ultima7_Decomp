#ifndef VIDPAGE_H
#define VIDPAGE_H

/* Screen modes and the pages of spare video memory a mode leaves free. */
#ifdef __cplusplus
extern "C" {
#endif
void pascal SetScreenMode(int mode);
int pascal AllocatePageRun(int size);
void pascal FreePageRun(int first);
void DumpPageRuns(void);
#ifdef __cplusplus
}
#endif

#endif
