/* Serpent Isle SI.EXE, overlay segment 323 (file offsets 0x090770 to 0x091639, 3785 bytes).
 * Borland C++ 2.0 -mm -O -P -d -Y rebuilds it byte for byte as C++.
 */

/* path: convmgr.c */
#include "objref.h"
#include "lowlevel.h"
#include "u7manage.h"
#include "colbuf.h"
#include "keywords.h"
#include "gumps.h"
#include "signgump.h"
#include "combat.h"
#include "itemcmd.h"
#include "camera.h"
#include "mevent.h"
#include "oops.h"
#include "sounds.h"
#include "mouse.h"
#include "itable.h"
#include "init.h"
#include "u7event.h"
#include "npcref.h"
#include "palctrl.h"
#include "palfade.h"
#include "sprite.h"
#include "voice.h"
#include "convmgr.h"
#include "u7point.h"

struct View;
struct ConversationObject;

struct Rect : Point {
	int x1, y1;
	Rect(int a, int b, int c, int d) : Point(a, b) { x1 = c; y1 = d; }
	int width() { return x1 - x + 1; }
};

struct FaceGump : Control {
	int shape;
	char unusedWhere[8];
	int frame;
	FaceGump(int x, int y);
	FaceGump(Rect &r);
	void set(int shape, int frame, unsigned char absolute);
	void moveTo(int, int);
	unsigned char handle(MouseState *);
	void draw(View *);
};

/* the part of the answers gump that holds the answers */
struct AnswerPane : Control {
	char unusedLayout[11];
	void layout();
	void moveTo(int, int);
	unsigned char handle(MouseState *);
	void draw(View *);
};

/* the gump that lists the answers */
struct AnswerGump : Control {
	AnswerPane options;
	char unusedBounds[8];
	int unusedWord;
	AnswerGump(int x0, int y0, int x1, int y1);
	void moveTo(int, int);
	unsigned char handle(MouseState *);
	void draw(View *);
};

/* a box of text lines, `width` characters each */
struct TextBox : Control {
	Rect r;
	int used;
	char *text;
	int width;
	int height;
	char centered;
	TextBox(Rect &r, int width, int height);
	~TextBox();
	void add(char far *s);
	void moveTo(int, int);
	unsigned char handle(MouseState *);
	void draw(View *);
	int lines();
};

extern View Viewport;
extern unsigned LastUsecodeFunction;

#define CONVMGR_ERROR(line) AssertFail(__FILE__, line)

/* faces that speak alone, in mode 4 */
#define IS_LONE_FACE(npc) ((npc) == 256 || (npc) > 292 && (npc) < 297)

/* the one object of this class resets the conversation state at start-up */
struct ConvMgr {
	ConvMgr();
};

ConvMgr ConversationManager;
int ConversationMode = 0;               /* the mode, 0 when no conversation is open */
int FaceNPC[2] = { 0, 0 };              /* the npc shown in each face */
int SpeakingFace = -1;                  /* the face speaking now */
int SpeakerLineCount = 0;               /* lines added since that face began */
AnswerGump *ConversationAnswers = 0;
FaceGump *AvatarFace = 0;
FaceGump *NPCFaces[2] = { 0, 0 };
TextBox *TextBoxes[2] = { 0, 0 };
int SavedScreen = -1;                   /* the screen under the gumps */
char ConversationShown = 0;
int TextBoxLeft[2] = { 75, 75 };        /* mode 1: where each text box lies */
int TextBoxTop[2] = { 14, 129 };
int TextBoxRight[2] = { 331, 331 };
int TextBoxBottom[2] = { 74, 187 };

ConvMgr::ConvMgr()
{
	int i;

	ConversationMode = 0;
	for (i = 0; i < 2; i++) {
		NPCFaces[i] = 0;
		TextBoxes[i] = 0;
		FaceNPC[i] = -1;
	}
	AvatarFace = 0;
	ConversationAnswers = 0;
	SpeakingFace = -1;
}

/* close every face and box and put the screen back */
void CloseConversationGumps(void)
{
	int i;

	if (ConversationMode > 1)
		WaitForTextClick();
	for (i = 0; i < 2; i++) {
		if (NPCFaces[i]) {
			delete NPCFaces[i];
			NPCFaces[i] = 0;
		}
		if (TextBoxes[i]) {
			delete TextBoxes[i];
			TextBoxes[i] = 0;
		}
		FaceNPC[i] = -1;
	}
	if (AvatarFace) {
		delete AvatarFace;
		AvatarFace = 0;
	}
	if (ConversationAnswers) {
		delete ConversationAnswers;
		ConversationAnswers = 0;
		ClearAllBarks(-1);
	}
	OfferedAnswers.clear();
	ConversationMode = 0;
	SpeakingFace = -1;
	YellowTextPrinter.setFont(0);
	if (CursorBase != 48)
		SelectMouseCursor(GameInput.isGumpMode() ? 0 : !IsAvatarInCombat() ? 8 : 32);
	if (SavedScreen != -1) {
		gShapeManager.releaseBlock(SavedScreen);
		SavedScreen = -1;
	}
}

/* save the screen and build the gumps of a mode; 0 when memory runs out */
unsigned char OpenConversationGumps(int mode)
{
	View *saved;

	SetFixedPalette(0);
	SelectMouseCursor(0);
	gCamera.beginFrame();
	SavedScreen = gShapeManager.allocateView(0, 0, 319, 199);
	saved = gShapeManager.lockView(SavedScreen);
	CopyView(&Viewport, saved);
	AvatarFace = new FaceGump(71, 195);
	if (!AvatarFace) {
		ReportOutOfNearMemory();
		return 0;
	}
	AvatarFace->set(0, 0, 0);
	ConversationAnswers = new AnswerGump(76, 122, 261, 198);
	if (!ConversationAnswers)
		return 0;
	if (mode == 1) {
		int dx = -4, dy = 42;
		for (int i = 0; i < 2; i++) {
			NPCFaces[i] = new FaceGump(Rect(0, TextBoxTop[i], TextBoxLeft[i] + dx, TextBoxTop[i] + dy));
			if (!NPCFaces[i])
				return 0;
			TextBoxes[i] = new TextBox(Rect(TextBoxLeft[i], TextBoxTop[i], TextBoxRight[i], TextBoxBottom[i]), 40, 5);
			if (!TextBoxes[i])
				return 0;
			dx = -4;
			dy = 42;
		}
		YellowTextPrinter.setFont(0);
	} else if (mode == 2 || mode == 5) {
		NPCFaces[0] = new FaceGump(12, 1);
		if (!NPCFaces[0])
			return 0;
		TextBoxes[0] = new TextBox(Rect(64, 36, 206, 146), 40, 21);
		if (!TextBoxes[0])
			return 0;
		if (mode == 5 && !Npc_hasReadFlag(&AvatarRef))
			YellowTextPrinter.setFont(8);
		else
			YellowTextPrinter.setFont(4);
		mode = 2;
	} else if (mode == 3 || mode == 6) {
		NPCFaces[0] = new FaceGump(3, 13);
		if (!NPCFaces[0])
			return 0;
		int lines = 12;
		if (mode == 3)
			lines = 19;
		else if (Npc_hasReadFlag(&AvatarRef))
			lines = 19;
		TextBoxes[0] = new TextBox(Rect(38, 25, 162, 158), 40, lines);
		TextBoxes[1] = new TextBox(Rect(178, 25, 302, 158), 40, lines);
		if (!TextBoxes[0] || !TextBoxes[1])
			return 0;
		if (mode == 6 && !Npc_hasReadFlag(&AvatarRef))
			YellowTextPrinter.setFont(8);
		else
			YellowTextPrinter.setFont(4);
	} else {
		mode = 1;
		NPCFaces[0] = new FaceGump(0, 0);
		if (!NPCFaces[0])
			return 0;
		TextBoxes[0] = new TextBox(Rect(55, 160, 265, 195), 40, 4);
		if (!TextBoxes[0])
			return 0;
		TextBoxes[0]->centered = 1;
		YellowTextPrinter.setFont(7);
	}
	ConversationMode = mode;
	return 1;
}

/* put the saved screen back and paint every gump over it */
void RepaintConversation(void)
{
	int i;

	CopyView(gShapeManager.lockView(SavedScreen), &Viewport);
	for (i = 0; i < 2; i++)
		if (NPCFaces[i])
			NPCFaces[i]->paint(&Viewport);
	if (AvatarFace)
		AvatarFace->paint(&Viewport);
	for (i = 0; i < 2; i++)
		if (TextBoxes[i])
			TextBoxes[i]->paint(&Viewport);
	if (ConversationAnswers)
		ConversationAnswers->paint(&Viewport);
	CopyFrameBuffer();
}

/* show an npc's face, opening the conversation if needed, and make it the one speaking */
unsigned char ShowNPCFace(int npc, int frame)
{
	int mode;

	ConversationShown = 1;
	mode = 1;
	if (npc > 325 || npc < 0)
		npc = 1;
	if (IS_LONE_FACE(npc) || npc == 300) {
		mode = 4;
		if (ConversationMode != 0)
			CloseConversationGumps();
	} else if (ConversationMode != 0 && (IS_LONE_FACE(FaceNPC[0]) || FaceNPC[0] == 300))
		CloseConversationGumps();
	if (ConversationMode == 0 && !OpenConversationGumps(mode))
		ReportOutOfNearMemory();
	if (FaceNPC[0] == npc)
		SpeakingFace = 0;
	else if (FaceNPC[1] == npc)
		SpeakingFace = 1;
	else if (FaceNPC[0] == -1) {
		FaceNPC[0] = npc;
		SpeakingFace = 0;
	} else if (FaceNPC[1] == -1) {
		FaceNPC[1] = npc;
		SpeakingFace = 1;
	} else
		SpeakingFace = 1;
	NPCFaces[SpeakingFace]->set(npc, frame, 0);
	NPCFaces[SpeakingFace]->show();
	TextBoxes[SpeakingFace]->show();
	RepaintConversation();
	return 1;
}

void ShowFirstNPCFace(void)
{
	ShowNPCFace(FaceNPC[0], 0);
}

/* take an npc's face down */
void HideNPCFace(int npc)
{
	if (!IS_LONE_FACE(npc)) {
		if (FaceNPC[0] == npc) {
			NPCFaces[0]->hide();
			ClearTextBox(0);
			TextBoxes[0]->hide();
			FaceNPC[0] = -1;
		} else if (FaceNPC[1] == npc) {
			NPCFaces[1]->hide();
			ClearTextBox(1);
			TextBoxes[1]->hide();
			FaceNPC[1] = -1;
		}
	}
}

/* show an npc's face in the first place */
unsigned char ShowNPCFace0(int npc, int frame)
{
	int mode;

	ConversationShown = 1;
	mode = 1;
	if (npc > 325 || npc < 0)
		npc = 1;
	if (ConversationMode == 0 && !OpenConversationGumps(mode))
		ReportOutOfNearMemory();
	if (FaceNPC[0] == npc)
		SpeakingFace = 0;
	else if (FaceNPC[0] == -1) {
		FaceNPC[0] = npc;
		SpeakingFace = 0;
	}
	NPCFaces[SpeakingFace]->set(npc, frame, 0);
	NPCFaces[SpeakingFace]->show();
	TextBoxes[SpeakingFace]->show();
	RepaintConversation();
	return 1;
}

/* show an npc's face in the second place */
unsigned char ShowNPCFace1(int npc, int frame)
{
	int mode;

	ConversationShown = 1;
	mode = 1;
	if (npc > 325 || npc < 0)
		npc = 1;
	if (ConversationMode == 0 && !OpenConversationGumps(mode))
		ReportOutOfNearMemory();
	if (FaceNPC[1] == npc)
		SpeakingFace = 1;
	else if (FaceNPC[1] == -1) {
		FaceNPC[1] = npc;
		SpeakingFace = 1;
	}
	NPCFaces[SpeakingFace]->set(npc, frame, 0);
	NPCFaces[SpeakingFace]->show();
	TextBoxes[SpeakingFace]->show();
	RepaintConversation();
	return 1;
}

void HideNPCFace0(void)
{
	NPCFaces[0]->hide();
	ClearTextBox(0);
	TextBoxes[0]->hide();
	FaceNPC[0] = -1;
}

void HideNPCFace1(void)
{
	NPCFaces[1]->hide();
	ClearTextBox(1);
	TextBoxes[1]->hide();
	FaceNPC[1] = -1;
}

/* take the second face down, keeping its npc */
void ClearSecondFace(void)
{
	if (FaceNPC[1] != -1) {
		NPCFaces[1]->hide();
		ClearTextBox(1);
		TextBoxes[1]->hide();
	}
}

void SetSpeakingFace(int n)
{
	if (n >= 0 && n <= 1)
		SpeakingFace = n;
}

/* redraw the first face with another frame */
unsigned char ChangeNPCFace0(int frame)
{
	if (NPCFaces[0]->frame == frame)
		return 1;
	NPCFaces[0]->set(FaceNPC[0], frame, 0);
	NPCFaces[0]->show();
	TextBoxes[0]->show();
	RepaintConversation();
	return 1;
}

/* redraw the second face with another frame */
unsigned char ChangeNPCFace1(int frame)
{
	if (NPCFaces[1]->frame == frame)
		return 1;
	NPCFaces[1]->set(FaceNPC[1], frame, 0);
	NPCFaces[1]->show();
	RepaintConversation();
	return 1;
}

/* open a gump of text with one picture: a scroll in mode 2 or 5, a book in mode 3 or 6 */
unsigned char OpenBook(int type)
{
	int shape;

	ConversationShown = 1;
	switch (type) {
	case 797:
		type = 2;
		shape = 49;
		break;
	case 642:
		type = 3;
		shape = 27;
		break;
	case 707:
		type = 5;
		shape = 49;
		break;
	case 705:
		type = 6;
		shape = 27;
		break;
	}
	if (ConversationMode == 0 && !OpenConversationGumps(type))
		ReportOutOfNearMemory();
	SpeakingFace = 0;
	SpeakerLineCount = 0;
	NPCFaces[SpeakingFace]->set(shape + 1423, 0, 1);
	NPCFaces[SpeakingFace]->show();
	if (TextBoxes[0])
		TextBoxes[0]->show();
	if (TextBoxes[1])
		TextBoxes[1]->show();
	RepaintConversation();
	return 1;
}

/* in mode 3, move on to the next box */
void AdvanceTextBox(void)
{
	if (SpeakingFace != -1 && ConversationMode >= 2) {
		if (ConversationMode == 2)
			ClearTextBox(0);
		else if (SpeakingFace == -1)
			SpeakingFace = 0;
		else if (SpeakingFace == 0) {
			SpeakingFace = 1;
			SpeakerLineCount = 0;
		} else {
			ClearTextBox(1);
			SpeakingFace = 0;
			SpeakerLineCount = 0;
			ClearTextBox(0);
		}
	}
}

unsigned char IsOnLeftPage(void)
{
	if (ConversationMode != 3 && ConversationMode != 6)
		return 0;
	if (SpeakingFace == 0)
		return 1;
	return 0;
}

/* add a line to the box of the face speaking */
void AddConversationText(char far *text)
{
	if (SpeakingFace == -1)
		CONVMGR_ERROR(1000);
	TextBoxes[SpeakingFace]->add(text);
	SpeakerLineCount++;
}

/* show the answers and wait until one is clicked */
void WaitForAnswer(void)
{
	unsigned char done = 0;
	MouseState m;

	if (SpeakingFace == -1)
		CONVMGR_ERROR(1017);
	AvatarFace->show();
	ConversationAnswers->show();
	ConversationAnswers->options.layout();
	RepaintConversation();
	GameInput.enableKeyboardMouse();
	/* only the return below ends the loop */
	while (!done) {
		PlayAmbientSounds();
		CyclePalette();
		UpdateAndCopyMouseState(&m);
		if (m.clicked() && ConversationAnswers->handle(&m)) {
			ConversationAnswers->hide();
			AvatarFace->hide();
			return;
		}
	}
}

/* wait for a click */
void WaitForTextClick(void)
{
	GameInput.enableKeyboardMouse();
	RepaintConversation();
	SkipToMouseRelease();
	while (!UpdateAndGetMouseState()->clicked() || PollKeyToGlobalDiscarding()) {
		PlayAmbientSounds();
		CyclePalette();
	}
}

void ClearSpeakerText(void)
{
	if (SpeakingFace != -1)
		ClearTextBox(SpeakingFace);
}

/* empty a text box */
void ClearTextBox(int n)
{
	if (n >= 0 && n < 2)
		TextBoxes[n]->used = 0;
}

/* the size of the speaking face's box, in characters and lines */
void GetTextBoxSize(int *width, int *lines)
{
	if (SpeakingFace == -1)
		FatalError(__FILE__, 1096, LastUsecodeFunction);
	*width = TextBoxes[SpeakingFace]->r.width();
	*lines = TextBoxes[SpeakingFace]->lines();
}

void GetTextCharWidth(char c)
{
	YellowTextPrinter.charWidth(c);
}

/* show a gump with a message until the player clicks */
void ShowSignGump(int shape, char *text)
{
	SignGump box(shape, text);

	box.show();
	PlaySoundSimple(94);
	box.paint(&Viewport);
	SetFixedPalette(0);
	CopyFrameBuffer();
	while (!GameInput.isButtonPressed(1))
		;
}

void EndConversation(void)
{
	CloseConversationGumps();
}

void RunOptionsLoop(void)
{
	WaitForAnswer();
}

void ShowSpeaker(int npc, int frame)
{
	ShowNPCFace(npc, frame);
}

void ShowFirstSpeaker(void)
{
	ShowFirstNPCFace();
}

void RemoveSpeaker(int npc)
{
	HideNPCFace(npc);
}

void ShowSpeaker0(int npc, int frame)
{
	ShowNPCFace0(npc, frame);
}

void ShowSpeaker1(int npc, int frame)
{
	ShowNPCFace1(npc, frame);
}

void RemoveSpeaker0(void)
{
	HideNPCFace0();
}

void RemoveSpeaker1(void)
{
	HideNPCFace1();
}

void ClearSecondSpeaker(void)
{
	ClearSecondFace();
}

void SetSpeakerSlot(int n)
{
	SetSpeakingFace(n);
}

void ChangeSpeaker0(int frame)
{
	ChangeNPCFace0(frame);
}

void ChangeSpeaker1(int frame)
{
	ChangeNPCFace1(frame);
}

void ShowSign(int shape, char *text)
{
	ShowSignGump(shape, text);
}

void BeginConversation(int type)
{
	OpenBook(type);
}
