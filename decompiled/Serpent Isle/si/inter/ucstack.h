#ifndef UCSTACK_H
#define UCSTACK_H

#include "ucvalue.h"

#define STACK_SIZE  80

/* the interpreter's values: every function's arguments and locals, then its work */
struct ValueStack {
	Value values[STACK_SIZE];
	int count;
	ValueStack() { count = 0; }
	void push(Value *);
	void pushNode(Node *);
	void pushString(String &);
	void pushInt(int);
	void pushChar(unsigned char);
	void pushFarString(char far *);
	void pushLinearString(char far *);
	void pop(Value *);
	void reportOverflow();
	int reserve(int);
	void drop(int);
	void clear();
};

#endif
