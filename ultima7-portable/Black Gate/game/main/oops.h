#ifndef OOPS_H
#define OOPS_H

/* Fatal errors: each reports and quits. */
#ifdef __cplusplus
extern "C" {
#endif

void ReportOutOfFarMemory(void);
void ReportOutOfNearMemory(void);
void ReportOutOfVoodooMemory(void);
void ReportFileNotFound(char *name);
void ReportFileReadError(char *name);
void ReportInvalidSaveGame(void);

#ifdef __cplusplus
}
#endif

#endif
