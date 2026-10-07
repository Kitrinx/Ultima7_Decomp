#ifndef MEVENT_H
#define MEVENT_H

/* MouseEvent types; 0 is none. */
#define MOUSE_EVENT_PRESSED     1
#define MOUSE_EVENT_RELEASED    2
#define MOUSE_EVENT_MOVED       3

/* One queued mouse event. */
struct MouseEvent {
	uint8_t type;
	uint8_t button;       /* 1 left, 2 right */
	int16_t x;
	int16_t y;
	int8_t state;                 /* buttons held */
	MouseEvent() { type = 0; }
};

void SkipToMouseRelease(void);
MouseEvent *GetMouseEvent(void);
MouseEvent *CopyMouseEvent(MouseEvent *e);
uint8_t CheckMouseMoved(void);
void FlushMouseToRelease(void);

extern int16_t MouseEventSize;
extern char *MouseQueueStart;
extern char *MouseQueueEnd;
extern char *MouseQueueHead;
extern char *MouseQueueTail;
extern MouseEvent LastMouseEvent;
int8_t IsQueuedMouseEvent(char *p);
char *NextMouseSlot(char *p);
void EnqueueMouseEvent(void *e);
uint8_t DequeueMouseEvent(void *e);

#endif
