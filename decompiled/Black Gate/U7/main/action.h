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
	unsigned char previous, next;
	ItemId target;
	int due;
	unsigned char flags, cursor;
	unsigned char *script;
	unsigned char marked(unsigned char mask) { return flags & mask; }
	unsigned char finished() { return cursor >= script[0]; }
	unsigned char matches(unsigned id) { return target.off == id; }
	unsigned char dueNow(int tick) { return due == tick; }
	unsigned char dueNext(int tick) { return due == tick + 1; }
	char active() { return script != 0; }
	void setTable(Action *);
	char *describe();
	char *describeName();
	void write(int);
	void clear();
	void setScript(unsigned char *);
	void release();
	void cancel();
	void suspend(unsigned char);
	unsigned char setFlag(unsigned char);
	void execute(unsigned char);
	void prepare();
	void set(int, unsigned, unsigned char, unsigned char *, unsigned char, unsigned char);
	unsigned char canRun();
	void run(unsigned char);
	void delay();
};

extern Action *ActionTable;
extern unsigned char *EmptyScript;
extern int ActionQueueTime;
char *far DescribeScript(char *script);

#endif
