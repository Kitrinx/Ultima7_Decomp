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
void far LinkList_removeAndDestroyAll(List *);

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
void far List_removeAndDestroyAll(DoubleList *);

/* A list that owns its nodes. */
struct DoubleList {
	DoubleLink *head, *tail;
	DoubleList() { head = 0; tail = 0; }
	~DoubleList() { List_removeAndDestroyAll(this); }
};

/* An interrupt vector taken over while the game runs. */
struct InterruptHook {
	unsigned char vector;
	void far *old;
	HookRecord rec;
	InterruptHook();
	void far *install(void far *);
	void restore();
	~InterruptHook();
	void disableBreak();
};

struct CtrlBreakTrap : InterruptHook { CtrlBreakTrap(); };
struct BiosHook : InterruptHook { BiosHook(); };
struct CtrlCTrap : InterruptHook { CtrlCTrap(); };
struct DivideTrap : InterruptHook { DivideTrap(); };

extern char IretStub;
extern char GameArgsFileName[];
extern long CollisionGrid;

void interrupt NullInterrupt(void);
void NullRoutine(void);
void interrupt HandleDivideByZero(void);
void far SetDosVerify(char);

void far List_insertAtHead(DoubleList *list, DoubleLink *node);
void far List_insertAtTail(DoubleList *list, DoubleLink *node);
void far List_insertAfter(DoubleList *list, DoubleLink *after, DoubleLink *node);
void far List_insertBefore(DoubleList *list, DoubleLink *before, DoubleLink *node);
void far List_moveAfter(DoubleList *list, DoubleLink *after, DoubleLink *node);
void far List_moveBefore(DoubleList *list, DoubleLink *before, DoubleLink *node);
void far List_bringToFront(DoubleList *list, DoubleLink *node);
void far List_sendToBack(DoubleList *list, DoubleLink *node);
void far List_unlink(DoubleList *list, DoubleLink *node);
void far List_removeAndDestroy(DoubleList *list, DoubleLink *node);
int far List_stepForward(DoubleList *list, DoubleLink **current);
int far List_stepBackward(DoubleList *list, DoubleLink **current);
long far List_count(DoubleList *list);

void far LinkList_insertAtHead(List *list, Link *node);
void far LinkList_append(List *list, Link *node);
void far LinkList_removeAndDestroy(List *list, Link *node, Link *prev);
void far LinkList_insertChainAfter(List *list, Link *after, Link *chain);
void far LinkList_unlink(List *list, Link *node, Link *prev);
int far LinkList_stepForward(List *list, Link **current);
long far LinkList_count(List *list);

void far ReadFileToVoodoo(DataFile *, long &, long);
void far CreateCollisionBuffer(void);
void far SaveGameArgs(void);
char far HasGameArgs(void);
void far LoadGameArgs(void);

#endif
