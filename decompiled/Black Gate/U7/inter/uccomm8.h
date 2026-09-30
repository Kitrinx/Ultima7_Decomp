#ifndef UCCOMM8_H
#define UCCOMM8_H

struct Value;

void far UC_GetItemShape(Value *args, Value *ret);
void far UC_GetItemFrame(Value *args, Value *ret);
void far UC_SetItemFrame(Value *args);
void far UC_GetQuality(Value *args, Value *ret);
void far UC_SetQuality(Value *args, Value *ret);
void far UC_GetItemQuantity(Value *args, Value *ret);
void far UC_SetQuantity(Value *args, Value *ret);
void far UC_GetCoord(Value *args, Value *ret);
void far UC_IsNpc(Value *args, Value *ret);
void far UC_GetPartyList(Value *, Value *ret);
void far UC_GetPartyList2(Value *, Value *ret);
void far UC_GetContainer(Value *args, Value *ret);
void far UC_RemoveItem(Value *args);
void far UC_ItemSay2(Value *args);
void far UC_CloseGumps();
void far UC_InGumpMode(Value *, Value *ret);
void far UC_Random(Value *args, Value *ret);
void far UC_DieRoll(Value *args, Value *ret);
void far UC_GetArraySize(Value *args, Value *ret);
void far UC_GetNpcNumber(Value *args, Value *ret);
void far UC_FindNearby(Value *args, Value *ret);
void far UC_GetTarget(Value *, Value *ret);
void far UC_GetAvatarRef(Value *, Value *ret);
void far UC_Sfx(Value *args);
void far UC_PlaySoundEffect2(Value *args);

#endif
