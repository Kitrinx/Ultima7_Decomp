#ifndef CONVMGR_H
#define CONVMGR_H

struct AnswerGump;
struct ConvMgr;
struct FaceGump;
struct TextBox;

extern ConvMgr ConversationManager;
extern int16_t ConversationMode;
extern int16_t FaceNPC[2];
extern int16_t SpeakingFace;
extern int16_t SpeakerLineCount;
extern AnswerGump *ConversationAnswers;
extern FaceGump *AvatarFace;
extern FaceGump *NPCFaces[2];
extern TextBox *TextBoxes[2];
extern int16_t SavedScreen;
extern int8_t ConversationShown;
extern const int16_t TextBoxLeft[2];
extern const int16_t TextBoxTop[2];
extern const int16_t TextBoxRight[2];
extern const int16_t TextBoxBottom[2];

void CloseConversationGumps(void);
uint8_t OpenConversationGumps(int16_t mode);
void RepaintConversation(void);
uint8_t ShowNPCFace(int16_t npc, int16_t frame);
void ShowFirstNPCFace(void);
void HideNPCFace(int16_t npc);
uint8_t ShowNPCFace0(int16_t npc, int16_t frame);
uint8_t ShowNPCFace1(int16_t npc, int16_t frame);
void HideNPCFace0(void);
void HideNPCFace1(void);
void ClearSecondFace(void);
void SetSpeakingFace(int16_t n);
uint8_t ChangeNPCFace0(int16_t frame);
uint8_t ChangeNPCFace1(int16_t frame);
uint8_t OpenBook(int16_t type);
void AdvanceTextBox(void);
uint8_t IsOnLeftPage(void);
void AddConversationText(char *text);
void WaitForAnswer(void);
void WaitForTextClick(void);
void ClearSpeakerText(void);
void ClearTextBox(int16_t n);
void GetTextBoxSize(int16_t *width, int16_t *lines);
int16_t GetTextCharWidth(int8_t c);
void ShowSignGump(int16_t shape, char *text);
void EndConversation(void);
void RunOptionsLoop(void);
void ShowSpeaker(int16_t npc, int16_t frame);
void ShowFirstSpeaker(void);
void RemoveSpeaker(int16_t npc);
void ShowSpeaker0(int16_t npc, int16_t frame);
void ShowSpeaker1(int16_t npc, int16_t frame);
void RemoveSpeaker0(void);
void RemoveSpeaker1(void);
void ClearSecondSpeaker(void);
void SetSpeakerSlot(int16_t n);
void ChangeSpeaker0(int16_t frame);
void ChangeSpeaker1(int16_t frame);
void ShowSign(int16_t shape, char *text);
void BeginConversation(int16_t type);

#endif
