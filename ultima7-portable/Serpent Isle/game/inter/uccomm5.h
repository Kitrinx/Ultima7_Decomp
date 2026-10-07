#ifndef UCCOMM5_H
#define UCCOMM5_H

struct Coord;

extern int16_t RemoteViewActive;
extern Coord MarkedX, MarkedY, MarkedZ;     /* the place SaveCoord keeps for RecallCoord */

struct Value;

void UC_AttackObject(Value *args, Value *ret);
void UC_FlashMouse(Value *args, Value *);
void UC_FireProjectile(Value *args, Value *);
void UC_AdvanceTime(Value *args, Value *);
void UC_NapTime(Value *args, Value *);
void UC_StartSpeech(Value *args, Value *ret);
void UC_CallGuards(Value *, Value *);
void UC_AttackAvatar(Value *, Value *);
void UC_StopArrest(Value *, Value *);
void UC_FadePalette(Value *args, Value *);
void UC_FadeForSleep(Value *args, Value *);
void UC_InCombat(Value *, Value *ret);
void UC_Summon(Value *args, Value *);
void UC_Polymorph(Value *args, Value *);
void UC_RevertSchedule(Value *args, Value *);
void UC_Schedule(Value *args, Value *);
void UC_ChangeSchedule(Value *args, Value *);
void UC_NewSchedule(Value *args, Value *);

extern const char UsecodeSpeechFileName[];

#endif
