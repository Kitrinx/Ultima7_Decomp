#ifndef REDTIMER_H
#define REDTIMER_H

extern long RedScreenStepTicks;
#ifdef __cplusplus
extern "C" {
#endif
void far UnhookRedScreenTimer(void);
void far StartRedScreenCycle(void);
void far StopRedScreenCycle(void);
void far HookRedScreenTimer(void);
#ifdef __cplusplus
}
#endif

#endif
