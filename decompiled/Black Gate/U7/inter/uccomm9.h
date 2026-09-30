#ifndef UCCOMM9_H
#define UCCOMM9_H

struct Value;

void far UC_ItemSay(Value *args, Value *);
void far UC_Post(Value *args, Value *ret);
void far UC_PostInFuture(Value *args, Value *ret);
void far UC_InUsecode(Value *args, Value *ret);
void far UC_RollToWin(Value *args, Value *ret);

#endif
