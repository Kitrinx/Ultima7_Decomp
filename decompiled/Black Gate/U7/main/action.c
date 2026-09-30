/* Black Gate U7.EXE, resident segment 2 (file offsets 0x0092e5 to 0x00a04c, 3431 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include <stdio.h>
#include <string.h>
#include <mem.h>
#include "dosio.h"
#include "easyfile.h"
#include "debug.h"
#include "datanode.h"
#include "actqueue.h"
#include "u7ibuf.h"
#include "gtimer.h"
#include "action.h"

extern unsigned char far Item_isFree(ItemId *);
extern unsigned char far Item_isRemoved(ItemId *);
extern unsigned char far Item_hasNoType(ItemId *);
extern unsigned char far Item_stopMissile(ItemId *, int);
extern void far Item_releaseNPC(ItemId *);
extern void far Item_removeFromPlay(ItemId *);
extern unsigned char far Item_haltNPC(ItemId *);
extern void far Item_clearScripted(ItemId *);
extern void far Item_setScripted(ItemId *);
extern unsigned char far Item_isInParty(ItemId *);
extern unsigned char far Item_isAvatarNear(ItemId *, int);
extern void far Item_moveInDirection(ItemId *, unsigned char);
extern char far Item_hatch(ItemId *);
extern void far Item_setScriptFrame(ItemId *, int);
extern void far Item_stepFrame(ItemId *, int);
extern void far Item_stepFrameClamped(ItemId *, int);
extern void far Item_walkStep(ItemId *, unsigned char, char);
extern void far Item_turnAndAnimate(ItemId *, unsigned char, unsigned char);
extern void far Item_face(ItemId *, unsigned char);
extern void far Item_playMusic(ItemId *, unsigned char, unsigned char);
extern unsigned char far Item_setWeather(ItemId, unsigned char);
extern void far Item_playSound(ItemId *, unsigned char);
extern void far Item_playSpeech(ItemId *, unsigned char);
extern void far Item_runUsecode(ItemId *, unsigned);
extern void far Item_teleport(ItemId *, unsigned char);
extern void far Item_bark(ItemId *, char *);
extern void far Item_setMappedFrame(ItemId *, int);
extern void far Item_hit(ItemId *, unsigned char, unsigned char);
extern void far Item_attackTarget(ItemId *);
extern unsigned char far Item_updateMissile(ItemId *, int);

Action *ActionTable = 0;
unsigned char *EmptyScript = (unsigned char *)"\002\002";
int ActionQueueTime;

char *far DescribeScript(char *script)
{
	char part[6];
	int length;
	char text[128];
	int i;
	length = *script;
	sprintf(text, "%d=", length);
	script++;
	for (i = 1; i < length; i++) {
		if (*script <= ' ' || *script > '~')
			sprintf(part, "#%d", (unsigned char)*script);
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
	char kind;
	char text[128];
	kind = ' ';
	if (marked(ACTION_CANCELLED))
		kind = 'L';
	else if (marked(ACTION_STOPPED))
		kind = 'T';
	sprintf(text, "%c%c%2dt [%4d] @%d:%-40.40s", kind,
		due < ActionQueueTime ? ' ' : '+', due - ActionQueueTime,
		(unsigned)target.off >> 3, cursor, DescribeScript((char *)script));
	return StorePath(text);
}

char *Action::describeName()
{
	char kind;
	char text[128];
	kind = ' ';
	if (marked(ACTION_CANCELLED))
		kind = 'L';
	else if (marked(ACTION_STOPPED))
		kind = 'T';
	sprintf(text, "%c%c%2d\"%-6.6s\"%04u@%d:%-40.40s", kind,
		due < ActionQueueTime ? ' ' : '+', due - ActionQueueTime,
		GetItemName(target.off), (unsigned)target.off >> 3, cursor,
		DescribeScript((char *)script));
	return StorePath(text);
}

void Action::write(int handle)
{
	DosWrite(handle, -1L, 2L, &target);
	DosWrite(handle, -1L, 2L, &due);
	DosWrite(handle, -1L, 1L, &flags);
	DosWrite(handle, -1L, 1L, &cursor);
	DosWrite(handle, -1L, script[0], script);
}

void Action::clear()
{
	previous = next = 0;
	script = 0;
	due = 0;
	flags = 0;
	cursor = 0;
}

void Action::setScript(unsigned char *commands)
{
	int length;
	if (commands == 0) {
		if (script != 0 && script != EmptyScript)
			delete script;
		script = 0;
	} else {
		length = *commands;
		if (length > 128)
			script = EmptyScript;
		else {
			script = new unsigned char[length];
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

void Action::suspend(unsigned char reason)
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

unsigned char Action::setFlag(unsigned char command)
{
	unsigned char handled = 1;
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

void Action::execute(unsigned char immediate)
{
	unsigned char completed, wait, endScript;
	unsigned char nextPosition, current, command;
	int limit;
	unsigned char argument, branch;
	unsigned char frameMode, first, second;
	int value;
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
						if ((char)branch < 1 || branch > script[0]) {
							CheatPrintfWait("Too Far in %s", describeName());
							script[nextPosition] = 0;
						} else if (script[nextPosition] > 0) {
							if (script[nextPosition] != 255)
								script[nextPosition] = script[nextPosition] - 1;
							if (cursor == branch)
								nextPosition = ClampInt(0, 1, 127);
							else
								nextPosition = ClampInt(0, (char)branch, 127);
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
					Item_moveInDirection(&target, (int)command - '0');
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
						argument = (int)script[nextPosition++] - '0';
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
						while (script[nextPosition] != '"')
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
						int function = script[nextPosition++];
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
	int index = 1;
	unsigned char handled;
	do {
		handled = 0;
		if (index < script[0])
			handled = setFlag(script[index++]);
	} while (handled);
}

void Action::set(int tick, unsigned item, unsigned char options,
	unsigned char *commands, unsigned char position, unsigned char initialize)
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

unsigned char Action::canRun()
{
	return GameTime.running() || Item_isInParty(&target);
}

void Action::run(unsigned char immediate)
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
