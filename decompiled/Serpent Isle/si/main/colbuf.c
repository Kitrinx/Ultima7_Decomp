/* Serpent Isle SI.EXE, resident segment 5 (file offsets 0x00c250 to 0x00cc02, 2482 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y -b- rebuilds it byte for byte as C++.
 */

#include <dos.h>
#include <stdio.h>
#include <string.h>
#include "lowlevel.h"
#include "dosio.h"
#include "view.h"
#include "itable.h"
#include "u7manage.h"
#include "bltshape.h"
#include "u7npc.h"
#include "easyfile.h"
#include "chkfile.h"
#include "vooalloc.h"
#include "init.h"
#include "u7event.h"
#include "oops.h"
#include "fileutil.h"
#include "colbuf.h"

extern objref AvatarRef;
void far Npc_setSkinColor(objref *npc, unsigned char color);

char IretStub = 0xcf;   /* an IRET: a handler that ignores its interrupt */
char GameArgsFileName[] = "gameargs.dat";
unsigned char GameArgFlags = 0;
long CollisionGrid;

FontTextPrinter::~FontTextPrinter() {}

void FontTextPrinter::reset() {}

void FontTextPrinter::setShape(int n)
{
	font = n + 1086;
}

void FontTextPrinter::drawChar(View *target, int x, int y, char c)
{
	ShapeManager_draw(&gShapeManager, target, x, y, font, c, 0, 0);
}

int FontTextPrinter::charHeight(char c)
{
	int width, height;
	gShapeManager.getFrameSize(&width, &height, font, c);
	return height;
}

int FontTextPrinter::charWidth(char c)
{
	int width, height;
	gShapeManager.getFrameSize(&width, &height, font, c);
	return width;
}

InterruptHook::InterruptHook()
{
	int *p;

	rec.next = 0;
	rec.vector = -1;
	/* clear the saved handler a word at a time */
	p = &rec.vector;
	p++;
	*p = 0;
	p++;
	*p = 0;
}

void interrupt NullInterrupt(void) {}

void far *InterruptHook::install(void far *handler)
{
	HookInterruptVector(vector, (InterruptHandler)handler, &rec, (long far *)&old);
	return old;
}

void InterruptHook::restore()
{
	UnhookInterrupt(vector);
}

InterruptHook::~InterruptHook()
{
	restore();
}

void InterruptHook::disableBreak()
{
	union REGS r;
	install(&IretStub);
	if (vector == 0x23 || vector == 0x1b) {
		r.h.ah = 0x33;
		r.h.al = 1;
		r.h.dl = 0;
		int86(0x21, &r, &r);
	}
}

CtrlBreakTrap::CtrlBreakTrap()
{
	vector = 0x1b;
	disableBreak();
}

BiosHook::BiosHook()
{
	vector = 0x15;
	PrevKeyIntercept = install((void far *)(long) InterceptCtrlC);
}

void NullRoutine(void) {}

CtrlCTrap::CtrlCTrap()
{
	vector = 0x23;
	disableBreak();
}

void interrupt HandleDivideByZero(void)
{
	FatalError("Divide by Zero!");
}

DivideTrap::DivideTrap()
{
	vector = 0;
	install((void far *) HandleDivideByZero);
}

void far SetDosVerify(char enabled)
{
	union REGS r;
	r.h.ah = 0x2e;
	r.h.dl = 0;
	r.h.al = enabled ? 1 : 0;
	int86(0x21, &r, &r);
}

DoubleLink::~DoubleLink() {}

void far List_insertAtHead(DoubleList *list, DoubleLink *node)
{
	node->prev = 0;
	node->next = list->head;
	if (list->head != 0)
		list->head->prev = node;
	else
		list->tail = node;
	list->head = node;
}

void far List_insertAtTail(DoubleList *list, DoubleLink *node)
{
	node->prev = list->tail;
	node->next = 0;
	if (list->tail != 0)
		list->tail->next = node;
	else
		list->head = node;
	list->tail = node;
}

void far List_insertAfter(DoubleList *list, DoubleLink *after, DoubleLink *node)
{
	if (node != 0) {
		if (after == 0)
			List_insertAtHead(list, node);
		else {
			if (list->tail != after) {
				node->next = after->next;
				node->prev = after;
				after->next->prev = node;
			} else {
				list->tail = node;
				node->next = 0;
				node->prev = after;
			}
			after->next = node;
		}
	}
}

void far List_insertBefore(DoubleList *list, DoubleLink *before, DoubleLink *node)
{
	if (node != 0) {
		if (before == 0)
			List_insertAtTail(list, node);
		else {
			if (list->head != before) {
				node->next = before;
				node->prev = before->prev;
				before->prev->next = node;
			} else {
				list->head = node;
				node->prev = 0;
				node->next = before;
			}
			before->prev = node;
		}
	}
}

void far List_moveAfter(DoubleList *list, DoubleLink *after, DoubleLink *node)
{
	List_unlink(list, node);
	List_insertAfter(list, after, node);
}

void far List_moveBefore(DoubleList *list, DoubleLink *before, DoubleLink *node)
{
	List_unlink(list, node);
	List_insertBefore(list, before, node);
}

void far List_bringToFront(DoubleList *list, DoubleLink *node)
{
	List_unlink(list, node);
	List_insertAfter(list, 0, node);
}

void far List_sendToBack(DoubleList *list, DoubleLink *node)
{
	List_unlink(list, node);
	List_insertBefore(list, 0, node);
}

void far List_unlink(DoubleList *list, DoubleLink *node)
{
	DoubleLink *prev;
	if (node != 0) {
		prev = node->prev;
		if (prev != 0)
			prev->next = node->next;
		else
			list->head = node->next;
		if (node->next != 0)
			node->next->prev = prev;
		else {
			if (prev != 0)
				prev->next = 0;
			else
				list->head = 0;
			list->tail = prev;
		}
	}
}

void far List_removeAndDestroy(DoubleList *list, DoubleLink *node)
{
	List_unlink(list, node);
	delete node;
}

void far List_removeAndDestroyAll(DoubleList *list)
{
	DoubleLink *next;
	while (list->head != 0) {
		next = list->head->next;
		delete list->head;
		list->head = next;
	}
	list->head = list->tail = 0;
}

int far List_stepForward(DoubleList *list, DoubleLink **current)
{
	if (*current == 0)
		*current = list->head;
	else
		*current = (*current)->next;
	return *current != 0;
}

int far List_stepBackward(DoubleList *list, DoubleLink **current)
{
	if (*current == 0)
		*current = list->tail;
	else
		*current = (*current)->prev;
	return *current != 0;
}

long far List_count(DoubleList *list)
{
	long count = 0;
	DoubleLink *node;
	for (node = list->head; node != 0; node = node->next)
		count++;
	return count;
}

Link::~Link() {}

void far LinkList_insertAtHead(List *list, Link *node)
{
	if (node != 0) {
		node->next = list->head;
		list->head = node;
		if (list->tail == 0)
			list->tail = list->head;
	}
}

void far LinkList_append(List *list, Link *node)
{
	if (node != 0) {
		if (list->tail != 0)
			list->tail->next = node;
		else
			list->head = node;
		list->tail = node;
	}
}

void far LinkList_removeAndDestroy(List *list, Link *node, Link *prev)
{
	LinkList_unlink(list, node, prev);
	delete node;
}

void far LinkList_insertChainAfter(List *list, Link *after, Link *chain)
{
	Link *node, *last;
	if (after != 0 && chain != 0) {
		last = chain;
		node = chain->next;
		while (node != 0) {
			last = node;
			node = node->next;
		}
		if (list->tail != after) {
			last->next = after->next;
			after->next = chain;
		} else {
			list->tail = last;
			last->next = 0;
			after->next = chain;
		}
	}
}

void far LinkList_unlink(List *list, Link *node, Link *prev)
{
	Link *cur;

	if (prev == 0) {
		for (cur = list->head; cur != 0 && cur != node; cur = cur->next)
			prev = cur;
		if (cur == 0)
			return;
	}
	if (prev == 0)
		list->head = list->head->next;
	if (list->tail != node) {
		if (prev != 0)
			prev->next = node->next;
		if (list->head == node)
			list->head = node->next;
	} else {
		list->tail = prev;
		if (prev != 0)
			prev->next = 0;
	}
}

void far LinkList_removeAndDestroyAll(List *list)
{
	Link *next;
	while (list->head != 0) {
		next = list->head->next;
		delete list->head;
		list->head = next;
	}
	list->head = list->tail = 0;
}

int far LinkList_stepForward(List *list, Link **current)
{
	if (*current == 0)
		*current = list->head;
	else
		*current = (*current)->next;
	return *current != 0;
}

long far LinkList_count(List *list)
{
	long count = 0;
	Link *node;
	for (node = list->head; node != 0; node = node->next)
		count++;
	return count;
}

void far ReadFileToVoodoo(DataFile *f, long &buffer, long size)
{
	ReadHandleToVoodoo(f->handle, -1L, size, &buffer);
}

void far CreateCollisionBuffer(void)
{
	if ((CollisionGrid = AllocateVoodooMemory(&VoodooXmsBlock, 0xa000L)) == 0)
		ReportOutOfVoodooMemory();
	FillLinear(CollisionGrid, 0, 0xa000L, 0x100);
}

void far SaveGameArgs(void)
{
	DataFile f;
	f.open(GameArgsFileName, FILE_CREATE);
	f.writeByte(GameArgFlags);
	f.write(GetNpcBufferForIbo(&AvatarRef)->name, 15L);
	f.close();
}

char far HasGameArgs(void)
{
	return FileExists(GameArgsFileName);
}

void far LoadGameArgs(void)
{
	DataFile f;
	char name[16];

	FindAvatarNpc();
	if (f.open(GameArgsFileName, FILE_OPEN) == 1) {
		Npc_setFemale(&AvatarRef);
		GameArgFlags = f.readByte();
		if (GameArgFlags & 1)
			Npc_setMale(&AvatarRef);
		switch (GameArgFlags / 2) {
		case 1: Npc_setSkinColor(&AvatarRef, 1); break;
		case 2: Npc_setSkinColor(&AvatarRef, 2); break;
		default: Npc_setSkinColor(&AvatarRef, 0); break;
		}
		f.read(name, 15L);
		_fstrcpy(GetNpcBufferForIbo(&AvatarRef)->name, name);
		f.close();
	}
	unlink(GameArgsFileName);
}
