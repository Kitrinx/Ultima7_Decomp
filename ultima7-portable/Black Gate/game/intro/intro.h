#ifndef INTRO_INTRO_H
#define INTRO_INTRO_H

namespace Intro {

/* The folders the game's files are read from, each with its trailing backslash. */
extern char *const StaticPath;
extern char *const GamedatPath;

/* How many screen paints the machine manages in a measured time; motion is scaled by it. */
extern const int16_t SpeedDivisor;

char *DataPath(char *dir, char *name);

/* Fills a view with frames of random pixels in one color, as a television's static. */
void DrawStatic(View *view, int16_t frames, int16_t color);

}

#endif
