/* Serpent Isle SI.EXE, resident segment 3 (file offsets 0x00a90e to 0x00b6f5, 3559 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: actqueue.c */
#include <stdarg.h>
#include <stdio.h>
#include "dosio.h"
#include "objref.h"
#include "easyfile.h"
#include "debug.h"
#include "datanode.h"
#include "action.h"
#include "actqueue.h"
#include "oops.h"
#include "itemcmd.h"
#include "tools.h"
#include "camera.h"
#include "bltshape.h"

#define ACTION_SLOTS    90

inline ActionScheduler::ActionScheduler() : DataNode()
{
	count = 0;
	entries = 0;
	ActionQueueTime = 10000;
}

char *ActionFileName = "ACTION.DAT";
ActionScheduler ActionQueue;
unsigned char SuspendedActionCount = 0;
int ActionSweepDepth = 0;

void far DebugPrintfAtRow(int row, char *format, ...)
{
	va_list args;
	if (format != WorkString) {
		va_start(args, format);
		vsprintf(WorkString, format, args);
	}
	DebugPrintfAtCoords(1, row, WorkString);
}

void ActionScheduler::dump()
{
	unsigned char current;
	int row;
	current = first();
	row = 25;
	DebugPrintfAtRow(row--, "START OF %d", count);
	while (current) {
		DebugPrintfAtRow(row, "%-38.38s", entries[current].describeName());
		current = entries[current].next;
		row--;
	}
	DebugPrintfAtRow(row--, "END T=%d", ActionQueueTime);
}

char *ActionScheduler::name()
{
	return ActionFileName;
}

void ActionScheduler::save(char *dir)
{
	unsigned char current;
	int handle = CreateFileOrFail(BuildPath(dir, ActionFileName, 0));
	DosWrite(handle, -1L, 2L, &ActionQueueTime);
	DosWrite(handle, -1L, 1L, &SuspendedActionCount);
	DosWrite(handle, -1L, 2L, &count);
	for (current = first(); current; current = entries[current].next)
		entries[current].write(handle);
	DosClose(handle);
}

void ActionScheduler::readEntry(int handle, unsigned *target, int *due, unsigned char *flags, unsigned char *cursor,
	char *script)
{
	char length;
	DosRead(handle, -1L, 2L, target);
	DosRead(handle, -1L, 2L, due);
	DosRead(handle, -1L, 1L, flags);
	DosRead(handle, -1L, 1L, cursor);
	DosRead(handle, -1L, 1L, &length);
	*script = length;
	DosRead(handle, -1L, length - 1, script + 1);
}

void ActionScheduler::load(char *dir)
{
	unsigned target;
	int due;
	unsigned char flags, cursor;
	int remaining;
	char script[128];
	int handle;
	reset();
	handle = OpenFileOrFail(BuildPath(dir, ActionFileName, 0));
	DosRead(handle, -1L, 2L, &ActionQueueTime);
	DosRead(handle, -1L, 1L, &SuspendedActionCount);
	DosRead(handle, -1L, 2L, &remaining);
	while (remaining > 0) {
		readEntry(handle, &target, &due, &flags, &cursor, script);
		add(due - ActionQueueTime, target, script, flags, cursor, 0);
		remaining--;
	}
	DosClose(handle);
}

void ActionScheduler::reset()
{
	unsigned char i;
	if (!entries) {
		entries = new Action[ACTION_SLOTS];
		if (!entries)
			ReportOutOfNearMemory();
		ActionTable = entries;
		for (i = 0; i < ACTION_SLOTS; i++)
			entries[i].clear();
	} else {
		for (i = 0; i < ACTION_SLOTS; i++)
			entries[i].release();
	}
	count = 0;
	SuspendedActionCount = 0;
}

unsigned char ActionScheduler::first()
{
	return entries->next;
}

unsigned char ActionScheduler::available()
{
	unsigned char current;
	for (current = 1; current < ACTION_SLOTS && entries[current].active(); current++)
		;
	if (current >= ACTION_SLOTS)
		current = 0;
	return current;
}

void ActionScheduler::suppressBarks(unsigned target)
{
	unsigned char current;
	for (current = first(); current; current = entries[current].next)
		if (entries[current].matches(target))
			entries[current].suppressBarks();
}

void ActionScheduler::link(unsigned char current)
{
	int next;
	unsigned char previous;
	unsigned char found;
	previous = 0;
	do {
		found = 0;
		next = entries[previous].next;
		if (next) {
			if (entries[next].marked(ACTION_STOPPED))
				found = 1;
			else if (entries[next].due - ActionQueueTime <= entries[current].due - ActionQueueTime)
				found = 1;
		}
		if (found)
			previous = next;
	} while (found);
	entries[entries[previous].next].previous = current;
	entries[current].previous = previous;
	entries[current].next = entries[previous].next;
	entries[previous].next = current;
}

void ActionScheduler::unlink(unsigned char current)
{
	entries[entries[current].previous].next = entries[current].next;
	entries[entries[current].next].previous = entries[current].previous;
}

void ActionScheduler::process()
{
	unsigned char current;
	if (SuspendedActionCount == 0)
		return;
	ActionSweepDepth++;
	if (ActionSweepDepth > 1)
		return;
	while (ActionSweepDepth > 0) {
		current = first();
		while (current) {
			if (entries[current].marked(ACTION_STOPPED) && !entries[current].marked(ACTION_CANCELLED))
				entries[current].cancel();
			current = entries[current].next;
		}
		ActionSweepDepth--;
	}
}

void ActionScheduler::collect()
{
	unsigned char current, next;
	current = first();
	while (current) {
		next = entries[current].next;
		if (entries[current].marked(ACTION_CANCELLED)) {
			unlink(current);
			entries[current].release();
			count--;
		}
		current = next;
	}
}

void ActionScheduler::remove(unsigned target, unsigned char skip, unsigned char mode)
{
	unsigned char current;
	unsigned char flag;
	if (target < 8 || first() == 0)
		return;
	flag = mode ? ACTION_DONE : ACTION_HALTED;
	for (current = first(); current; current = entries[current].next) {
		if (!entries[current].marked(ACTION_CANCELLED) && entries[current].matches(target)
			&& (!skip || !entries[current].marked(ACTION_UNHELD)))
			entries[current].suspend(flag);
	}
	process();
}

void ActionScheduler::replace(unsigned target, unsigned replacement)
{
	unsigned char current;
	for (current = first(); current; current = entries[current].next)
		if (entries[current].matches(target))
			entries[current].target.off = replacement;
}

void ActionScheduler::reschedule(unsigned char current)
{
	if (!entries[current].dueNext(ActionQueueTime)) {
		unlink(current);
		link(current);
	}
}

void ActionScheduler::run(unsigned char current)
{
	entries[current].run(1);
	process();
}

void ActionScheduler::tick()
{
	unsigned char current;
	ActionQueueTime++;
	/* Pull every time back before the counter overflows */
	if (ActionQueueTime > 30000) {
		current = first();
		while (current) {
			entries[current].due -= 28000;
			current = entries[current].next;
		}
		ActionQueueTime -= 28000;
	}
}

void ActionScheduler::prepare(unsigned char *prepared)
{
	unsigned char current;
	unsigned char last = 0;
	int unusedRemoved;
	if (!*prepared) {
		unusedRemoved = 0;
		for (current = first(); current; current = entries[current].next) {
			if (entries[current].dueNow(ActionQueueTime) && !entries[current].marked(ACTION_STOPPED)
				&& !Item_isRemoved(&entries[current].target)) {
				Item_removeFromWorld(&entries[current].target);
				unusedRemoved++;
			}
			last = current;
		}
		gCamera.beginFrame();
		for (current = last; current; current = entries[current].previous) {
			if (entries[current].dueNow(ActionQueueTime) && !entries[current].marked(ACTION_STOPPED)
				&& Item_isRemoved(&entries[current].target)) {
				Item_returnToWorld(&entries[current].target);
				unusedRemoved--;
			}
		}
		*prepared = 1;
	}
	FinishFrame(0);
	SetDrawTargetScreen();
	for (current = first(); current; current = entries[current].next)
		if (entries[current].dueNow(ActionQueueTime))
			Item_addToView(&entries[current].target);
	SetDrawTargetViewport();
}

void ActionScheduler::advance()
{
	unsigned char current, next, pending, prepared;
	int limit = 100;
	prepared = 0;
	process();
	tick();
	do {
		current = first();
		pending = 0;
		while (current) {
			next = entries[current].next;
			if (!entries[current].marked(ACTION_STOPPED) && !entries[current].marked(ACTION_CANCELLED)) {
				if (!entries[current].dueNow(ActionQueueTime))
					break;
				entries[current].run(0);
				if (!entries[current].marked(ACTION_STOPPED)) {
					if (entries[current].dueNow(ActionQueueTime))
						pending = 1;
					reschedule(current);
				}
			}
			current = next;
		}
		if (pending)
			prepare(&prepared);
		process();
		collect();
	} while (pending && --limit);
	if (limit == 0)
		;   /* ran out of passes */
}

unsigned char ActionScheduler::add(int delay, unsigned target, char *script, unsigned char flags,
	unsigned char position, unsigned char initialize)
{
	unsigned char current;
	if (IsNPCHalted(target))
		return 0;
	current = available();
	if (!current)
		return 0;
	if (!entries[current].set(ActionQueueTime + delay, target, flags, (unsigned char *)script, position, initialize))
		return 0;
	link(current);
	count++;
	return current;
}

void ActionScheduler::add(unsigned target, char *script, unsigned char flags)
{
	add(1, target, script, flags, 1, 1);
}

unsigned char ActionScheduler::add(unsigned target, char *script)
{
	return add(1, target, script, 0, 1, 1);
}

unsigned char ActionScheduler::add(int delay, unsigned target, char *script)
{
	return add(delay, target, script, 0, 1, 1);
}

unsigned char ActionScheduler::contains(unsigned target)
{
	unsigned char current;
	for (current = first(); current; current = entries[current].next)
		if (entries[current].matches(target))
			return 1;
	return 0;
}

void far HaltItemScripts(unsigned target, unsigned char skip)
{
	ActionQueue.remove(target, skip, 0);
}

void far RetargetItemScripts(unsigned target, unsigned replacement)
{
	ActionQueue.replace(target, replacement);
}

void far RunActionQueue()
{
	ActionQueue.advance();
}

void far DumpActionQueue()
{
	ActionQueue.dump();
}

int far GetActionCount()
{
	return ActionQueue.count;
}

int far IsActionQueueRoom()
{
	if (ActionQueue.count < ACTION_SLOTS - 16)
		return 1;
	return 0;
}
