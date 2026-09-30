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
	void *operator new(size_t);
};

struct objref;

void ShowBarkInDialogs(objref obj, char *text);
void RemoveItemDialog(objref obj);
void CloseDialogs();

struct MouseState;

uint8_t ProcessDialogInput(MouseState *state, uint8_t picking, objref *out);
uint8_t PickWorldItem(objref *out);

extern uint8_t GameRestored;
extern char *GumpMgrFileName;
extern GumpFile GumpMgrFile;
void StartNumberedDialog(int8_t number);
void OpenItemDialog(objref obj);
void EndNumberedDialog();
void RedrawDialogs();
uint8_t DisplayItemDialog(objref obj, uint8_t stats);
void ReturnDraggedItem(Panel *from, int16_t quantity);
void DropDraggedItem(objref obj, Panel *from, Panel *to, uint8_t quantity, uint8_t gump);
void DragItem(Panel *from, MouseState *state);
void ItemDialogInputLoop();
GumpNode *GetOpenItemDialogListNode(objref obj, uint8_t stats);
void UpdateOpenInventoryDialogs(int8_t force);
int8_t RunSaveDialog(uint8_t *quit);
uint8_t HandleItemDialogClick(MouseState *state, objref *out);
uint8_t HandleWorldClick(MouseState *state, objref *out);
void ShowItemNameCaption(objref obj, MouseState *state);
void AddBarkCaption(objref obj, char *text);
void CloseItemDialog(objref obj);
int16_t RunSliderGump(int16_t low, int16_t high, int16_t increment, int16_t initial, int16_t x, int16_t y);
void OpenAndLoopItemDialog(objref obj);
void StartAndLoopNumberedDialog(int8_t number);
uint8_t WaitForClick(MouseState state);
int16_t GetSliderValue(int16_t low, int16_t high, int16_t increment, int16_t initial, int16_t x, int16_t y);
int16_t GetDialogMemoryNeeded();

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
extern uint8_t DialogState;

extern Caption *BarkTexts[10];
extern uint8_t TheftReported;
extern uint8_t PickingItem;

#endif
