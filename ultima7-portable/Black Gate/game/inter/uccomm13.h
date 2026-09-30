#ifndef UCCOMM13_H
#define UCCOMM13_H

struct Value;

void UC_SetItemShape(Value *args, Value *);
void UC_GetDist(Value *args, Value *ret);
void UC_FindDirection(Value *args, Value *ret);
void UC_DirectionFrom(Value *args, Value *ret);

#endif
