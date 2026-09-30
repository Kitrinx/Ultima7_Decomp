#ifndef UCCOMM1_H
#define UCCOMM1_H

struct Value;

void UC_GetNpcProp(Value *args, Value *ret);
void UC_SetNpcProp(Value *args, Value *ret);
void UC_CountObjects(Value *args, Value *ret);
void UC_FindObject(Value *args, Value *ret);
void UC_ReduceHealthPlain(Value *args);
void UC_ReduceHealth(Value *args);
void UC_IsDead(Value *args, Value *ret);
void UC_ApplyDamage(Value *args, Value *ret);
void UC_IsReadied(Value *args, Value *ret);
void UC_ResetPalette();
void UC_SetTimePalette();
void UC_AOrAn(Value *args, Value *ret);

#endif
