#ifndef UCCOMM11_H
#define UCCOMM11_H

struct Value;

void far UC_DisplayRunes(Value *args, Value *ret);
void far UC_GetWeather(Value *args, Value *ret);
void far UC_SetWeather(Value *args, Value *ret);
void far UC_DisplayMap(Value *args, Value *ret);
void far UC_ErrorMessage(Value *args, Value *ret);
void far UC_BookMode(Value *args, Value *ret);
void far UC_SetAttackMode(Value *args, Value *ret);
void far UC_SetOppressor(Value *args, Value *ret);

#endif
