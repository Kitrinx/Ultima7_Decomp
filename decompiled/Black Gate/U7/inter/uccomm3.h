#ifndef UCCOMM3_H
#define UCCOMM3_H

struct Value;

void far UC_SetSpeaker(Value *args, Value *);
void far UC_ResetConvFace(Value *, Value *);
void far UC_CloseSpeaker(Value *args, Value *);
void far UC_GetInput(Value *, Value *ret);
void far UC_GetOrdInput(Value *, Value *ret);
void far UC_InputNumericValue(Value *args, Value *ret);
void far UC_SitDown(Value *args, Value *);
void far UC_GetWorkType(Value *args, Value *ret);
void far UC_SetWorkType(Value *args, Value *);
void far UC_PathRunUsecode(Value *args, Value *ret);
void far UC_SetPathFailure(Value *args, Value *);

#endif
