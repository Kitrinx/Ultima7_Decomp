#ifndef SYSCLK_H
#define SYSCLK_H

#ifdef __cplusplus
extern "C" {
#endif
void interrupt BiosClockHandler(...);
void interrupt TimerTickHandler(...);
extern long far BiosClockVector;
extern long far TickChainVector;
#ifdef __cplusplus
}
#endif

#endif
