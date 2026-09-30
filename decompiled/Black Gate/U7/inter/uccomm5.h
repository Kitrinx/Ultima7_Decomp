#ifndef UCCOMM5_H
#define UCCOMM5_H

extern int RemoteViewActive;

struct Value;

void far UC_AttackObject(Value *args, Value *ret);
void far UC_FlashMouse(Value *args, Value *);
void far UC_FireProjectile(Value *args, Value *);
void far UC_AdvanceTime(Value *args, Value *);
void far UC_NapTime(Value *args, Value *);
void far UC_StartSpeech(Value *args, Value *ret);
void far UC_CallGuards(Value *, Value *);
void far UC_AttackAvatar(Value *, Value *);
void far UC_FadePalette(Value *args, Value *);
void far UC_InCombat(Value *, Value *ret);
void far UC_StartBlockingSpeech(Value *args, Value *ret);
void far UC_Summon(Value *args, Value *);

extern char UsecodeSpeechFileName[];

#endif
