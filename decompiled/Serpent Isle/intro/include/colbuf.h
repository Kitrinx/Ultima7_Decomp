#ifndef COLBUF_H
#define COLBUF_H

/* A doubly linked node. */
struct DoubleLink {
	DoubleLink *next, *prev;
	DoubleLink() { next = 0; prev = 0; }
};

struct DoubleList;

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
void far List_removeAndDestroyAll(DoubleList *list);
int far List_stepForward(DoubleList *list, DoubleLink **current);
int far List_stepBackward(DoubleList *list, DoubleLink **current);
unsigned long far List_count(DoubleList *list);

/* A list that owns its nodes. */
struct DoubleList {
	DoubleLink *head, *tail;
	DoubleList() { head = 0; tail = 0; }
	virtual ~DoubleList() { List_removeAndDestroyAll(this); }
	void insertAtHead(DoubleLink *node) { List_insertAtHead(this, node); }
	void insertAtTail(DoubleLink *node) { List_insertAtTail(this, node); }
	void insertAfter(DoubleLink *after, DoubleLink *node) { List_insertAfter(this, after, node); }
	void unlink(DoubleLink *node) { List_unlink(this, node); }
	void removeAll() { List_removeAndDestroyAll(this); }
	int stepForward(DoubleLink **current) { return List_stepForward(this, current); }
	unsigned long count() { return List_count(this); }
};

#endif
