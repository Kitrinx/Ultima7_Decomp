#ifndef ACTQUEUE_H
#define ACTQUEUE_H

#include "datanode.h"

struct Action;

/* Item scripts waiting to run, kept in order of when they are due. */
struct ActionScheduler : DataNode {
	int count;
	Action *entries;
	ActionScheduler();
	void dump();
	virtual char *name();
	virtual void save(char *dir);
	void readEntry(int, unsigned *, int *, unsigned char *, unsigned char *, char *);
	virtual void load(char *dir);
	void reset();
	unsigned char first();
	unsigned char available();
	void link(unsigned char);
	void unlink(unsigned char);
	void process();
	void collect();
	void remove(unsigned, unsigned char, unsigned char);
	void replace(unsigned, unsigned);
	void reschedule(unsigned char);
	void run(unsigned char);
	void tick();
	void prepare(unsigned char *);
	void advance();
	unsigned char add(int, unsigned, char *, unsigned char, unsigned char, unsigned char);
	void add(unsigned, char *, unsigned char);
	unsigned char add(unsigned, char *);
	unsigned char add(int, unsigned, char *);
	unsigned char contains(unsigned);
};

extern ActionScheduler ActionQueue;

extern unsigned char SuspendedActionCount;
extern int ActionSweepDepth;
void far DebugPrintfAtRow(int row, char *format, ...);
void far HaltItemScripts(unsigned target, unsigned char skip);
void far RetargetItemScripts(unsigned target, unsigned replacement);
void far RunActionQueue();
void far DumpActionQueue();
int far GetActionCount();
int far IsActionQueueRoom();

extern char *ActionFileName;

#endif
