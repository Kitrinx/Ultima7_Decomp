#ifndef GUMPMGR_H
#define GUMPMGR_H

#include "gumps.h"

struct Caption;
struct GumpFile;
struct GumpList;
struct GumpMgr;
struct GumpNode;
struct SaveGump;
struct WorldGump;

/* An open gump in the manager's list. */
struct GumpNode : DoubleLink {
	Gump *dialog;
	GumpNode() { dialog = 0; }
	GumpNode(Gump *g) { dialog = g; }
	~GumpNode();
	void *operator new(unsigned);
};

struct objref;

void ShowBarkInDialogs(objref obj, char *text);
void RemoveItemDialog(objref obj);
void CloseDialogs();

struct MouseState;

unsigned char ProcessDialogInput(MouseState *state, unsigned char picking, objref *out);
unsigned char PickWorldItem(objref *out);

extern unsigned char GameRestored;
extern char *GumpMgrFileName;
extern GumpFile GumpMgrFile;
void StartNumberedDialog(char number);
void OpenItemDialog(objref obj);
void EndNumberedDialog();
void RedrawDialogs();
unsigned char DisplayItemDialog(objref obj, unsigned char stats);
void ReturnDraggedItem(Panel *from, int quantity);
void DropDraggedItem(objref obj, Panel *from, Panel *to, unsigned char quantity, unsigned char gump);
void DragItem(Panel *from, MouseState *state);
void ItemDialogInputLoop();
GumpNode *GetOpenItemDialogListNode(objref obj, unsigned char stats);
void UpdateOpenInventoryDialogs(char force);
char RunSaveDialog(unsigned char *quit);
unsigned char HandleItemDialogClick(MouseState *state, objref *out);
unsigned char HandleWorldClick(MouseState *state, objref *out);
void ShowItemNameCaption(objref obj, MouseState *state);
void AddBarkCaption(objref obj, char *text);
void CloseItemDialog(objref obj);
int RunSliderGump(int low, int high, int increment, int initial, int x, int y);
void OpenAndLoopItemDialog(objref obj);
void StartAndLoopNumberedDialog(char number);
unsigned char WaitForClick(MouseState state);
int GetSliderValue(int low, int high, int increment, int initial, int x, int y);
int GetDialogMemoryNeeded();

extern GumpMgr GumpManager;
extern GumpList OpenItemDialogsList;
extern Point *PaperdollPositions[8];
extern WorldGump *WorldArea;
extern SaveGump *OpenSaveDialog;

/* DialogState: which dialog is up, or what it is doing */
#define DIALOG_ITEM     1       /* an item's gump */
#define DIALOG_SAVE     2       /* the save and load gump */
#define DIALOG_STATS    3       /* the avatar's stats */
#define DIALOG_USING    4       /* running an item's usecode */
#define DIALOG_PICK     5       /* picking an item in the world */
#define DIALOG_DONE     6       /* closing */
extern unsigned char DialogState;

extern Caption *BarkTexts[10];
extern unsigned char TheftReported;
extern unsigned char PickingItem;

#endif
