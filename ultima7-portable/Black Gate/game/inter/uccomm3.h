#ifndef UCCOMM3_H
#define UCCOMM3_H

struct Value;

void UC_SetSpeaker(Value *args, Value *);
void UC_ResetConvFace(Value *, Value *);
void UC_CloseSpeaker(Value *args, Value *);
void UC_GetInput(Value *, Value *ret);
void UC_GetOrdInput(Value *, Value *ret);
void UC_InputNumericValue(Value *args, Value *ret);
void UC_SitDown(Value *args, Value *);
void UC_GetWorkType(Value *args, Value *ret);
void UC_SetWorkType(Value *args, Value *);
void UC_PathRunUsecode(Value *args, Value *ret);
void UC_SetPathFailure(Value *args, Value *);

#endif
