/* Black Gate U7.EXE, resident segment 6 (file offsets 0x00d904 to 0x00e289, 2437 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y -b- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "plat.h"
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

char GameArgsFileName[] = "gameargs.dat";
int32_t CollisionGrid;

FontTextPrinter::~FontTextPrinter() {}

void FontTextPrinter::reset() {}

void FontTextPrinter::setShape(int16_t n)
{
	font = n + 1049;
}

void FontTextPrinter::drawChar(View *target, int16_t x, int16_t y, int8_t c)
{
	ShapeManager_draw(&gShapeManager, target, x, y, font, c, 0, 0);
}

int16_t FontTextPrinter::charHeight(int8_t c)
{
	int16_t width, height;
	gShapeManager.getFrameSize(&width, &height, font, c);
	return height;
}

int16_t FontTextPrinter::charWidth(int8_t c)
{
	int16_t width, height;
	gShapeManager.getFrameSize(&width, &height, font, c);
	return width;
}

void NullRoutine(void) {}

DoubleLink::~DoubleLink() {}

void List_insertAtHead(DoubleList *list, DoubleLink *node)
{
	node->prev = 0;
	node->next = list->head;
	if (list->head != 0)
		list->head->prev = node;
	else
		list->tail = node;
	list->head = node;
}

void List_insertAtTail(DoubleList *list, DoubleLink *node)
{
	node->prev = list->tail;
	node->next = 0;
	if (list->tail != 0)
		list->tail->next = node;
	else
		list->head = node;
	list->tail = node;
}

void List_insertAfter(DoubleList *list, DoubleLink *after, DoubleLink *node)
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

void List_insertBefore(DoubleList *list, DoubleLink *before, DoubleLink *node)
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

void List_moveAfter(DoubleList *list, DoubleLink *after, DoubleLink *node)
{
	List_unlink(list, node);
	List_insertAfter(list, after, node);
}

void List_moveBefore(DoubleList *list, DoubleLink *before, DoubleLink *node)
{
	List_unlink(list, node);
	List_insertBefore(list, before, node);
}

void List_bringToFront(DoubleList *list, DoubleLink *node)
{
	List_unlink(list, node);
	List_insertAfter(list, 0, node);
}

void List_sendToBack(DoubleList *list, DoubleLink *node)
{
	List_unlink(list, node);
	List_insertBefore(list, 0, node);
}

void List_unlink(DoubleList *list, DoubleLink *node)
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

void List_removeAndDestroy(DoubleList *list, DoubleLink *node)
{
	List_unlink(list, node);
	delete node;
}

void List_removeAndDestroyAll(DoubleList *list)
{
	DoubleLink *next;
	while (list->head != 0) {
		next = list->head->next;
		delete list->head;
		list->head = next;
	}
	list->head = list->tail = 0;
}

int16_t List_stepForward(DoubleList *list, DoubleLink **current)
{
	if (*current == 0)
		*current = list->head;
	else
		*current = (*current)->next;
	return *current != 0;
}

int16_t List_stepBackward(DoubleList *list, DoubleLink **current)
{
	if (*current == 0)
		*current = list->tail;
	else
		*current = (*current)->prev;
	return *current != 0;
}

int32_t List_count(DoubleList *list)
{
	int32_t count = 0;
	DoubleLink *node;
	for (node = list->head; node != 0; node = node->next)
		count++;
	return count;
}

Link::~Link() {}

void LinkList_insertAtHead(List *list, Link *node)
{
	if (node != 0) {
		node->next = list->head;
		list->head = node;
		if (list->tail == 0)
			list->tail = list->head;
	}
}

void LinkList_append(List *list, Link *node)
{
	if (node != 0) {
		if (list->tail != 0)
			list->tail->next = node;
		else
			list->head = node;
		list->tail = node;
	}
}

void LinkList_removeAndDestroy(List *list, Link *node, Link *prev)
{
	LinkList_unlink(list, node, prev);
	delete node;
}

void LinkList_insertChainAfter(List *list, Link *after, Link *chain)
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

void LinkList_unlink(List *list, Link *node, Link *prev)
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

void LinkList_removeAndDestroyAll(List *list)
{
	Link *next;
	while (list->head != 0) {
		next = list->head->next;
		delete list->head;
		list->head = next;
	}
	list->head = list->tail = 0;
}

int16_t LinkList_stepForward(List *list, Link **current)
{
	if (*current == 0)
		*current = list->head;
	else
		*current = (*current)->next;
	return *current != 0;
}

int32_t LinkList_count(List *list)
{
	int32_t count = 0;
	Link *node;
	for (node = list->head; node != 0; node = node->next)
		count++;
	return count;
}

void ReadFileToVoodoo(DataFile *f, int32_t &buffer, int32_t size)
{
	ReadHandleToVoodoo(f->handle, -INT32_C(1), size, &buffer);
}

void CreateCollisionBuffer(void)
{
	if ((CollisionGrid = AllocateVoodooMemory(&VoodooXmsBlock, INT32_C(0xa000))) == 0)
		ReportOutOfVoodooMemory();
	FillLinear(CollisionGrid, 0, INT32_C(0xa000), 0x100);
}

void SaveGameArgs(void)
{
	DataFile f;
	f.open(GameArgsFileName, FILE_CREATE);
	f.writeByte(!Npc_isMale(&AvatarRef));
	f.write(GetNpcBufferForIbo(&AvatarRef)->name, INT32_C(15));
	f.close();
}

int8_t HasGameArgs(void)
{
	return FileExists(GameArgsFileName);
}

void LoadGameArgs(void)
{
	DataFile f;
	char name[16];
	if (f.open(GameArgsFileName, FILE_OPEN) == 1) {
		Npc_setMale(&AvatarRef);
		if (f.readByte())
			Npc_setFemale(&AvatarRef);
		f.read(name, INT32_C(15));
		_fstrcpy(GetNpcBufferForIbo(&AvatarRef)->name, name);
		f.close();
	}
	plat_file_remove(GameArgsFileName);
}

extern "C" void ResetColbufGlobals(void)
{
	memcpy(GameArgsFileName, "gameargs.dat", sizeof(GameArgsFileName));
	CollisionGrid = 0;
}
