#ifndef CHEAT_H
#define CHEAT_H

struct NPCRef;
struct objref;

extern int8_t CheatsEnabled;
extern uint8_t FrameRateShown;
extern char PromptWordBuffer[50];
extern int16_t DoScheduleNpc;
extern uint8_t ShowNpcNumbers;
extern uint8_t ShowAvatarLocation;
extern uint8_t PowerAvatar;
extern uint8_t QueueToggle;
extern uint8_t UnkBugChecking;
extern uint8_t StatusLineRaised;
extern uint8_t FollowersEnabled;

/* Defined with the game menu. */
extern uint8_t HackMoverEnabled;

void ConsolePrintAt(int16_t x, int16_t y, char *fmt, ...);
void ConsolePrintAndWait(int16_t x, int16_t y, char *fmt, ...);
void ConsoleBlankLine(int16_t x, int16_t y);
void ConsoleShowMessage(char *msg);
char *PromptForWord(char *prompt);
int32_t PromptForLong(char *prompt, int8_t hex);
int16_t PromptForIntegerWord(char *prompt);
int8_t PromptForKey(char *prompt);
void ClearLeftPanel(void);
void ClearRightPanel(void);
void PrintInRightPanel(int16_t x, int16_t y, char *fmt, ...);
void PrintInLeftPanel(int16_t x, int16_t y, char *fmt, ...);
void EditNpcTargets(NPCRef *npc);
void EditNpcAttackMode(NPCRef *npc);
void ShowTeleportMenu(void);
void EditNpcFlags(objref *npc);
void EditNpcStats(objref *npc);
void ModifyNpc(NPCRef *npc);
void DumpNpcActivity(objref *npc);
void ShowDebugMenu(void);
void ShowReadableTexts(void);
void CheatPlaySound(void);
void CheatPlayMusic(void);
void EmptyPickedContainer(void);
int8_t PromptForUsecodeEvent(char *event, int16_t *index);

#endif
