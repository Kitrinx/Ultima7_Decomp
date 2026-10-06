#ifndef OOPS_H
#define OOPS_H

/* Fatal errors: each reports and quits. */
#ifdef __cplusplus
extern "C" {
#endif

void far ReportOutOfFarMemory(void);
void far ReportOutOfNearMemory(void);
void far ReportOutOfVoodooMemory(void);
void far ReportFileNotFound(char far *name);
void far ReportFileReadError(char far *name);
void far ReportInvalidSaveGame(void);

#ifdef __cplusplus
}
#endif

#endif
