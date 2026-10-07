#ifndef COLLGRID_H
#define COLLGRID_H

/* The collision grid: a dword of bits per cell, 128 cells a row. */
#ifdef __cplusplus
extern "C" {
#endif
uint32_t GetCollisionCell(int16_t row, int16_t col);
int32_t OrCollisionBlock(int16_t row, int16_t col, int16_t rows, int16_t cols);
int8_t IsCollisionBlockSet(int16_t row, int16_t col, int16_t rows, int16_t cols, int32_t bits);
void OrCollisionCell(int16_t row, int16_t col, int32_t bits);
void AndCollisionCell(int16_t row, int16_t col, int32_t bits);
void SetCollisionCell(int16_t row, int16_t col, int32_t bits);
#ifdef __cplusplus
}
#endif

#endif
