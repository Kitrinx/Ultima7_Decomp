#ifndef UCCOMM6_H
#define UCCOMM6_H

struct Value;

void far UC_TurnOn(Value *args, Value *);
void far UC_TurnOff(Value *args, Value *);
void far UC_PushKeys(Value *, Value *);
void far UC_PopKeys(Value *, Value *);
void far UC_ClearKeys(Value *, Value *);
void far UC_JoinParty(Value *args, Value *);
void far UC_LeaveParty(Value *args, Value *);
void far UC_GetNPCName(Value *args, Value *ret);
void far UC_RemovePartyItems(Value *args, Value *ret);
void far UC_AddPartyItems(Value *args, Value *ret);
void far UC_AvatarSex(Value *, Value *ret);
void far UC_GetNpcObject(Value *args, Value *ret);

#endif
