/* Black Gate U7.EXE, resident segment 2 (file offsets 0x0092e5 to 0x00a04c, 3431 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "plat.h"
#include <stdio.h>
#include <string.h>
#include "dosio.h"
#include "easyfile.h"
#include "debug.h"
#include "datanode.h"
#include "actqueue.h"
#include "u7ibuf.h"
#include "gtimer.h"
#include "action.h"

extern uint8_t Item_isFree(ItemId *);
extern uint8_t Item_isRemoved(ItemId *);
extern uint8_t Item_hasNoType(ItemId *);
extern uint8_t Item_stopMissile(ItemId *, int16_t);
extern void Item_releaseNPC(ItemId *);
extern void Item_removeFromPlay(ItemId *);
extern uint8_t Item_haltNPC(ItemId *);
extern void Item_clearScripted(ItemId *);
extern void Item_setScripted(ItemId *);
extern uint8_t Item_isInParty(ItemId *);
extern uint8_t Item_isAvatarNear(ItemId *, int16_t);
extern void Item_moveInDirection(ItemId *, uint8_t);
extern int8_t Item_hatch(ItemId *);
extern void Item_setScriptFrame(ItemId *, int16_t);
extern void Item_stepFrame(ItemId *, int16_t);
extern void Item_stepFrameClamped(ItemId *, int16_t);
extern void Item_walkStep(ItemId *, uint8_t, int8_t);
extern void Item_turnAndAnimate(ItemId *, uint8_t, uint8_t);
extern void Item_face(ItemId *, uint8_t);
extern void Item_playMusic(ItemId *, uint8_t, uint8_t);
extern uint8_t Item_setWeather(ItemId, uint8_t);
extern void Item_playSound(ItemId *, uint8_t);
extern void Item_playSpeech(ItemId *, uint8_t);
extern void Item_runUsecode(ItemId *, uint16_t);
extern void Item_teleport(ItemId *, uint8_t);
extern void Item_bark(ItemId *, char *);
extern void Item_setMappedFrame(ItemId *, int16_t);
extern void Item_hit(ItemId *, uint8_t, uint8_t);
extern void Item_attackTarget(ItemId *);
extern uint8_t Item_updateMissile(ItemId *, int16_t);

Action *ActionTable = 0;
uint8_t *EmptyScript = (uint8_t *)"\002\002";
int16_t ActionQueueTime;

char * DescribeScript(char *script)
{
	char part[6];
	int16_t length;
	char text[128];
	int16_t i;
	length = *script;
	sprintf(text, "%d=", length);
	script++;
	for (i = 1; i < length; i++) {
		if (*script <= ' ' || *script > '~')
			sprintf(part, "#%d", (uint8_t)*script);
		else
			sprintf(part, "%c", *script);
		if (strlen(text) + strlen(part) < 128)
			strcat(text, part);
		script++;
	}
	return StorePath(text);
}

void Action::setTable(Action *table)
{
	ActionTable = table;
}

char *Action::describe()
{
	int8_t kind;
	char text[128];
	kind = ' ';
	if (marked(ACTION_CANCELLED))
		kind = 'L';
	else if (marked(ACTION_STOPPED))
		kind = 'T';
	sprintf(text, "%c%c%2dt [%4d] @%d:%-40.40s", kind,
		due < ActionQueueTime ? ' ' : '+', due - ActionQueueTime,
		(uint16_t)target.off >> 3, cursor, DescribeScript((char *)script));
	return StorePath(text);
}

char *Action::describeName()
{
	int8_t kind;
	char text[128];
	kind = ' ';
	if (marked(ACTION_CANCELLED))
		kind = 'L';
	else if (marked(ACTION_STOPPED))
		kind = 'T';
	sprintf(text, "%c%c%2d\"%-6.6s\"%04u@%d:%-40.40s", kind,
		due < ActionQueueTime ? ' ' : '+', due - ActionQueueTime,
		GetItemName(target.off), (uint16_t)target.off >> 3, cursor,
		DescribeScript((char *)script));
	return StorePath(text);
}

void Action::write(int16_t handle)
{
	DosWrite(handle, -INT32_C(1), INT32_C(2), &target);
	DosWrite(handle, -INT32_C(1), INT32_C(2), &due);
	DosWrite(handle, -INT32_C(1), INT32_C(1), &flags);
	DosWrite(handle, -INT32_C(1), INT32_C(1), &cursor);
	DosWrite(handle, -INT32_C(1), script[0], script);
}

void Action::clear()
{
	previous = next = 0;
	script = 0;
	due = 0;
	flags = 0;
	cursor = 0;
}

/* A script can be dropped by a command it is still running, which the DOS heap tolerated.
 * Scripts dropped during a run are freed only once the outermost run returns. */
static int16_t ExecuteDepth = 0;
#define RETIRED_LIMIT 64
static uint8_t *RetiredScripts[RETIRED_LIMIT];
static int16_t RetiredCount = 0;

static void RetireScript(uint8_t *script)
{
	if (ExecuteDepth > 0 && RetiredCount < RETIRED_LIMIT)
		RetiredScripts[RetiredCount++] = script;
	else
		delete script;
}

static void FreeRetiredScripts()
{
	while (RetiredCount > 0)
		delete RetiredScripts[--RetiredCount];
}

void Action::setScript(uint8_t *commands)
{
	int16_t length;
	if (commands == 0) {
		if (script != 0 && script != EmptyScript)
			RetireScript(script);
		script = 0;
	} else {
		length = *commands;
		if (length > 128)
			script = EmptyScript;
		else {
			script = new uint8_t[length];
			if (script == 0)
				script = EmptyScript;
			else
				memcpy(script, commands, length);
		}
	}
}

void Action::release()
{
	setScript(0);
	clear();
}

void Action::cancel()
{
	if (marked(ACTION_CANCELLED))
		return;
	if (marked(ACTION_FINISH) && !Item_isFree(&target) && !Item_isRemoved(&target)
		&& !Item_hasNoType(&target))
		execute(1);
	flags |= ACTION_CANCELLED;
	SuspendedActionCount--;
	if (marked(ACTION_MISSILE) && Item_stopMissile(&target, script[4]))
		flags |= ACTION_REMOVE_ITEM;
	if (!marked(ACTION_UNHELD))
		Item_releaseNPC(&target);
	if (marked(ACTION_REMOVE_ITEM) && !Item_isFree(&target) && !Item_hasNoType(&target))
		Item_removeFromPlay(&target);
}

void Action::suspend(uint8_t reason)
{
	if (marked(ACTION_CANCELLED) == 0 && !marked(ACTION_STOPPED)) {
		if (reason == ACTION_HALTED && marked(ACTION_REMOVE_ITEM))
			Item_haltNPC(&target);
		flags |= reason;
		SuspendedActionCount++;
		if (!marked(ACTION_FINISH))
			Item_clearScripted(&target);
	}
}

uint8_t Action::setFlag(uint8_t command)
{
	uint8_t handled = 1;
	switch (command) {
	case '-':
		flags |= ACTION_REMOVE_ITEM;
		Item_haltNPC(&target);
		break;
	case '.':
		flags |= ACTION_MISSILE;
		break;
	case ',':
		flags |= ACTION_FINISH;
		break;
	case '#':
		if (cursor > 1 && !marked(ACTION_UNHELD))
			Item_clearScripted(&target);
		flags |= ACTION_UNHELD;
		break;
	default:
		handled = 0;
	}
	return handled;
}

void Action::execute(uint8_t immediate)
{
	ExecuteDepth++;
	executeScript(immediate);
	if (--ExecuteDepth == 0)
		FreeRetiredScripts();
}

void Action::executeScript(uint8_t immediate)
{
	uint8_t completed, wait, endScript;
	uint8_t nextPosition, current, command;
	int16_t limit;
	uint8_t argument, branch;
	uint8_t frameMode, first, second;
	int16_t value;
	char text[80];

	if (Item_hasNoType(&target))
		flags |= ACTION_HALTED;
	else {
		completed = 0;
		wait = 0;
		endScript = 1;
		limit = 300;
		flags |= ACTION_RUNNING;
		due = ActionQueueTime + 1;
		if (finished())
			completed = 1;
		else {
			do {
				wait = 1;
				endScript = 1;
				command = script[cursor];
				current = cursor;
				nextPosition = cursor + 1;
				if (command <= 12) {
					switch (command) {
					case 5:     /* hold another action back a tick */
						argument = script[nextPosition++];
						ActionTable[argument].delay();
						break;
					case 10:    /* loop back to the start */
						if (!immediate)
							nextPosition = 1;
						break;
					case 11:    /* jump by an offset, a counted number of times */
					case 12:
						endScript = 0;
						branch = cursor + script[nextPosition++];
						if ((int8_t)branch < 1 || branch > script[0]) {
							CheatPrintfWait("Too Far in %s", describeName());
							script[nextPosition] = 0;
						} else if (script[nextPosition] > 0) {
							if (script[nextPosition] != 255)
								script[nextPosition] = script[nextPosition] - 1;
							if (cursor == branch)
								nextPosition = ClampInt(0, 1, 127);
							else
								nextPosition = ClampInt(0, (int8_t)branch, 127);
						} else {
							if (command == 12) {
								script[nextPosition] = script[nextPosition + 1];
								nextPosition++;
							}
							nextPosition++;
							if (nextPosition >= script[0])
								endScript = 1;
						}
						break;
					}
					wait = 0;
				} else if (command < '0') {
					switch (command) {
					case '"':
						due = ActionQueueTime;
						wait = 0;
						break;
					case '\'':
						due = ActionQueueTime + script[nextPosition++];
						endScript = 0;
						break;
					case '(':
						due = ActionQueueTime + script[nextPosition++] * 25;
						endScript = 0;
						break;
					case ')':
						due = ActionQueueTime + script[nextPosition++] * 1500;
						endScript = 0;
						break;
					case '+':
						if (!Item_isAvatarNear(&target, script[nextPosition++]) && !immediate)
							nextPosition = current;
						break;
					case '$':
						if (Item_isAvatarNear(&target, script[nextPosition++]) && !immediate)
							nextPosition = current;
						break;
					case '!':
						break;
					default:
						setFlag(command);
						break;
					}
				} else if (command <= '9') {
					Item_moveInDirection(&target, (int16_t)command - '0');
					wait = 0;
				} else if (command <= 'P') {
					switch (command) {
					case 'H':
						if (Item_hatch(&target))
							flags |= ACTION_REMOVE_ITEM;
						break;
					case 'F':
						Item_setScriptFrame(&target, script[nextPosition++]);
						break;
					case 'N':
						Item_stepFrame(&target, 1);
						break;
					case 'P':
						Item_stepFrame(&target, -1);
						break;
					case 'M':
						Item_stepFrameClamped(&target, 1);
						break;
					case 'O':
						Item_stepFrameClamped(&target, -1);
						break;
					}
				} else if (command <= 'Z') {
					switch (command) {
					case 'S':
						argument = (int16_t)script[nextPosition++] - '0';
						branch = script[nextPosition++];
						Item_walkStep(&target, argument, branch);
						break;
					case 'W':
						frameMode = script[nextPosition++];
						Item_turnAndAnimate(&target, frameMode, script[nextPosition++]);
						break;
					case 'Y':
						Item_face(&target, script[nextPosition++] - '0');
						break;
					case 'T':
						first = script[nextPosition++];
						second = script[nextPosition++];
						Item_playMusic(&target, first, second);
						break;
					case 'Z':
						if (!Item_setWeather(target, script[nextPosition++])) {
							if (!immediate)
								nextPosition = current;
							wait = 1;
						}
						break;
					case 'X':
						Item_playSound(&target, script[nextPosition++]);
						break;
					case 'V':
						Item_playSpeech(&target, script[nextPosition++]);
						break;
					case 'U':
						value = script[nextPosition++];
						value += script[nextPosition++] << 8;
						Item_runUsecode(&target, value);
						break;
					case 'Q':
						Item_teleport(&target, script[nextPosition++]);
						break;
					case 'R': {
						char *letter = text;
						nextPosition++;
						/* Stops at the script's end and the buffer's size: a script cut off at 128
						 * bytes loses its closing quote, and DOS read on into whatever followed. */
						while (nextPosition < script[0] && script[nextPosition] != '"'
							&& letter < text + sizeof text - 1)
							*letter++ = script[nextPosition++];
						*letter = 0;
						nextPosition++;
						Item_bark(&target, text);
						break;
					}
					}
				} else if (command <= 'p') {
					if (command >= 'a' && command <= 'p')
						Item_setMappedFrame(&target, command - 'a');
				} else {
					switch (command) {
					case 'x':
						argument = script[nextPosition++];
						Item_hit(&target, argument, script[nextPosition++]);
						break;
					case 'z':
						Item_attackTarget(&target);
						break;
					case 'y': {
						int16_t function = script[nextPosition++];
						if (Item_updateMissile(&target, function))
							completed = 1;
						wait = 1;
						break;
					}
					}
				}
				cursor = nextPosition;
				if (!completed) {
					if (finished())
						completed = endScript;
					else if (wait && script[cursor] == 1) {
						wait = 0;
						cursor++;
					}
					if (immediate)
						wait = 0;
				}
				if (marked(ACTION_CANCELLED))
					wait = 1;
			} while (!wait && !completed && --limit);
		}
		if (!completed && endScript && finished())
			completed = 1;
		if (completed)
			suspend(ACTION_DONE);
	}
	flags &= ~ACTION_RUNNING;
}

void Action::prepare()
{
	int16_t index = 1;
	uint8_t handled;
	do {
		handled = 0;
		if (index < script[0])
			handled = setFlag(script[index++]);
	} while (handled);
}

void Action::set(int16_t tick, uint16_t item, uint8_t options,
	uint8_t *commands, uint8_t position, uint8_t initialize)
{
	if (item < 8)
		return;
	previous = next = 0;
	due = tick;
	target.off = item;
	flags = options;
	cursor = position;
	setScript(commands);
	if (initialize) {
		prepare();
		if (!marked(ACTION_UNHELD))
			Item_setScripted(&target);
	}
}

uint8_t Action::canRun()
{
	return GameTime.running() || Item_isInParty(&target);
}

void Action::run(uint8_t immediate)
{
	if (marked(ACTION_STOPPED) == 0) {
		if (immediate || canRun())
			execute(immediate);
		else
			due = ActionQueueTime + 1;
	}
}

void Action::delay()
{
	due = ActionQueueTime + 1;
}
