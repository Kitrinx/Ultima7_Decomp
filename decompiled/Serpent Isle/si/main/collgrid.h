#ifndef COLLGRID_H
#define COLLGRID_H

/* The collision grid: a dword of bits per cell, 128 cells a row. */
#ifdef __cplusplus
extern "C" {
#endif
unsigned long far GetCollisionCell(int row, int col);
long far OrCollisionBlock(int row, int col, int rows, int cols);
char far IsCollisionBlockSet(int row, int col, int rows, int cols, long bits);
void far OrCollisionCell(int row, int col, long bits);
void far AndCollisionCell(int row, int col, long bits);
void far SetCollisionCell(int row, int col, long bits);
#ifdef __cplusplus
}
#endif

#endif
