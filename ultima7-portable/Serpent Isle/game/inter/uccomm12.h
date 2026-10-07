#ifndef UCCOMM12_H
#define UCCOMM12_H

#include "ucvalue.h"

void UC_HaltScheduled(Value *args);
void UC_GetTime(Value *args, Value *ret);
void UC_IsNPCNear(Value *args, Value *ret);
void UC_CanVisit(Value *args, Value *ret);
void UC_KeyringContains(Value *args, Value *ret);
void UC_KeyringAdd(Value *args);
void UC_RemoveItemsInArea(Value *args);

#endif
