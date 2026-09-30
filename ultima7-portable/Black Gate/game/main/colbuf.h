#ifndef COLBUF_H
#define COLBUF_H

#include "init.h"

struct DataFile;

/* A singly linked node. */
struct Link {
	Link *next;
	Link() { next = 0; }
	virtual ~Link();
};

struct List;
void LinkList_removeAndDestroyAll(List *);

/* A singly linked list that owns its links. */
struct List {
	Link *head;
	Link *tail;
	List() { head = tail = 0; }
	~List() { LinkList_removeAndDestroyAll(this); }
};

/* A doubly linked node; the virtual destructor lets a list delete any kind of node. */
struct DoubleLink {
	DoubleLink *next, *prev;
	DoubleLink() { next = 0; prev = 0; }
	virtual ~DoubleLink();
};

struct DoubleList;
void List_removeAndDestroyAll(DoubleList *);

/* A list that owns its nodes. */
struct DoubleList {
	DoubleLink *head, *tail;
	DoubleList() { head = 0; tail = 0; }
	~DoubleList() { List_removeAndDestroyAll(this); }
};

extern char GameArgsFileName[];
extern int32_t CollisionGrid;

void NullRoutine(void);

void List_insertAtHead(DoubleList *list, DoubleLink *node);
void List_insertAtTail(DoubleList *list, DoubleLink *node);
void List_insertAfter(DoubleList *list, DoubleLink *after, DoubleLink *node);
void List_insertBefore(DoubleList *list, DoubleLink *before, DoubleLink *node);
void List_moveAfter(DoubleList *list, DoubleLink *after, DoubleLink *node);
void List_moveBefore(DoubleList *list, DoubleLink *before, DoubleLink *node);
void List_bringToFront(DoubleList *list, DoubleLink *node);
void List_sendToBack(DoubleList *list, DoubleLink *node);
void List_unlink(DoubleList *list, DoubleLink *node);
void List_removeAndDestroy(DoubleList *list, DoubleLink *node);
int16_t List_stepForward(DoubleList *list, DoubleLink **current);
int16_t List_stepBackward(DoubleList *list, DoubleLink **current);
int32_t List_count(DoubleList *list);

void LinkList_insertAtHead(List *list, Link *node);
void LinkList_append(List *list, Link *node);
void LinkList_removeAndDestroy(List *list, Link *node, Link *prev);
void LinkList_insertChainAfter(List *list, Link *after, Link *chain);
void LinkList_unlink(List *list, Link *node, Link *prev);
int16_t LinkList_stepForward(List *list, Link **current);
int32_t LinkList_count(List *list);

void ReadFileToVoodoo(DataFile *, int32_t &, int32_t);
void CreateCollisionBuffer(void);
void SaveGameArgs(void);
int8_t HasGameArgs(void);
void LoadGameArgs(void);

#endif
