#ifndef UCCOMM14_H
#define UCCOMM14_H

struct Value;

void far UC_CreateNewObject(Value *args, Value *ret);
void far UC_SetLastCreated(Value *args, Value *ret);
void far UC_PopToMap(Value *args, Value *ret);
void far UC_PopToNPC(Value *args, Value *ret);
void far UC_CloseGump(Value *args, Value *);
void far UC_GetContItems(Value *args, Value *ret);
void far UC_FindNearbyAvatar(Value *args, Value *ret);

#endif
