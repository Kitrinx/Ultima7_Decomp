#ifndef PROGRAMS_H
#define PROGRAMS_H

#include <stdint.h>

/* The helper programs' entry points, each once its own EXE. */
#ifdef __cplusplus
extern "C" {
#endif
int16_t MainMenuMain(int16_t argc, char **argv);
int16_t IntroMain(int16_t argc, char **argv);
int16_t EndgameMain(int16_t argc, char **argv);
#ifdef __cplusplus
}
#endif

#endif
