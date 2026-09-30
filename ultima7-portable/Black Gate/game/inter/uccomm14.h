#ifndef UCCOMM14_H
#define UCCOMM14_H

struct Value;

void UC_CreateNewObject(Value *args, Value *ret);
void UC_SetLastCreated(Value *args, Value *ret);
void UC_PopToMap(Value *args, Value *ret);
void UC_PopToNPC(Value *args, Value *ret);
void UC_CloseGump(Value *args, Value *);
void UC_GetContItems(Value *args, Value *ret);
void UC_FindNearbyAvatar(Value *args, Value *ret);

#endif
