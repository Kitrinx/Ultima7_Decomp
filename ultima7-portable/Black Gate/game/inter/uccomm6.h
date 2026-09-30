#ifndef UCCOMM6_H
#define UCCOMM6_H

struct Value;

void UC_TurnOn(Value *args, Value *);
void UC_TurnOff(Value *args, Value *);
void UC_PushKeys(Value *, Value *);
void UC_PopKeys(Value *, Value *);
void UC_ClearKeys(Value *, Value *);
void UC_JoinParty(Value *args, Value *);
void UC_LeaveParty(Value *args, Value *);
void UC_GetNPCName(Value *args, Value *ret);
void UC_RemovePartyItems(Value *args, Value *ret);
void UC_AddPartyItems(Value *args, Value *ret);
void UC_AvatarSex(Value *, Value *ret);
void UC_GetNpcObject(Value *args, Value *ret);

#endif
