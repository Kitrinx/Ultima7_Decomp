/* Serpent Isle SI.EXE, resident segment 3 (file offsets 0x00a90e to 0x00b6f5, 3559 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: actqueue.c */
#include "u7port.h"
#include <new>
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
#include "vidmode.h"
#include "bltshape.h"

#define ACTION_SLOTS    90

inline ActionScheduler::ActionScheduler() : DataNode()
{
	count = 0;
	entries = 0;
	ActionQueueTime = 10000;
}

char *const ActionFileName = "ACTION.DAT";
ActionScheduler ActionQueue;
uint8_t SuspendedActionCount = 0;
int16_t ActionSweepDepth = 0;

void DebugPrintfAtRow(int16_t row, char *format, ...)
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
	uint8_t current;
	int16_t row;
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
	uint8_t current;
	int16_t handle = CreateFileOrFail(BuildPath(dir, ActionFileName, 0));
	DosWrite(handle, -INT32_C(1), INT32_C(2), &ActionQueueTime);
	DosWrite(handle, -INT32_C(1), INT32_C(1), &SuspendedActionCount);
	DosWrite(handle, -INT32_C(1), INT32_C(2), &count);
	for (current = first(); current; current = entries[current].next)
		entries[current].write(handle);
	DosClose(handle);
}

void ActionScheduler::readEntry(int16_t handle, uint16_t *target, int16_t *due, uint8_t *flags, uint8_t *cursor,
	char *script)
{
	int8_t length;
	DosRead(handle, -INT32_C(1), INT32_C(2), target);
	DosRead(handle, -INT32_C(1), INT32_C(2), due);
	DosRead(handle, -INT32_C(1), INT32_C(1), flags);
	DosRead(handle, -INT32_C(1), INT32_C(1), cursor);
	DosRead(handle, -INT32_C(1), INT32_C(1), &length);
	*script = length;
	DosRead(handle, -INT32_C(1), length - 1, script + 1);
}

void ActionScheduler::load(char *dir)
{
	uint16_t target;
	int16_t due;
	uint8_t flags, cursor;
	int16_t remaining;
	char script[128];
	int16_t handle;
	reset();
	handle = OpenFileOrFail(BuildPath(dir, ActionFileName, 0));
	DosRead(handle, -INT32_C(1), INT32_C(2), &ActionQueueTime);
	DosRead(handle, -INT32_C(1), INT32_C(1), &SuspendedActionCount);
	DosRead(handle, -INT32_C(1), INT32_C(2), &remaining);
	while (remaining > 0) {
		readEntry(handle, &target, &due, &flags, &cursor, script);
		add(due - ActionQueueTime, target, script, flags, cursor, 0);
		remaining--;
	}
	DosClose(handle);
}

void ActionScheduler::reset()
{
	uint8_t i;
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

uint8_t ActionScheduler::first()
{
	return entries->next;
}

uint8_t ActionScheduler::available()
{
	uint8_t current;
	for (current = 1; current < ACTION_SLOTS && entries[current].active(); current++)
		;
	if (current >= ACTION_SLOTS)
		current = 0;
	return current;
}

void ActionScheduler::suppressBarks(uint16_t target)
{
	uint8_t current;
	for (current = first(); current; current = entries[current].next)
		if (entries[current].matches(target))
			entries[current].suppressBarks();
}

void ActionScheduler::link(uint8_t current)
{
	int16_t next;
	uint8_t previous;
	uint8_t found;
	previous = 0;
	do {
		found = 0;
		next = entries[previous].next;
		if (next) {
			if (entries[next].marked(ACTION_STOPPED))
				found = 1;
			/* Times wrap at 16 bits, as DOS int did: a due past 32767 is still in the future. */
			else if ((int16_t)(entries[next].due - ActionQueueTime) <= (int16_t)(entries[current].due - ActionQueueTime))
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

void ActionScheduler::unlink(uint8_t current)
{
	entries[entries[current].previous].next = entries[current].next;
	entries[entries[current].next].previous = entries[current].previous;
}

void ActionScheduler::process()
{
	uint8_t current;
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
	uint8_t current, next;
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

void ActionScheduler::remove(uint16_t target, uint8_t skip, uint8_t mode)
{
	uint8_t current;
	uint8_t flag;
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

void ActionScheduler::replace(uint16_t target, uint16_t replacement)
{
	uint8_t current;
	for (current = first(); current; current = entries[current].next)
		if (entries[current].matches(target))
			entries[current].target.off = replacement;
}

void ActionScheduler::reschedule(uint8_t current)
{
	if (!entries[current].dueNext(ActionQueueTime)) {
		unlink(current);
		link(current);
	}
}

void ActionScheduler::run(uint8_t current)
{
	entries[current].run(1);
	process();
}

void ActionScheduler::tick()
{
	uint8_t current;
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

void ActionScheduler::prepare(uint8_t *prepared)
{
	uint8_t current;
	uint8_t last = 0;
	int16_t unusedRemoved;
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
	/* These items went straight to the screen, which DOS always showed; show them now. */
	WaitForRetrace();
}

void ActionScheduler::advance()
{
	uint8_t current, next, pending, prepared;
	int16_t limit = 100;
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

uint8_t ActionScheduler::add(int16_t delay, uint16_t target, char *script, uint8_t flags,
	uint8_t position, uint8_t initialize)
{
	uint8_t current;
	if (IsNPCHalted(target))
		return 0;
	current = available();
	if (!current)
		return 0;
	if (!entries[current].set(ActionQueueTime + delay, target, flags, (uint8_t *)script, position, initialize))
		return 0;
	link(current);
	count++;
	return current;
}

void ActionScheduler::add(uint16_t target, char *script, uint8_t flags)
{
	add(1, target, script, flags, 1, 1);
}

uint8_t ActionScheduler::add(uint16_t target, char *script)
{
	return add(1, target, script, 0, 1, 1);
}

uint8_t ActionScheduler::add(int16_t delay, uint16_t target, char *script)
{
	return add(delay, target, script, 0, 1, 1);
}

uint8_t ActionScheduler::contains(uint16_t target)
{
	uint8_t current;
	for (current = first(); current; current = entries[current].next)
		if (entries[current].matches(target))
			return 1;
	return 0;
}

void HaltItemScripts(uint16_t target, uint8_t skip)
{
	ActionQueue.remove(target, skip, 0);
}

void RetargetItemScripts(uint16_t target, uint16_t replacement)
{
	ActionQueue.replace(target, replacement);
}

void RunActionQueue()
{
	ActionQueue.advance();
}

void DumpActionQueue()
{
	ActionQueue.dump();
}

int16_t GetActionCount()
{
	return ActionQueue.count;
}

int16_t IsActionQueueRoom()
{
	if (ActionQueue.count < ACTION_SLOTS - 16)
		return 1;
	return 0;
}

extern "C" void ResetActqueueGlobals(void)
{
	memset((void *)&ActionQueue, 0, sizeof(ActionQueue));
	SuspendedActionCount = 0;
	ActionSweepDepth = 0;
}

extern "C" void ConstructActqueueGlobals(void)
{
	new (&ActionQueue) ActionScheduler();
}
