#ifndef UCCOMM8_H
#define UCCOMM8_H

struct Value;

void UC_GetItemShape(Value *args, Value *ret);
void UC_GetItemFrame(Value *args, Value *ret);
void UC_SetItemFrame(Value *args);
void UC_GetQuality(Value *args, Value *ret);
void UC_SetQuality(Value *args, Value *ret);
void UC_GetItemQuantity(Value *args, Value *ret);
void UC_SetQuantity(Value *args, Value *ret);
void UC_GetCoord(Value *args, Value *ret);
void UC_IsNpc(Value *args, Value *ret);
void UC_GetPartyList(Value *, Value *ret);
void UC_GetPartyList2(Value *, Value *ret);
void UC_GetContainer(Value *args, Value *ret);
void UC_RemoveItem(Value *args);
void UC_ItemSay2(Value *args);
void UC_CloseGumps();
void UC_InGumpMode(Value *, Value *ret);
void UC_Random(Value *args, Value *ret);
void UC_DieRoll(Value *args, Value *ret);
void UC_GetArraySize(Value *args, Value *ret);
void UC_GetNpcNumber(Value *args, Value *ret);
void UC_FindNearby(Value *args, Value *ret);
void UC_GetTarget(Value *, Value *ret);
void UC_GetAvatarRef(Value *, Value *ret);
void UC_Sfx(Value *args);
void UC_PlaySoundEffect2(Value *args);

#endif
