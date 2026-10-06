#ifndef U7MAP_H
#define U7MAP_H

struct MapCoordsSaver;

struct Coord;

void ScreenToWorldCoords(int x, int y, int *tx, int *ty, char snap);
void WorldCoordsToScreen(Coord tx, Coord ty, int *x, int *y);

extern MapCoordsSaver MapCoordsFile;
extern char *MapCoordFileName;

#endif
