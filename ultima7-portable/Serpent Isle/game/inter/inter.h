#ifndef INTER_H
#define INTER_H

struct CallStack;
struct ValueStack;
struct UsecodeRoutine;
struct Value;

/* Usecode opcodes */
#define OP_FOREACH          0x02    /* next element of a list */
#define OP_CONVERSE         0x04    /* ask for an answer */
#define OP_JUMP_IF_FALSE    0x05
#define OP_JUMP             0x06
#define OP_CASE             0x07    /* was the answer one of these strings? */
#define OP_ADD              0x09
#define OP_SUBTRACT         0x0a
#define OP_DIVIDE           0x0b
#define OP_MULTIPLY         0x0c
#define OP_MODULO           0x0d
#define OP_AND              0x0e
#define OP_OR               0x0f
#define OP_NOT              0x10
#define OP_POP_LOCAL        0x12
#define OP_PUSH_TRUE        0x13
#define OP_PUSH_FALSE       0x14
#define OP_GREATER          0x16
#define OP_LESS             0x17
#define OP_GREATER_EQUAL    0x18
#define OP_LESS_EQUAL       0x19
#define OP_NOT_EQUAL        0x1a
#define OP_SAY_TEXT         0x1c    /* add a string of the code's text to what is said */
#define OP_PUSH_TEXT        0x1d
#define OP_MAKE_LIST        0x1e
#define OP_PUSH_INT         0x1f
#define OP_PUSH_LOCAL       0x21
#define OP_EQUAL            0x22
#define OP_CALL             0x24
#define OP_RETURN           0x25
#define OP_PUSH_ELEMENT     0x26
#define OP_RETURN_ALT       0x2c    /* returns as OP_RETURN does */
#define OP_RETURN_VALUE     0x2d
#define OP_FOREACH_START    0x2e
#define OP_SAY_LOCAL        0x2f    /* add a local to what is said */
#define OP_IN               0x30
#define OP_DEFAULT          0x31    /* the conversation's default case */
#define OP_RETURN_ZERO      0x32
#define OP_SAY              0x33
#define OP_CFUNCTION        0x38    /* engine call that returns a value */
#define OP_CROUTINE         0x39    /* engine call that returns none */
#define OP_PUSH_ITEM        0x3e
#define OP_ABORT            0x3f
#define OP_ANSWERED         0x40    /* take the answer, skipping the cases after */
#define OP_PUSH_FLAG        0x42
#define OP_POP_FLAG         0x43
#define OP_PUSH_BYTE        0x44
#define OP_POP_ELEMENT      0x46
#define OP_CALL_ITEM        0x47    /* call for the item on top of the stack */
#define OP_PUSH_EVENT       0x48
#define OP_NULL_ROUTINE     0x49
#define OP_APPEND           0x4a
#define OP_POP_EVENT        0x4b
#define OP_DEBUG_LINE       0x4c    /* source line number, ignored */
#define OP_DEBUG_FUNC       0x4d    /* function number and local names, ignored */

/* why a usable runs */
#define EVENT_BARK          0
#define EVENT_ACTION        1       /* double-clicked */
#define EVENT_POSTED        2
#define EVENT_TRIGGER       3
#define EVENT_COMBAT        4
#define EVENT_EQUIP         5
#define EVENT_UNEQUIP       6

/* why the usecode was started: stored as a byte, pushed as a word */
union Event {
	uint8_t id;
	int16_t word;
};

extern Event CurrentEvent;
extern int16_t OpcodeCount;
void UC_CallIntrinsic(int16_t num, Value *args, Value *ret);

/* the engine calls, indexed by number */
#define ENGINE_CALL_COUNT   197

typedef void ( *EngineCall)(Value *args, Value *ret);

extern const EngineCall EngineCalls[];

void UC_Interpret(UsecodeRoutine *code, ValueStack *stack, CallStack *calls, char *answer, int16_t item, int16_t event);

#endif
