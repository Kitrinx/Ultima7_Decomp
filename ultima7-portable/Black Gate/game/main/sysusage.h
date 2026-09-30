#ifndef SYSUSAGE_H
#define SYSUSAGE_H

void LogMemoryUsage(char *fmt, ...);

extern uint16_t LastNearFree;
extern int32_t LastFarFree;
extern int32_t LastVoodooFree;

#endif
