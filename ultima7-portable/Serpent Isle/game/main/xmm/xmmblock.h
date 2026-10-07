#ifndef XMMBLOCK_H
#define XMMBLOCK_H

/* Bits of a dword in linear memory. */
#ifdef __cplusplus
extern "C" {
#endif
void ClearFlatBit(int32_t linear, int16_t bit);
void SetFlatBit(int32_t linear, int16_t bit);
int16_t TestFlatBit(int32_t linear, int16_t bit);
#ifdef __cplusplus
}
#endif

#endif
