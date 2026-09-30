#ifndef SYSUSAGE_H
#define SYSUSAGE_H

void LogMemoryUsage(char *fmt, ...);

extern unsigned LastNearFree;
extern long LastFarFree;
extern long LastVoodooFree;

#endif
