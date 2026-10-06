#ifndef UCCOMM5_H
#define UCCOMM5_H

struct Coord;

extern int RemoteViewActive;
extern Coord MarkedX, MarkedY, MarkedZ;     /* the place SaveCoord keeps for RecallCoord */

struct Value;

void far UC_AttackObject(Value *args, Value *ret);
void far UC_FlashMouse(Value *args, Value *);
void far UC_FireProjectile(Value *args, Value *);
void far UC_AdvanceTime(Value *args, Value *);
void far UC_NapTime(Value *args, Value *);
void far UC_StartSpeech(Value *args, Value *ret);
void far UC_CallGuards(Value *, Value *);
void far UC_AttackAvatar(Value *, Value *);
void far UC_StopArrest(Value *, Value *);
void far UC_FadePalette(Value *args, Value *);
void far UC_FadeForSleep(Value *args, Value *);
void far UC_InCombat(Value *, Value *ret);
void far UC_Summon(Value *args, Value *);
void far UC_Polymorph(Value *args, Value *);
void far UC_RevertSchedule(Value *args, Value *);
void far UC_Schedule(Value *args, Value *);
void far UC_ChangeSchedule(Value *args, Value *);
void far UC_NewSchedule(Value *args, Value *);

extern char UsecodeSpeechFileName[];

#endif
