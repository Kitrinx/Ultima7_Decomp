#ifndef UCCOMM7_H
#define UCCOMM7_H

struct Node;

struct Value;

void PrintUsecodeList(Value *value);

void UC_SetLight(Value *args);
void UC_OnBarge(Value *, Value *ret);
void UC_GetMusicTrack(Value *, Value *ret);
void UC_PlayMusic(Value *args);
void UC_SetOrrery(Value *args);
void UC_IsPcInside(Value *, Value *ret);
void UC_SetPaletteFadedIn();
void UC_WearingFellowship(Value *, Value *ret);
void UC_MouseExists(Value *, Value *ret);
void UC_IsWater(Value *args, Value *ret);
void UC_Telekinesis(Value *args);
void PrintUsecodeNode(Node *node);
void UC_DoNothing();
void UC_GetItemFrameRot(Value *args, Value *ret);
void UC_SetItemFrameRot(Value *args);
void UC_GetAlignment(Value *args, Value *ret);
void UC_SetAlignment(Value *args);
void UC_MoveObject(Value *args);
void UC_IsNotBlocked(Value *args, Value *ret);

#endif
