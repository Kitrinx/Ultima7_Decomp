#ifndef UCCOMM1_H
#define UCCOMM1_H

struct Value;

void far UC_GetNpcProp(Value *args, Value *ret);
void far UC_SetNpcProp(Value *args, Value *ret);
void far UC_CountObjects(Value *args, Value *ret);
void far UC_FindObject(Value *args, Value *ret);
void far UC_ReduceHealthPlain(Value *args);
void far UC_ReduceHealth(Value *args);
void far UC_IsDead(Value *args, Value *ret);
void far UC_ApplyDamage(Value *args, Value *ret);
void far UC_IsReadied(Value *args, Value *ret);
void far UC_ResetPalette();
void far UC_SetTimePalette();
void far UC_AOrAn(Value *args, Value *ret);

#endif
