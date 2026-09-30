/* Black Gate MAINMENU.EXE, resident segment 41 (file offsets 0x0123c8 to 0x01267e, 694 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "colbuf.h"

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
