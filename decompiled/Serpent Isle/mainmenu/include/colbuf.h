#ifndef COLBUF_H
#define COLBUF_H

#include "init.h"

/* A doubly linked node; the virtual destructor lets a list delete any kind of node. */
struct DoubleLink {
	DoubleLink *next, *prev;
	DoubleLink(DoubleLink *n = 0, DoubleLink *p = 0) { next = n; prev = p; }
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
	void setVector(unsigned char n) { vector = n; }
};

struct CtrlBreakTrap : InterruptHook { CtrlBreakTrap(); };
struct BiosHook : InterruptHook { BiosHook(); };
struct CtrlCTrap : InterruptHook { CtrlCTrap(); };
struct DivideTrap : InterruptHook { DivideTrap(); };

extern char IretStub;

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

void interrupt NullInterrupt(void);
void NullRoutine(void);
void interrupt HandleDivideByZero(void);
void far SetDosVerify(char);

#endif
