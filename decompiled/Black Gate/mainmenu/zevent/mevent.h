#ifndef MEVENT_H
#define MEVENT_H

/* MouseEvent types; 0 is none. */
#define MOUSE_EVENT_PRESSED     1
#define MOUSE_EVENT_RELEASED    2
#define MOUSE_EVENT_MOVED       3

/* One queued mouse event. */
struct MouseEvent {
	unsigned char type;
	unsigned char button;       /* 1 left, 2 right */
	int x;
	int y;
	char state;                 /* buttons held */
	MouseEvent() { type = 0; }
	/* True when CopyMouseEvent found an event. */
	unsigned char valid() { return type != 0; }
};

void SkipToMouseRelease(void);
MouseEvent *GetMouseEvent(void);
MouseEvent *CopyMouseEvent(MouseEvent *e);
unsigned char CheckMouseMoved(void);
void FlushMouseToRelease(void);

extern int MouseEventSize;
extern char *MouseQueueStart;
extern char *MouseQueueEnd;
extern char *MouseQueueHead;
extern char *MouseQueueTail;
extern MouseEvent LastMouseEvent;
char IsQueuedMouseEvent(char *p);
char *NextMouseSlot(char *p);
void EnqueueMouseEvent(void *e);
unsigned char DequeueMouseEvent(void *e);

#endif
