#ifndef COLBUF_H
#define COLBUF_H

#include "init.h"

/* A doubly linked node. */
struct DoubleLink {
	DoubleLink *next, *prev;
	DoubleLink() { next = 0; prev = 0; }
};

struct DoubleList;
void far List_removeAndDestroyAll(DoubleList *);

/* A list that owns its nodes. */
struct DoubleList {
	DoubleLink *head, *tail;
	DoubleList() { head = 0; tail = 0; }
	~DoubleList() { List_removeAndDestroyAll(this); }
};

/* An interrupt vector taken over while the program runs. */
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

void far List_insertAtHead(DoubleList *list, DoubleLink *node);
void far List_insertAtTail(DoubleList *list, DoubleLink *node);
void far List_insertAfter(DoubleList *list, DoubleLink *after, DoubleLink *node);
void far List_insertBefore(DoubleList *list, DoubleLink *before, DoubleLink *node);
void far List_unlink(DoubleList *list, DoubleLink *node);
int far List_stepForward(DoubleList *list, DoubleLink **current);
int far List_stepBackward(DoubleList *list, DoubleLink **current);
long far List_count(DoubleList *list);

#endif
