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
	unsigned char type;
	String text;
	int number;
	Node() { type = NODE_EMPTY; }
	Node(String &s);
	Node(char far *s);
	Node(int n);
	Node(char c);
	Node(Node &n);
	Node(unsigned long address);
	~Node();
	void *operator new(size_t);
	void clear();
	Node &operator=(int n);
	Node &operator=(char c);
	Node &operator=(char *s);
	Node &operator=(String &s);
	Node &operator=(Node &n);
	Node &calculate(unsigned char op, Node &other);
	unsigned char compare(unsigned char op, Node &other);
	int toInt();
	int integer() { return number; }
};

struct Value : List {
	Value();
	~Value() { deleteNodes(); }
	void appendEmpty();
	void appendNode(Node &n);
	void appendString(String &s);
	void appendInt(int n);
	void appendChar(char c);
	void appendFarString(char far *s);
	void appendLinearString(unsigned long address);
	void setElement(Value *other, int index);
	Value &appendList(Value *other);
	Value &operator=(Value &v);
	Value &operator=(Node &n);
	int contains(Node &n);
	void deleteNodes();
	void clear();
};

int GetItemRef(Node *n);

#endif
