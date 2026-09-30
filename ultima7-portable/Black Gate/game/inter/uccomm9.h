#ifndef UCCOMM9_H
#define UCCOMM9_H

struct Value;

void UC_ItemSay(Value *args, Value *);
void UC_Post(Value *args, Value *ret);
void UC_PostInFuture(Value *args, Value *ret);
void UC_InUsecode(Value *args, Value *ret);
void UC_RollToWin(Value *args, Value *ret);

#endif
