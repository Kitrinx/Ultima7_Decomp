#ifndef KEYQUEUE_H
#define KEYQUEUE_H

/* Added to the second code getch returns for an extended key. */
#define KEY_EXTENDED    0x100

/* Keys read from the BIOS, each passed through an optional translation, waiting in a ring. */
struct KeyQueue {
	int *keys;
	int *tail;                  /* where the next key goes */
	int *head;                  /* the oldest key */
	int size;
	int count;
	int (far *translate)(int key);
	KeyQueue();
	KeyQueue(int length);
	~KeyQueue();
	void allocate(int length);
	unsigned char peek(int *key);
	unsigned char put(int key);
	unsigned char get(int *key);
	void reset();
	void poll();
	void setTranslate(int (far *f)(int key));
};

#endif
