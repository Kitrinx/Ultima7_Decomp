#ifndef UCCOMM11_H
#define UCCOMM11_H

struct Value;

void UC_DisplayRunes(Value *args, Value *ret);
void UC_GetWeather(Value *args, Value *ret);
void UC_SetWeather(Value *args, Value *ret);
void UC_DisplayMap(Value *args, Value *ret);
void UC_ErrorMessage(Value *args, Value *ret);
void UC_BookMode(Value *args, Value *ret);
void UC_SetAttackMode(Value *args, Value *ret);
void UC_SetOppressor(Value *args, Value *ret);

#endif
