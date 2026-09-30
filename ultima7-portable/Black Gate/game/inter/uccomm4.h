#ifndef UCCOMM4_H
#define UCCOMM4_H

struct Value;

void UC_AddSpell(Value *args, Value *ret);
void UC_CauseLight(Value *args);
void UC_StopTime(Value *args);
void UC_GetBarge(Value *args, Value *ret);
void UC_DisplayArea(Value *args);
void UC_WizardEye(Value *args);
void UC_Earthquake(Value *args);
void UC_Lightning();
void UC_GetHour(Value *args, Value *ret);
void UC_GameMinute(Value *args, Value *ret);
void UC_GetClock(Value *args, Value *ret);
void UC_SetClock(Value *args);
void UC_MarkVirtueStone(Value *args);
void UC_RecallVirtueStone(Value *args);
void UC_FindNearest(Value *args, Value *ret);
void UC_RestoreMouseCursor();
void UC_SelectNoArrowCursor();
void UC_RestartGame();
void UC_RunEndgame(Value *args);
void UC_SetCamera(Value *args);
void UC_GetDeadParty(Value *args, Value *ret);
void UC_ViewTile(Value *args);

#endif
