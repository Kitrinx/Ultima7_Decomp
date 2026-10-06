#ifndef LINEAR_H
#define LINEAR_H

/* Reads and writes of flat (32-bit linear) memory, in linear.asm. */
#ifdef __cplusplus
extern "C" {
#endif
long far PeekLong(long);
void far PokeLong(long, long);
unsigned far PeekWord(unsigned long);
void far PokeWord(long, int);
unsigned char far PeekByte(long);
void far PokeByte(long, char);
void far FillLinear(long, char, long, int);
int far MoveLinear(long, long, unsigned long, int);
int far CopyFarToLinear(long, void far *, long);
int far CopyLinearToFar(void far *, long, long);
#ifdef __cplusplus
}
#endif

#endif
