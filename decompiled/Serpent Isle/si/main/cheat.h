#ifndef CHEAT_H
#define CHEAT_H

struct NPCRef;
struct objref;

extern char CheatsEnabled;
extern unsigned char FrameRateShown;
extern char PromptWordBuffer[50];
extern int DoScheduleNpc;
extern unsigned char ShowNpcNumbers;
extern unsigned char ShowAvatarLocation;
extern unsigned char PowerAvatar;
extern unsigned char QueueToggle;
extern unsigned char UnkBugChecking;
extern unsigned char StatusLineRaised;
extern unsigned char FollowersEnabled;

/* Defined with the game menu. */
extern unsigned char HackMoverEnabled;

void ConsolePrintAt(int x, int y, char *fmt, ...);
void ConsolePrintAndWait(int x, int y, char *fmt, ...);
void ConsoleBlankLine(int x, int y);
void ConsoleShowMessage(char *msg);
char *PromptForWord(char *prompt);
long PromptForLong(char *prompt, char hex);
int PromptForIntegerWord(char *prompt);
char PromptForKey(char *prompt);
void ClearLeftPanel(void);
void ClearRightPanel(void);
void PrintInRightPanel(int x, int y, char *fmt, ...);
void PrintInLeftPanel(int x, int y, char *fmt, ...);
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
char PromptForUsecodeEvent(char *event, int *index);

#endif
