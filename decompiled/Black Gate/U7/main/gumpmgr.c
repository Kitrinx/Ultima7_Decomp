/* Black Gate U7.EXE, overlay segment 340 (file offsets 0x09f590 to 0x0a1475, 7909 bytes).
 * Borland C++ 2.0 -mm -O -P -Y rebuilds it byte for byte as C++.
 */

#include <dos.h>
#include <mem.h>
#include "u7manage.h"
#include "item.h"
#include "npcref.h"
#include "coord.h"
#include "colbuf.h"
#include "gumps.h"
#include "spellbk.h"
#include "dosio.h"
#include "easyfile.h"
#include "gumpmgr.h"
#include "worldgmp.h"
#include "itable.h"
#include "datanode.h"
#include "camera.h"
#include "cheat.h"
#include "oops.h"
#include "mouse.h"
#include "bogus.h"
#include "combat.h"
#include "crime.h"
#include "palctrl.h"
#include "u7point.h"
#include "use.h"
#include "u7npc.h"
#include "u7event.h"
#include "debug.h"
#include "cast.h"
#include "itemcmd.h"
#include "itemovr1.h"
#include "look.h"
#include "text.h"
#include "palfade.h"
#include "partymov.h"
#include "sortitem.h"
#include "sounds.h"
#include "spell.h"
#include "trigger.h"
#include "usehook.h"
#include "voice.h"
#include "type.h"
#include "preload.h"
#include "systimer.h"
#include "iteminfo.h"
#include "collide.h"
#include "u7map.h"

#define TYPE(p) ((p)->typeFrame & 0x3ff)
#define FRAME(p) (((p)->typeFrame & 0x7c00) >> 10)
#define TYPE_CLASS(p) (gItemTypeInfo[TYPE(p)].typeClass)
#define IS_CLASS(p, f) ((char) (TYPE_CLASS(p) == (f)))

struct String {
	char *str;
	int len;
	String() { str = 0; len = 0; }
	~String();
};

struct GumpList : DoubleList {
	GumpList() : DoubleList() {}
	~GumpList() { List_removeAndDestroyAll(this); }
	void prepend(Gump *);
	void append(Gump *);
	void bringToFront(DoubleLink *);
};

/* floating text over the world or a dialog */
struct Caption : Control {
	int x, y;
	String message;
	Timer delay;
	Caption(int, int, char *);
	unsigned char showing() { return x != -1 && !Timer_hasFinished(&delay); }
	void draw(View *);
	void moveTo(int, int);
	unsigned char handle(MouseState *);
};

/* the save, load and options dialog */
struct SaveGump : Control {
	SaveGump();
	~SaveGump();
	void moveTo(int, int);
	unsigned char handle(MouseState *);
	void draw(View *);
};

struct Button : Control {
	int x, y, shape, frame, image, width, height;
	Button(int);
	void moveTo(int, int);
	unsigned char handle(MouseState *);
	void draw(View *);
	virtual int getFrame();
	virtual void setFrame(int);
	virtual int getShape();
	virtual void size(int *, int *);
	virtual void press();
	virtual void release();
};

struct RepeatButton : Button {
	RepeatButton(int shape) : Button(shape) {}
	unsigned char handle(MouseState *);
};

/* a dialog that asks for a number */
struct SliderGump : Control {
	ProportionalTextPrinter text;
	Button accept;
	RepeatButton down, up;
	int thumbShape, backgroundShape, endShape;
	int minimum, maximum, step;
	int left, right, x, y, value;
	SliderGump(int, int, int, int, int, int);
	void moveTo(int, int);
	void draw(View *);
	unsigned char handle(MouseState *);
};

struct GumpFile : DataNode {
	char *name();
	void load(char *dir);
	void save(char *dir);
};

/* sets up the dialog state once at startup; it holds no data of its own */
struct GumpMgr {
	GumpMgr();
};

extern View Viewport;

GumpMgr GumpManager;
GumpList OpenItemDialogsList;
Point *PaperdollPositions[8] = { 0 };
WorldGump *WorldArea = 0;
SaveGump *OpenSaveDialog = 0;
unsigned char DialogState = 0;
Caption *BarkTexts[10] = { 0 };
unsigned char TheftReported = 0;
unsigned char PickingItem = 0;
unsigned char GameRestored = 0;
char *GumpMgrFileName = "GUMPMGR.DAT";
GumpFile GumpMgrFile;

char *GumpFile::name()
{
	return GumpMgrFileName;
}

void GumpFile::save(char *path)
{
	int fd;
	int i;

	fd = CreateFileOrFail(BuildPath(path, GumpMgrFileName, 0));
	for (i = 0; i < 8; i++)
		DosWrite(fd, -1L, (long) sizeof(Point), PaperdollPositions[i]);
	DosClose(fd);
}

void GumpFile::load(char *path)
{
	int fd;
	int i;

	fd = DosOpen(BuildPath(path, GumpMgrFileName, 0));
	if (fd != -1)
		for (i = 0; i < 8; i++)
			DosRead(fd, -1L, (long) sizeof(Point), PaperdollPositions[i]);
	DosClose(fd);
}

GumpNode::~GumpNode()
{
	if (dialog) {
		delete dialog;
		dialog = 0;
	}
}

void *GumpNode::operator new(unsigned)
{
	GumpNode *node = ::new GumpNode;

	if (!node)
		ReportOutOfNearMemory();
	return node;
}

void GumpList::prepend(Gump *dialog)
{
	GumpNode *node = new GumpNode(dialog);

	node->dialog = dialog;
	List_insertAtHead(this, node);
}

void GumpList::append(Gump *dialog)
{
	GumpNode *node = new GumpNode(dialog);

	node->dialog = dialog;
	List_insertAtTail(this, node);
}

void GumpList::bringToFront(DoubleLink *node)
{
	List_bringToFront(this, node);
}

GumpMgr::GumpMgr()
{
	int i;

	WorldArea = 0;
	OpenSaveDialog = 0;
	PickingItem = 0;
	DialogState = 0;
	for (i = 0; i < 8; i++) {
		PaperdollPositions[i] = new Point(0, i * 20);
		if (!PaperdollPositions[i])
			ReportOutOfNearMemory();
	}
	memset(BarkTexts, 0, sizeof(BarkTexts));
}

void StartNumberedDialog(char number)
{
	int i;

	DialogState = number;
	if (number != DIALOG_PICK) {
		SelectMouseCursor(0);
		GameInput.enableKeyboardMouse();
		SetFixedPalette(0);
	}
	GameInput.enterGumpMode();
	PickingItem = 0;
	TheftReported = 0;
	WorldArea = new WorldGump;
	if (!WorldArea)
		ReportOutOfNearMemory();
	if (number == DIALOG_SAVE) {
		ShouldExitMainGameLoop = 0;
		RunSaveDialog(&ShouldExitMainGameLoop);
		DialogState = DIALOG_DONE;
		EndNumberedDialog();
	} else if (number == DIALOG_STATS && !DisplayItemDialog(AvatarRef, 1)) {
		DialogState = DIALOG_DONE;
		EndNumberedDialog();
	}
	for (i = 0; i < 10; i++) {
		if (BarkTexts[i]) {
			delete BarkTexts[i];
			BarkTexts[i] = 0;
		}
	}
}

void OpenItemDialog(objref obj)
{
	StartNumberedDialog(DIALOG_ITEM);
	if (DialogState && !DisplayItemDialog(obj, 0)) {
		DialogState = DIALOG_DONE;
		EndNumberedDialog();
	}
}

void EndNumberedDialog()
{
	int i;

	if (DialogState != DIALOG_DONE) {
		DebugPrintfWait("Hey!! What's happening?");
		return;
	}
	if (WorldArea) {
		delete WorldArea;
		WorldArea = 0;
	}
	List_removeAndDestroyAll(&OpenItemDialogsList);
	if (OpenSaveDialog) {
		delete OpenSaveDialog;
		OpenSaveDialog = 0;
	}
	HideCursor();
	SelectMouseCursor(!IsAvatarInCombat() ? 8 : 32);
	ShowCursor();
	GameInput.leaveGumpMode();
	GameRestored = 0;
	TheftReported = 0;
	DialogState = 0;
	for (i = 0; i < 10; i++) {
		if (BarkTexts[i]) {
			delete BarkTexts[i];
			BarkTexts[i] = 0;
		}
	}
}

void RedrawDialogs()
{
	int i;

	if (WorldArea)
		WorldArea->paint(&Viewport);
	GumpNode *node = 0;
	while (List_stepBackward(&OpenItemDialogsList, (DoubleLink **) &node))
		node->dialog->paint(&Viewport);
	if (OpenSaveDialog)
		OpenSaveDialog->paint(&Viewport);
	for (i = 0; i < 10; i++)
		if (BarkTexts[i])
			BarkTexts[i]->paint(&Viewport);
	CopyFrameBuffer();
}

unsigned char DisplayItemDialog(objref obj, unsigned char stats)
{
	GumpNode *node = 0;
	InventoryGump *doll;
	ContainerGump *container;
	StatsGump *statsGump;
	Spellbook *book;
	Gump *dialog;

	if (!Item_canBeOpened(obj))
		return 0;
	if (IsNpcUnconscious(&AvatarRef) && !stats)
		return 0;
	int n;

	while (List_stepForward(&OpenItemDialogsList, (DoubleLink **) &node)) {
		if (node->dialog->object() == obj
			&& ((!node->dialog->stats && !stats) || (node->dialog->stats && stats))) {
			OpenItemDialogsList.bringToFront(node);
			node->dialog->Control::hide();
			RedrawDialogs();
			node->dialog->Control::show();
			return 0;
		}
	}
	if (stats) {
		statsGump = new StatsGump(obj);
		if (!statsGump)
			return 0;
		dialog = statsGump;
		n = GetPartyMemberIndex(&NPCRef(obj));
		if (n > 8)
			n = 7;
		dialog->moveTo(PaperdollPositions[n]->x + 3, PaperdollPositions[n]->y + 3);
	} else if (IS_CLASS(ITEM(obj.off), TYPE_CLASS_HUMAN)) {
		doll = new InventoryGump(NPCRef(obj));
		if (!doll)
			return 0;
		dialog = doll;
		n = GetPartyMemberIndex(&NPCRef(obj));
		if (n > 8)
			n = 7;
		dialog->moveTo(PaperdollPositions[n]->x, PaperdollPositions[n]->y);
		int who = GetGroupLeader(1);
		if (who != -1) {
			objref npc;

			GetNpcIbo(&npc, who);
			if (npc == obj)
				doll->protectButton.setFrame(1);
		}
	} else if (IS_CLASS(ITEM(obj.off), TYPE_CLASS_SPELLBOOK)) {
		book = new Spellbook(obj);
		if (!book)
			return 0;
		dialog = book;
	} else {
		container = new ContainerGump(obj);
		if (!container)
			return 0;
		dialog = container;
	}
	OpenItemDialogsList.prepend(dialog);
	PlaySoundSimple(14);
	RedrawDialogs();
	return 1;
}

void ReturnDraggedItem(Panel *from, int quantity)
{
	int x, y;
	unsigned char mover;

	if (!quantity)
		return;
	x = from->mouseX();
	y = from->mouseY();
	if (from == WorldArea) {
		x += WorldArea->dragX();
		y += WorldArea->dragY();
	}
	mover = HackMoverEnabled;
	HackMoverEnabled = 1;
	if (!from->accepts(from->selected(), x, y)) {
		PlaceItem(&from->selected(), Item_getX(AvatarRef), Item_getY(AvatarRef), Item_getZ(&AvatarRef));
	}
	HackMoverEnabled = mover;
	if (quantity > -1) {
		Item_setQuantity(from->selected(), quantity, 0);
	}
}

void DropDraggedItem(objref obj, Panel *from, Panel *to, unsigned char quantity, unsigned char gump)
{
	unsigned char done = 0;
	int mx, my, dx, dy;
	Loc x, y;

	from->dirty = 1;
	to->dirty = 1;
	mx = MouseState_getX(GetLastMouseState());
	my = GetLastMouseState()->y;
	dx = from->dragX();
	dy = from->dragY();
	to->setDragX(dx);
	to->setDragY(dy);
	x = Item_getX(obj);
	y = Item_getY(obj);
	if (quantity && (unsigned char)Item_getQuantity(&obj) > 1 && (from != to || !gump)) {
		objref dragged;
		int count;
		int total;

		total = (unsigned char)Item_getQuantity(&obj);
		CloneItem(&obj);
		GetItemBeingDragged(&dragged);
		count = RunSliderGump(0, total, 1, total, -1, -1);
		Item_setQuantity(dragged, count, 0);
		if (count) {
			Item_setQuantity(obj, total - count, 0);
			if (!to->accepts(dragged, mx, my)) {
				count = 0;
				ZapDetachedItem(&dragged);
			} else {
				if (to == WorldArea) {
					done = TriggerEggsUnderItem(dragged, Item_getX(dragged), Item_getY(dragged));
				}
				if (from == WorldArea && !done) {
					TriggerEggsUnderItem(dragged, x, y);
				}
				if (from == WorldArea) {
					SettleChunkItems(x, y);
					Item_setQualityFlags(&dragged, Item_getQualityFlags(&dragged) & 0xef);
				}
			}
		}
		ReturnDraggedItem(from, total - count);
	} else {
		if (to->accepts(obj, mx, my)) {
			if (to == WorldArea) {
				done = TriggerEggsUnderItem(obj, Item_getX(obj), Item_getY(obj));
			}
			if (from == WorldArea && !done) {
				done = TriggerEggsUnderItem(obj, x, y);
			}
			if (from == WorldArea) {
				SettleChunkItems(x, y);
			}
		} else
			ReturnDraggedItem(from, -1);
	}
	if (done)
		DialogState = DIALOG_DONE;
	PlaySoundSimple(73);
}

void DragItem(Panel *from, MouseState *state)
{
	unsigned char result = 0;
	unsigned char gump;
	Panel *to = 0;
	objref obj = from->selected();
	GumpNode *node;
	unsigned char quantity;

	Item_detach(&obj);
	{
		ItemDrag drag(obj, MouseState_getX(state), state->y, from->dragX(), from->dragY());

		drag.Control::hide();
		RedrawDialogs();
		drag.Control::show();
		drag.drag(state);
	}
	node = 0;
	quantity = IS_CLASS(ITEM(obj.off), TYPE_CLASS_QUANTITY);
	while (List_stepForward(&OpenItemDialogsList, (DoubleLink **) &node)) {
		result = node->dialog->handle(GetLastMouseState());
		if (result == GUMP_NO_DROP) {
			if (quantity) {
				ReturnDraggedItem(from, (unsigned char)Item_getQuantity(&obj));
				return;
			}
			ReturnDraggedItem(from, -1);
			return;
		}
		if (result == GUMP_DROP_HERE) {
			to = node->dialog;
			gump = 1;
			break;
		}
	}
	if (!to) {
		to = WorldArea;
		gump = 0;
	}
	DropDraggedItem(obj, from, to, quantity, gump);
}

void ItemDialogInputLoop()
{
	objref selected;
	int i;

	while (DialogState != DIALOG_DONE && DialogState != 0) {
		CyclePalette();
		ContinuePlayingSpeech();
		PlayAmbientSounds();
		MouseState state;
		UpdateAndCopyMouseState(&state);
		if (state.valid() && !state.moving())
			ProcessDialogInput(&state, 0, &selected);
		if (PollKeyToGlobalDiscarding()) {
			int key = GetPolledKey();
			unsigned char stats = 1;
			objref npc;
			unsigned char done;
			int n;

			switch (key) {
			case 'I':
			case 'i':
				stats = 0;
			case 'Z':
			case 'z':
				done = 0;
				for (n = 0; !done; n++) {
					if (n > 10)
						done = 1;
					else {
						GetNpcIbo(&npc, n);
						if (IsInParty(&npc) && !GetOpenItemDialogListNode(npc, stats)) {
							DisplayItemDialog(npc, stats);
							done = 1;
						}
					}
				}
				break;
			case 27:    /* Esc */
				DialogState = DIALOG_DONE;
				break;
			}
		}
		for (i = 0; i < 10; i++) {
			if (BarkTexts[i] && !BarkTexts[i]->showing()) {
				delete BarkTexts[i];
				BarkTexts[i] = 0;
				RedrawDialogs();
			}
		}
	}
	DialogState = DIALOG_DONE;
	EndNumberedDialog();
}

GumpNode *GetOpenItemDialogListNode(objref obj, unsigned char stats)
{
	GumpNode *node = 0;

	if (!obj.valid())
		return node;
	while (List_stepForward(&OpenItemDialogsList, (DoubleLink **) &node))
		if (node->dialog->object() == obj
			&& ((!node->dialog->stats && !stats) || (node->dialog->stats && stats)))
			return node;
}

void UpdateOpenInventoryDialogs(char force)
{
	GumpNode *node = 0;

	while (List_stepForward(&OpenItemDialogsList, (DoubleLink **) &node))
		node->dialog->refresh(force);
}

char RunSaveDialog(unsigned char *quit)
{
	MouseState state;
	unsigned char handled = 0;
	unsigned char result = 0;

	OpenSaveDialog = new SaveGump;
	if (!OpenSaveDialog) {
		ReportOutOfNearMemory();
		return 0;
	}
	SetFixedPalette(0);
	RedrawDialogs();
	while (!handled) {
		UpdateAndCopyMouseState(&state);
		result = OpenSaveDialog->handle(&state);
		handled = 1;
		if (result == GUMP_RESTORED)
			DialogState = DIALOG_DONE;
		else if (result == GUMP_QUIT) {
			DialogState = DIALOG_DONE;
			*quit = 1;
		} else if (result != GUMP_CLOSE)
			handled = 0;
	}
	delete OpenSaveDialog;
	OpenSaveDialog = 0;
	RedrawDialogs();
	return 1;
}

unsigned char HandleItemDialogClick(MouseState *state, objref *out)
{
	unsigned char result = 0;
	GumpNode *node = 0;
	Gump *dialog;
	int who;

	while (List_stepForward(&OpenItemDialogsList, (DoubleLink **) &node)) {
		result = node->dialog->handle(state);
		if (PickingItem && result != GUMP_SELECT_ITEM && result != 2 && result != GUMP_CLOSE
			&& result != GUMP_MOVE && result != GUMP_OPEN_ITEM && result != 0)
			break;
		if (result == 0)
			continue;
		switch (result) {
		case GUMP_CLOSE:
		case 2:
			List_removeAndDestroy(&OpenItemDialogsList, node);
			if (!OpenItemDialogsList.head)
				DialogState = DIALOG_DONE;
			break;
		case GUMP_OPEN_ITEM:
			objref selected = node->dialog->selected();
			DisplayItemDialog(selected, 0);
			if (PickingItem)
				DialogState = DIALOG_ITEM;
			break;
		case GUMP_DRAG_ITEM:
			DragItem(node->dialog, state);
			UpdateOpenInventoryDialogs(1);
			break;
		case GUMP_SELECT_ITEM:
			if (PickingItem) {
				*out = node->dialog->selected();
			} else {
				ShowItemNameCaption(node->dialog->selected(), state);
			}
			break;
		case GUMP_USE_ITEM: {
			objref used = node->dialog->selected();
			unsigned char savedState = DialogState;

			DialogState = DIALOG_USING;
			RunUsable(1, used.off, -1);
			if (DialogState != DIALOG_DONE)
				DialogState = savedState;
			UpdateOpenInventoryDialogs(1);
			break;
		}
		case GUMP_MOVE: {
			objref moved;

			moved = node->dialog->object();
			dialog = node->dialog;
			OpenItemDialogsList.bringToFront(node);
			dialog->Control::save();
			dialog->Control::hide();
			dialog->dragging = 1;
			RedrawDialogs();
			dialog->Control::restore();
			dialog->Control::show();
			dialog->drag(state);
			dialog->dragging = 0;
			if (IS_CLASS(ITEM(moved.off), TYPE_CLASS_HUMAN)) {
				int n = GetPartyMemberIndex(&NPCRef(moved));

				if (n > 8)
					n = 7;
				PaperdollPositions[n]->x = dialog->bounds.x;
				PaperdollPositions[n]->y = dialog->bounds.y;
			}
			break;
		}
		case GUMP_SAVE_DIALOG:
			RunSaveDialog(&ShouldExitMainGameLoop);
			break;
		case GUMP_STATS:
			objref opened = node->dialog->object();
			DisplayItemDialog(opened, 1);
			break;
		case GUMP_SET_LEADER:
			objref npc;
			objref member;
			member = node->dialog->object();
			GumpNode *found;
			if (GetPartySize() > 1) {
				who = GetGroupLeader(1);
				if (who != -1) {
					GetNpcIbo(&npc, who);
					found = GetOpenItemDialogListNode(npc, 0);
					dialog = found->dialog;
					((InventoryGump *) dialog)->protectButton.setFrame(0);
				}
				who = Item_getNpcNumber(&member);
				SetGroupLeader(who);
			}
			break;
		case GUMP_CLEAR_LEADER:
			ClearGroupLeader(1);
			ReleaseProtectors(1, -1);
			break;
		case GUMP_CAST_SPELL:
			objref caster;
			unsigned char spell;
			caster = node->dialog->object();
			List_removeAndDestroy(&OpenItemDialogsList, node);
			spell = GetSpellbookBookmark(&NPCRef(caster));
			RedrawDialogs();
			TryToCastSpell(AvatarRef, spell, 1, 1, 1);
			DialogState = DIALOG_DONE;
			break;
		case GUMP_REFRESH:
			UpdateOpenInventoryDialogs(1);
			break;
		}
		break;
	}
	return result;
}

unsigned char HandleWorldClick(MouseState *state, objref *out)
{
	unsigned char result = WorldArea->handle(state);

	if (PickingItem && result != GUMP_SELECT_ITEM && result != GUMP_OPEN_ITEM && result != GUMP_PICK_GROUND)
		return 0;
	if (result != GUMP_HANDLED && result != GUMP_PICK_GROUND) {
		if (!CanAvatarReach(WorldArea->selected(), 0)) {
			ReportNoCanDo(7);
			result = GUMP_HANDLED;
			if (PickingItem) {
				out->off = 0;
				return 0;
			}
		}
	}
	switch (result) {
	case GUMP_OPEN_ITEM:
		objref selected = WorldArea->selected();
		if ((unsigned char)Item_isAvatar(&NPCRef(selected)) && IsNpcUnconscious(&NPCRef(selected))) {
			RunSaveDialog(&ShouldExitMainGameLoop);
		} else {
			DisplayItemDialog(selected, 0);
		}
		break;
	case GUMP_DRAG_ITEM:
		DragItem(WorldArea, state);
		UpdateOpenInventoryDialogs(1);
		break;
	case GUMP_SELECT_ITEM:
		if (PickingItem) {
			*out = WorldArea->selected();
		} else {
			ShowItemNameCaption(WorldArea->selected(), state);
		}
		break;
	case GUMP_USE_ITEM: {
		objref used = WorldArea->selected();
		unsigned char savedState = DialogState;

		DialogState = DIALOG_USING;
		RunUsable(1, used.off, -1);
		if (DialogState != DIALOG_DONE)
			DialogState = savedState;
		UpdateOpenInventoryDialogs(1);
		break;
	}
	}
	return result;
}

unsigned char ProcessDialogInput(MouseState *state, unsigned char picking, objref *out)
{
	unsigned char result = 0;

	PickingItem = picking;
	if (PickingItem)
		out->off = 0;
	if (state->clicked()) {
		result = HandleItemDialogClick(state, out);
		if (result == 0)
			result = HandleWorldClick(state, out);
		if (result != 0) {
			RedrawDialogs();
			if (PickingItem && (result == GUMP_SELECT_ITEM || result == GUMP_PICK_GROUND))
				return 1;
		}
	}
	return 0;
}

void ShowItemNameCaption(objref obj, MouseState *state)
{
	char name[80];
	int height, width;
	int x, y;
	unsigned char quantity;
	int shape, frame;
	char counted;
	int i;

	if (obj.valid()) {
		x = MouseState_getX(state);
		y = state->y;
		shape = TYPE(ITEM(obj.off));
		frame = FRAME(ITEM(obj.off));
		counted = IS_CLASS(ITEM(obj.off), TYPE_CLASS_QUANTITY);
		if (counted)
			quantity = (unsigned char)Item_getQuantity(&obj);
		else
			quantity = 0;
		ProduceItemDisplayName(name, shape, quantity, frame);
		width = YellowTextPrinter.textWidth(name);
		height = YellowTextPrinter.charHeight(name[0]);
		if (x + width > 319)
			x = 319 - width;
		if (y - height < 0)
			y = height;
		for (i = 0; i < 10; i++) {
			if (!BarkTexts[i]) {
				BarkTexts[i] = new Caption(x, y, name);
				if (!BarkTexts[i])
					ReportOutOfNearMemory();
				break;
			}
		}
		RedrawDialogs();
	}
}

void AddBarkCaption(objref obj, char *text)
{
	GumpNode *node = 0;
	int x, y;
	int z = 0;
	int found = 0;
	int i;

	while (List_stepForward(&OpenItemDialogsList, (DoubleLink **) &node)) {
		if (node->dialog->findPosition(obj, &x, &y)) {
			found++;
			break;
		}
	}
	if (!found && (unsigned char)gCamera.isOnScreen(Item_getX(obj), Item_getY(obj))) {
		WorldCoordsToScreen(Item_getX(obj), Item_getY(obj), &x, &y);
		z = Item_getZ(&obj);
		z *= 4;
		found++;
	}
	if (found) {
		for (i = 0; i < 10; i++) {
			if (!BarkTexts[i]) {
				x -= z + 10;
				y -= z + 10;
				BarkTexts[i] = new Caption(x, y, text);
				if (!BarkTexts[i])
					ReportOutOfNearMemory();
				BarkTexts[i]->paint(&Viewport);
				CopyFrameBuffer();
				break;
			}
		}
	}
}

void ShowBarkInDialogs(objref obj, char *text)
{
	AddBarkCaption(obj, text);
}

void CloseItemDialog(objref obj)
{
	GumpNode *node = 0;

	while (List_stepForward(&OpenItemDialogsList, (DoubleLink **) &node)) {
		if (node->dialog->object() == obj) {
			List_removeAndDestroy(&OpenItemDialogsList, node);
			break;
		}
	}
}

void RemoveItemDialog(objref obj)
{
	CloseItemDialog(obj);
}

int RunSliderGump(int low, int high, int increment, int initial, int x, int y)
{
	unsigned char result = 0;
	int value;
	MouseState state;
	SliderGump box(low, high, increment, initial, x, y);

	box.show();
	box.paint(&Viewport);
	SetFixedPalette(0);
	CopyFrameBuffer();
	GameInput.enableKeyboardMouse();
	while (result != GUMP_CLOSE) {
		UpdateAndCopyMouseState(&state);
		if (state.clicked()) {
			result = box.handle(&state);
			if (result == GUMP_REDRAW) {
				box.paint(&Viewport);
				CopyFrameBuffer();
			}
		}
		CyclePalette();
		ContinuePlayingSpeech();
	}
	value = box.value;
	RedrawDialogs();
	return value;
}

unsigned char PickWorldItem(objref *out)
{
	unsigned char dragged = 0;
	unsigned char result;

	StartNumberedDialog(DIALOG_PICK);
	HideCursor();
	MouseState state = *GetLastMouseState();
	result = WorldArea->handle(&state);
	if (result == GUMP_SELECT_ITEM)
		*out = WorldArea->selected();
	else if (result == GUMP_DRAG_ITEM) {
		ShowCursor();
		DragItem(WorldArea, &state);
		dragged = 1;
	}
	DialogState = DIALOG_DONE;
	EndNumberedDialog();
	return dragged;
}

void OpenAndLoopItemDialog(objref obj)
{
	OpenItemDialog(obj);
	ItemDialogInputLoop();
}

void StartAndLoopNumberedDialog(char number)
{
	StartNumberedDialog(number);
	ItemDialogInputLoop();
}

/* After a press: 1 if the button comes up within 30 ticks, 0 if the mouse drags first. */
unsigned char WaitForClick(MouseState state)
{
	PinMouseHere();
	CursorTracking = 0;
	Timer wait(30);
	Timer_restart(&wait);
	while (!Timer_hasFinished(&wait)) {
		UpdateAndCopyMouseState(&state);
		if (state.action == MOUSE_RELEASE) {
			CursorTracking = 1;
			UnpinMouse();
			return 1;
		}
		if (state.action == MOUSE_MOVE)
			break;
	}
	CursorTracking = 1;
	UnpinMouse();
	return 0;
}

void CloseDialogs()
{
	DialogState = DIALOG_DONE;
}

int GetSliderValue(int low, int high, int increment, int initial, int x, int y)
{
	return RunSliderGump(low, high, increment, initial, x, y);
}

int GetDialogMemoryNeeded()
{
	return 4071;
}
