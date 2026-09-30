#ifndef UCCOMM2_H
#define UCCOMM2_H

extern int16_t SpeechTrack;

struct Value;

void UC_SpriteEffect(Value *args);
void UC_ObjSpriteEffect(Value *args);
void UC_SetToAttack(Value *args, Value *ret);
void UC_RemoveNpc(Value *args);
void UC_KillNpc(Value *args);
void UC_Clone(Value *args);
void UC_Resurrect(Value *args, Value *ret);
void UC_GetLift(Value *args, Value *ret);
void UC_SetLift(Value *args);
void UC_RetiredEmptyCall(void);
void UC_GetSpeechTrack(Value *args, Value *ret);
void UC_Armageddon(void);

#endif
