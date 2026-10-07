#ifndef KEYQUEUE_H
#define KEYQUEUE_H

namespace MainMenu {

/* Added to the second code getch returns for an extended key. */
#define KEY_EXTENDED    0x100

/* Keys read from the BIOS, each passed through an optional translation, waiting in a ring. */
struct KeyQueue {
	int16_t *keys;
	int16_t *tail;                  /* where the next key goes */
	int16_t *head;                  /* the oldest key */
	int16_t size;
	int16_t count;
	int16_t ( *translate)(int16_t key);
	KeyQueue();
	KeyQueue(int16_t length);
	~KeyQueue();
	void allocate(int16_t length);
	uint8_t peek(int16_t *key);
	uint8_t put(int16_t key);
	uint8_t get(int16_t *key);
	void reset();
	void poll();
	void setTranslate(int16_t ( *f)(int16_t key));
};

}

#endif
