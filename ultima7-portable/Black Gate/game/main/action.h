#ifndef ACTION_H
#define ACTION_H

#include "objref.h"

/* Action flags. The first four are set by the script's leading commands. */
#define ACTION_REMOVE_ITEM  0x01    /* take the item out of play when cancelled */
#define ACTION_MISSILE      0x02    /* stop the item's missile when cancelled */
#define ACTION_FINISH       0x04    /* run the rest of the script when cancelled */
#define ACTION_UNHELD       0x08    /* leave the item's scripted mark alone */
#define ACTION_RUNNING      0x10
#define ACTION_DONE         0x20
#define ACTION_HALTED       0x40
#define ACTION_STOPPED      (ACTION_DONE | ACTION_HALTED)
#define ACTION_CANCELLED    0x80

/* A queued action: a script run against a target when it falls due. */
struct Action {
	uint8_t previous, next;
	ItemId target;
	int16_t due;
	uint8_t flags, cursor;
	uint8_t *script;
	uint8_t marked(uint8_t mask) { return flags & mask; }
	uint8_t finished() { return cursor >= script[0]; }
	uint8_t matches(uint16_t id) { return target.off == id; }
	uint8_t dueNow(int16_t tick) { return due == tick; }
	uint8_t dueNext(int16_t tick) { return due == tick + 1; }
	int8_t active() { return script != 0; }
	void setTable(Action *);
	char *describe();
	char *describeName();
	void write(int16_t);
	void clear();
	void setScript(uint8_t *);
	void release();
	void cancel();
	void suspend(uint8_t);
	uint8_t setFlag(uint8_t);
	void execute(uint8_t);
	void executeScript(uint8_t);
	void prepare();
	void set(int16_t, uint16_t, uint8_t, uint8_t *, uint8_t, uint8_t);
	uint8_t canRun();
	void run(uint8_t);
	void delay();
};

extern Action *ActionTable;
extern uint8_t *const EmptyScript;
extern int16_t ActionQueueTime;
char * DescribeScript(char *script);

#endif
