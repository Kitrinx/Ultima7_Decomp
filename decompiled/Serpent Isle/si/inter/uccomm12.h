#ifndef UCCOMM12_H
#define UCCOMM12_H

#include "ucvalue.h"

void far UC_HaltScheduled(Value *args);
void far UC_GetTime(Value *args, Value *ret);
void far UC_IsNPCNear(Value *args, Value *ret);
void far UC_CanVisit(Value *args, Value *ret);
void far UC_KeyringContains(Value *args, Value *ret);
void far UC_KeyringAdd(Value *args);
void far UC_RemoveItemsInArea(Value *args);

#endif
