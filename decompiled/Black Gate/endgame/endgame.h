#ifndef ENDGAME_H
#define ENDGAME_H

/* The launcher's selector for the credits, run once the ending is over. */
#define EXIT_CREDITS    7

extern int FadeSpeed;
extern int SceneDelay;

void Quit(char *message);
void ReadError();
unsigned char WaitOrKey(unsigned long ticks);

#endif
