#ifndef MAIN_H
#define MAIN_H

extern uint8_t RestartRequested;
extern uint8_t EndgameRequested;
extern uint8_t EndgameQuitRequested;
extern int16_t TimeAdvanceRate;
#ifdef __cplusplus
extern "C" {
#endif
void MainGameLoop(void);
#ifdef __cplusplus
}
#endif
/* Holds world draws to the pace of the main loop's frame limiter. */
void WaitForFrameTime(void);

#endif
