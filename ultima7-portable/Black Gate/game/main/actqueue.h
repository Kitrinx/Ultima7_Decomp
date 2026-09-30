#ifndef ACTQUEUE_H
#define ACTQUEUE_H

#include "datanode.h"

struct Action;

/* Item scripts waiting to run, kept in order of when they are due. */
struct ActionScheduler : DataNode {
	int16_t count;
	Action *entries;
	ActionScheduler();
	void dump();
	virtual char *name();
	virtual void save(char *dir);
	void readEntry(int16_t, uint16_t *, int16_t *, uint8_t *, uint8_t *, char *);
	virtual void load(char *dir);
	void reset();
	uint8_t first();
	uint8_t available();
	void link(uint8_t);
	void unlink(uint8_t);
	void process();
	void collect();
	void remove(uint16_t, uint8_t, uint8_t);
	void replace(uint16_t, uint16_t);
	void reschedule(uint8_t);
	void run(uint8_t);
	void tick();
	void prepare(uint8_t *);
	void advance();
	uint8_t add(int16_t, uint16_t, char *, uint8_t, uint8_t, uint8_t);
	void add(uint16_t, char *, uint8_t);
	uint8_t add(uint16_t, char *);
	uint8_t add(int16_t, uint16_t, char *);
	uint8_t contains(uint16_t);
};

extern ActionScheduler ActionQueue;

extern uint8_t SuspendedActionCount;
extern int16_t ActionSweepDepth;
void DebugPrintfAtRow(int16_t row, char *format, ...);
void HaltItemScripts(uint16_t target, uint8_t skip);
void RetargetItemScripts(uint16_t target, uint16_t replacement);
void RunActionQueue();
void DumpActionQueue();
int16_t GetActionCount();
int16_t IsActionQueueRoom();

extern char *const ActionFileName;

#endif
