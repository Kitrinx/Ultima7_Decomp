#ifndef MAIN_H
#define MAIN_H

extern unsigned char RestartRequested;
extern unsigned char EndgameRequested;
extern unsigned char EndgameQuitRequested;
extern int TimeAdvanceRate;
#ifdef __cplusplus
extern "C" {
#endif
void far MainGameLoop(void);
#ifdef __cplusplus
}
#endif

#endif
