#ifndef UCCOMM13_H
#define UCCOMM13_H

struct Value;

void far UC_SetItemShape(Value *args, Value *);
void far UC_GetDist(Value *args, Value *ret);
void far UC_FindDirection(Value *args, Value *ret);
void far UC_DirectionFrom(Value *args, Value *ret);

#endif
