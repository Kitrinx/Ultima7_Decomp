#ifndef UCCOMM7_H
#define UCCOMM7_H

struct Node;

struct Value;

void far PrintUsecodeList(Value *value);

extern int InfravisionEnabled;
void far UC_SetLight(Value *args);
void far UC_SetInfravision(Value *args);
void far UC_OnBarge(Value *, Value *ret);
void far UC_GetMusicTrack(Value *, Value *ret);
void far UC_PlayMusic(Value *args);
void far UC_SetOrrery(Value *args);
void far UC_IsPcInside(Value *, Value *ret);
void far UC_SetPaletteFadedIn();
void far UC_WearingFellowship(Value *, Value *ret);
void far UC_MouseExists(Value *, Value *ret);
void far UC_IsWater(Value *args, Value *ret);
void far UC_Telekinesis(Value *args);
void far PrintUsecodeNode(Node *node);
void far UC_DoNothing();
void far UC_GetItemFrameRot(Value *args, Value *ret);
void far UC_SetItemFrameRot(Value *args);
void far UC_GetAlignment(Value *args, Value *ret);
void far UC_SetAlignment(Value *args);
void far UC_MoveObject(Value *args);
void far UC_IsNotBlocked(Value *args, Value *ret);

#endif
