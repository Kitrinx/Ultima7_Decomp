#ifndef UCSTACK_H
#define UCSTACK_H

#include "ucvalue.h"

#define STACK_SIZE  70

/* the interpreter's values: every function's arguments and locals, then its work */
struct ValueStack {
	Value values[STACK_SIZE];
	int16_t count;
	ValueStack() { count = 0; }
	void push(Value *);
	void pushNode(Node *);
	void pushString(String &);
	void pushInt(int16_t);
	void pushChar(uint8_t);
	void pushFarString(char *);
	void pushLinearString(int32_t address);
	void pop(Value *);
	void reportOverflow();
	int16_t reserve(int16_t);
	void drop(int16_t);
	void clear();
};

#endif
