#ifndef UCVALUE_H
#define UCVALUE_H

/* Usecode values: a Value is a list of Nodes, each holding nothing, text, a number or a char. */

#include <stddef.h>
#include "vstring.h"
#include "colbuf.h"

/* what a Node holds */
#define NODE_EMPTY  0
#define NODE_TEXT   1
#define NODE_INT    2
#define NODE_CHAR   3

/* numbers usecode passes in place of an item or a filter */
#define UC_AVATAR   (-356)
#define UC_PARTY    (-357)
#define UC_ALL      (-359)  /* any type, quality or frame */

struct Node : Link {
	uint8_t type;
	String text;
	int16_t number;
	Node() { type = NODE_EMPTY; }
	Node(String &s);
	Node(char *s);
	Node(int32_t n);
	Node(char c);
	Node(Node &n);
	Node(uint32_t address);
	~Node();
	void *operator new(size_t);
	void clear();
	Node &operator=(int32_t n);
	Node &operator=(char c);
	Node &operator=(char *s);
	Node &operator=(String &s);
	Node &operator=(Node &n);
	Node &calculate(uint8_t op, Node &other);
	uint8_t compare(uint8_t op, Node &other);
	int16_t toInt();
	int16_t integer() { return number; }
};

struct Value : List {
	Value();
	~Value() { deleteNodes(); }
	void appendEmpty();
	void appendNode(Node &n);
	void appendString(String &s);
	void appendInt(int16_t n);
	void appendChar(int8_t c);
	void appendFarString(char *s);
	void appendLinearString(uint32_t address);
	void setElement(Value *other, int16_t index);
	Value &appendList(Value *other);
	Value &operator=(Value &v);
	Value &operator=(Node &n);
	Value &operator=(Node &&n) { return *this = n; }
	int16_t contains(Node &n);
	void deleteNodes();
	void clear();
};

int16_t GetItemRef(Node *n);

#endif
