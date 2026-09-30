/* Black Gate U7.EXE, overlay segment 324 (file offsets 0x097250 to 0x097fb7, 3431 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: ..\inter\inter.c */
#include "u7port.h"
#include <string.h>
#include "dosio.h"
#include "objref.h"
#include "colbuf.h"
#include "keywords.h"
#include "ucstack.h"
#include "routine.h"
#include "farstr.h"
#include "convmgr.h"
#include "uclist.h"
#include "vstring.h"
#include "gumps.h"
#include "init.h"
#include "debug.h"
#include "flags.h"
#include "ucctrl.h"
#include "inter.h"

Event CurrentEvent;
int16_t OpcodeCount = 0;

/* the next byte or word of the running function's code */
#define NEXT_BYTE() code->readByte()
#define NEXT_WORD() code->readWord()

/* local n of the running function; its arguments come first */
#define LOCAL(n)    stack->values[base + (n)]

/* is s the answer picked? */
inline int8_t IsAnswer(char *answer, char *s)
{
	return !_fstrcmp(answer, s);
}

/* the first node of a value, and the int in it */
#define FIRST(v)    GetListNode(v, 1)
#define INT(v)      FIRST(v)->toInt()

#define POP(v)      stack->pop(&(v))
#define PUSH(v)     stack->push(v)
#define PUSH_INT(n) stack->pushInt(n)

/* a caller's state goes on the call stack */
#define SAVE(n)     CallStack_push(calls, n)
#define RESTORE()   CallStack_pop(calls)

/* Run engine call num on the arguments below args, leaving its result in ret. The check lets
 * num equal the count through. */
void UC_CallIntrinsic(int16_t num, Value *args, Value *ret)
{
	if (num > ENGINE_CALL_COUNT || num < 0)
		HaltWithMessage(__FILE__, 109, "%d", num);
	(*EngineCalls[num])(args, ret);
}

/* Run the usecode function loaded into code, and every function it calls. */
void UC_Interpret(UsecodeRoutine *code, ValueStack *stack, CallStack *calls, char *answer, int16_t item,
	int16_t event)
{
	uint8_t opcode;

	ConversationShown = 0;
	Value a, b;
	int16_t n1, n2, n3, n4;
	int16_t base, nlocals, func, start, nexterns, externs, text, var, i, count, size, nargs;
	int16_t found, answered, argc, id;

	CurrentEvent.id = event;
	SAVE(item);
	SAVE(code->position);
	SAVE(text = code->position);
	size = NEXT_WORD();
	code->skip(size);
	code->skip(2);          /* a usable takes no arguments */
	SAVE(0);
	SAVE(nlocals = NEXT_WORD());
	SAVE(0);
	stack->reserve(nlocals);
	nexterns = NEXT_WORD();
	SAVE(externs = code->position);
	code->position += nexterns * 2;
	base = 0;
	argc = 0;
	OpcodeCount = 0;
	while (calls->count != 0) {
		do {
			OpcodeCount++;
			opcode = NEXT_BYTE();
			switch (opcode) {
			case OP_NULL_ROUTINE:
				DebugPrintf("Null routine detected %d", id);
				goto abort;
			case OP_PUSH_EVENT:
				PUSH_INT(CurrentEvent.word);
				break;
			case OP_PUSH_ITEM:
				PUSH_INT(item);
				break;
			case OP_SAY_TEXT:
			case OP_SAY_LOCAL:
				/* append text and values until the say */
				{
					FarString s;

					while (opcode != OP_SAY) {
						if (opcode == OP_SAY_TEXT)
							s.appendMemory((uint32_t)code->text(text, NEXT_WORD()));
						else
							s.append(&LOCAL(NEXT_WORD()));
						opcode = NEXT_BYTE();
					}
					s.show();
				}
				break;
			case OP_PUSH_TEXT:
				stack->pushLinearString(code->text(text, NEXT_WORD()));
				break;
			case OP_PUSH_INT:
				PUSH_INT(NEXT_WORD());
				break;
			case OP_PUSH_BYTE:
				stack->pushChar(NEXT_BYTE());
				break;
			case OP_PUSH_LOCAL:
				PUSH(&LOCAL(NEXT_WORD()));
				break;
			case OP_PUSH_FLAG:
				PUSH_INT(GameFlags.get(NEXT_WORD()));
				break;
			case OP_ADD:  /* arithmetic works on the first nodes */
			case OP_SUBTRACT:
			case OP_DIVIDE:
			case OP_MULTIPLY:
			case OP_MODULO:
			case OP_APPEND:
				POP(a);
				POP(b);
				if (opcode == OP_APPEND) {
					PUSH(&b.appendList(&a));
					break;
				}
				stack->pushNode(&FIRST(&b)->calculate(opcode, *FIRST(&a)));
				break;
			case OP_GREATER:
			case OP_LESS:
			case OP_GREATER_EQUAL:
			case OP_LESS_EQUAL:
			case OP_NOT_EQUAL:
			case OP_EQUAL:
				POP(a);
				POP(b);
				PUSH_INT(FIRST(&b)->compare(opcode, *FIRST(&a)));
				break;
			case OP_IN:
				POP(a);
				POP(b);
				if (a.contains(*FIRST(&b)))
					PUSH_INT(1);
				else
					PUSH_INT(0);
				break;
			case OP_AND:
				POP(a);
				POP(b);
				PUSH_INT(INT(&a) && INT(&b));
				break;
			case OP_OR:
				POP(a);
				POP(b);
				PUSH_INT(INT(&a) || INT(&b));
				break;
			case OP_NOT:
				POP(a);
				PUSH_INT(!INT(&a));
				break;
			case OP_PUSH_TRUE:
				PUSH_INT(1);
				break;
			case OP_PUSH_FALSE:
				PUSH_INT(0);
				break;
			case OP_DEFAULT:
				/* taken when no case matched */
				code->skip(2);
				n1 = NEXT_WORD();
				if (answered == 0)
					answered = 1;
				else
					code->skip(n1);
				break;
			case OP_CASE:
				/* does the answer picked match any of n1 strings? */
				found = 0;
				n1 = NEXT_WORD();
				n2 = NEXT_WORD();
				for (n3 = 0; n3 < n1; n3++) {
					POP(a);
					if (answered == 0 && IsAnswer(answer, FIRST(&a)->text.str))
						found = answered = 1;
				}
				if (found == 0)
					code->skip(n2);
				break;
			case OP_MAKE_LIST:
				/* build a list from the top n1 values */
				a.deleteNodes();
				n1 = NEXT_WORD();
				for (n2 = 0; n2 < n1; n2++) {
					POP(b);
					count = LinkList_count(&b);
					for (i = 1; i <= count; i++)
						a.appendNode(*GetListNode(&b, i));
				}
				PUSH(&a);
				break;
			case OP_PUSH_ELEMENT:  /* one-based */
				POP(a);
				n1 = NEXT_WORD();
				stack->pushNode(GetListNode(&LOCAL(n1), INT(&a)));
				break;
			case OP_JUMP_IF_FALSE:
				POP(a);
				if (INT(&a) == 0)
					code->skip(NEXT_WORD());
				else
					NEXT_WORD();
				break;
			case OP_FOREACH_START:
				/* counter 1 and the list's length, then back up to run the OP_FOREACH */
				NEXT_BYTE();
				n1 = NEXT_WORD();
				n2 = NEXT_WORD();
				LOCAL(n1) = Node(1);
				NEXT_WORD();
				LOCAL(n2) = Node((int16_t)LinkList_count(&LOCAL(NEXT_WORD())));
				code->skip(-9);
				break;
			case OP_FOREACH:
				/* counter n1, length n2, element n3 of list n4 */
				n1 = NEXT_WORD();
				n2 = NEXT_WORD();
				n3 = NEXT_WORD();
				n4 = NEXT_WORD();
				if (FIRST(&LOCAL(n1))->number > FIRST(&LOCAL(n2))->number) {
					code->skip(NEXT_WORD());
					break;
				}
				LOCAL(n3) = *GetListNode(&LOCAL(n4), FIRST(&LOCAL(n1))->number);
				LOCAL(n1) = Node(FIRST(&LOCAL(n1))->number + 1);
				code->skip(2);
				break;
			case OP_ANSWERED:
				answered = 1;
				break;
			case OP_CONVERSE:
				n1 = NEXT_WORD();
				if (OfferedAnswers.head == 0) {
					DebugPrintfWait("Popped Last Keyword: BUG!!!");
					goto abort;
				}
				RunOptionsLoop();
				answered = 0;
				break;
			case OP_POP_LOCAL:
				POP(a);
				var = NEXT_WORD();
				LOCAL(var) = a;
				break;
			case OP_POP_EVENT:
				POP(a);
				CurrentEvent.id = INT(&a);
				break;
			case OP_POP_ELEMENT:
				POP(a);
				POP(b);
				var = NEXT_WORD();
				LOCAL(var).setElement(&b, INT(&a));
				break;
			case OP_POP_FLAG:
				POP(a);
				GameFlags.set(NEXT_WORD(), INT(&a));
				break;
			case OP_JUMP:
				n1 = NEXT_WORD();
				code->skip(n1);
				break;
			case OP_CFUNCTION:
			case OP_CROUTINE:
				/* engine call n1 on n2 arguments */
				n1 = NEXT_WORD();
				n2 = NEXT_BYTE();
				a.clear();
				UC_CallIntrinsic(n1, &stack->values[stack->count], &a);
				stack->drop(n2);
				if (opcode == OP_CFUNCTION)
					PUSH(&a);
				break;
			case OP_CALL:
			case OP_CALL_ITEM:
				func = NEXT_WORD();
				id = LinkedRoutines[func].id;
				SAVE(item);
				SAVE(code->position);
				SAVE(text);
				if (opcode == OP_CALL_ITEM) {
					n1 = 1;
					item = INT(&stack->values[stack->count - 1]);
				} else
					n1 = 0;
				text = start = code->resolve(func, externs, n1);
				code->position = start;
				code->skip(NEXT_WORD());
				SAVE(base);
				size = stack->count;
				nargs = NEXT_WORD();
				base = size - nargs;
				SAVE(nlocals);
				SAVE(argc);
				argc = nargs;
				nlocals = NEXT_WORD();
				stack->reserve(nlocals);
				SAVE(externs);
				nexterns = NEXT_WORD();
				externs = code->position;
				code->position += nexterns * 2;
				break;
			case OP_RETURN:
			case OP_RETURN_ALT:
			case OP_RETURN_VALUE:
			case OP_RETURN_ZERO:
				/* returning from the usable ends the run */
				a.deleteNodes();
				if (opcode == OP_RETURN_VALUE)
					POP(a);
				externs = RESTORE();
				stack->reserve(-nlocals);
				stack->drop(argc);
				argc = RESTORE();
				nlocals = RESTORE();
				base = RESTORE();
				text = RESTORE();
				if (opcode == OP_RETURN_VALUE)
					PUSH(&a);
				code->position = RESTORE();
				item = RESTORE();
				if (opcode == OP_RETURN_ZERO)
					PUSH_INT(0);
				if (calls->count == 0)
					opcode = OP_RETURN;
				break;
			case OP_ABORT:
				/* abandon every function back to the usable */
				goto abort;
			default:
				ReportError(0x6000);    /* bad opcode */
				break;
			}
		} while (opcode != OP_RETURN);
	}
abort:
	stack->clear();
	calls->count = 0;
	TempString.clear();
	ResultNode.clear();
	ScratchValue.clear();
	if (ConversationShown)
		EndConversation();
	if (ConversationShown)
		OfferedAnswers.clear();
}

extern "C" void ResetInterGlobals(void)
{
	memset(&CurrentEvent, 0, sizeof(CurrentEvent));
	OpcodeCount = 0;
}
