#ifndef CONVMGR_H
#define CONVMGR_H

struct AnswerGump;
struct ConvMgr;
struct FaceGump;
struct TextBox;

extern ConvMgr ConversationManager;
extern int ConversationMode;
extern int FaceNPC[2];
extern int SpeakingFace;
extern int SpeakerLineCount;
extern AnswerGump *ConversationAnswers;
extern FaceGump *AvatarFace;
extern FaceGump *NPCFaces[2];
extern TextBox *TextBoxes[2];
extern int SavedScreen;
extern char ConversationShown;
extern int TextBoxLeft[2];
extern int TextBoxTop[2];
extern int TextBoxRight[2];
extern int TextBoxBottom[2];

void CloseConversationGumps(void);
unsigned char OpenConversationGumps(int mode);
void RepaintConversation(void);
unsigned char ShowNPCFace(int npc, int frame);
void ShowFirstNPCFace(void);
void HideNPCFace(int npc);
unsigned char OpenBook(int type);
void AdvanceTextBox(void);
unsigned char IsOnLeftPage(void);
void AddConversationText(char far *text);
void WaitForAnswer(void);
void WaitForTextClick(void);
void ClearSpeakerText(void);
void ClearTextBox(int n);
void GetTextBoxSize(int *width, int *lines);
void GetTextCharWidth(char c);
void ShowSignGump(int shape, char *text);
void EndConversation(void);
void RunOptionsLoop(void);
void ShowSpeaker(int npc, int frame);
void ShowFirstSpeaker(void);
void RemoveSpeaker(int npc);
void ShowSign(int shape, char *text);
void BeginConversation(int type);

#endif
