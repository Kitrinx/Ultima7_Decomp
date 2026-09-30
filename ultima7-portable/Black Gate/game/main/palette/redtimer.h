#ifndef REDTIMER_H
#define REDTIMER_H

extern int32_t RedScreenStepTicks;
#ifdef __cplusplus
extern "C" {
#endif
void UnhookRedScreenTimer(void);
void StartRedScreenCycle(void);
void StopRedScreenCycle(void);
void HookRedScreenTimer(void);
#ifdef __cplusplus
}
#endif

#endif
