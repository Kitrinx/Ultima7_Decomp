/* Black Gate U7.EXE, overlay segment 322 (file offsets 0x094aa0 to 0x09668b, 7147 bytes).
 * Borland C++ 2.0 -mm -O -P -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "dosio.h"
#include "lowlevel.h"
#include "oops.h"
#include "ucvalue.h"
#include "uclist.h"
#include "inter.h"

Node ResultNode;
Value ScratchValue;

/* s as a whole number, allowing a sign and surrounding spaces; 0 when it is not one */
int8_t ParseWholeNumber(int16_t *result, char *s)
{
	int16_t i = strlen(s) - 1;
	int16_t value = 0;
	int16_t scale = 1;
	int8_t c;

	while (isspace(s[i]) && i != -1)
		i--;
	if (i == -1)
		return 0;
	while (isdigit(c = s[i]) && i != -1) {
		value += (c - '0') * scale;
		scale *= 10;
		i--;
	}
	if (s[i] == '-' && i != -1) {
		value = -value;
		i--;
	}
	while (isspace(s[i]) && i != -1)
		i--;
	if (i != -1)
		return 0;
	*result = value;
	return 1;
}

Node::Node(String &s)
{
	type = NODE_TEXT;
	text = s;
}

Node::Node(char *s)
{
	type = NODE_TEXT;
	text = s;
}

Node::Node(int32_t n)
{
	type = NODE_INT;
	number = n;
}

Node::Node(char c)
{
	type = NODE_CHAR;
	number = c;
}

Node::Node(Node &n)
{
	type = n.type;
	if (type == NODE_INT || type == NODE_CHAR)
		number = n.number;
	else if (type == NODE_TEXT)
		text = n.text;
	next = 0;
}

/* text copied from a linear address, by way of WorkString */
Node::Node(uint32_t address)
{
	type = NODE_TEXT;
	CopyLinearStringOut(WorkString, address, -1);
	text = WorkString;
}

void *Node::operator new(size_t)
{
	Node *p = ::new Node;

	if (p == 0)
		ReportOutOfNearMemory();
	return p;
}

void Node::clear()
{
	if (type == NODE_TEXT)
		text.clear();
	type = NODE_EMPTY;
}

Node &Node::operator=(int32_t n)
{
	clear();
	type = NODE_INT;
	number = n;
	return *this;
}

Node &Node::operator=(char c)
{
	clear();
	type = NODE_CHAR;
	number = c;
	return *this;
}

Node &Node::operator=(char *s)
{
	clear();
	type = NODE_TEXT;
	text = s;
	return *this;
}

Node &Node::operator=(String &s)
{
	clear();
	type = NODE_TEXT;
	text = s;
	return *this;
}

Node &Node::operator=(Node &n)
{
	if (type == NODE_TEXT)
		text.clear();
	type = n.type;
	if (type == NODE_INT || type == NODE_CHAR)
		number = n.number;
	else if (type == NODE_TEXT)
		text = n.text;
	return *this;
}

/* this op other, for the arithmetic opcodes; the answer is left in ResultNode */
Node &Node::calculate(uint8_t op, Node &other)
{
	String s;
	int16_t n = other.number;
	int8_t otherType;
	char buf[7];
	int16_t value;

	s = other.text.str;
	otherType = other.type;
	if (otherType == NODE_INT || otherType == NODE_CHAR) {
		switch (op) {
		case OP_ADD:
			if (type == NODE_INT || type == NODE_CHAR)
				return ResultNode = number + n;
			else if (type == NODE_TEXT) {
				itoa(n, buf, 10);
				ResultNode = text + buf;
				return ResultNode;
			} else
				return ResultNode = other;
		case OP_SUBTRACT:
			if (type == NODE_INT || type == NODE_CHAR)
				return ResultNode = number - n;
			else if (type == NODE_TEXT)
				return ResultNode = text;
			else
				return ResultNode = -n;
		case OP_MULTIPLY:
			if (type == NODE_INT || type == NODE_CHAR)
				return ResultNode = number * n;
			else if (type == NODE_TEXT)
				return ResultNode = text;
			else
				return ResultNode = 0;
		case OP_DIVIDE:
			if (type == NODE_INT || type == NODE_CHAR) {
				if (n == 0) {
					return ResultNode = 0x7fff;
				} else {
					return ResultNode = number / n;
				}
			} else if (type == NODE_TEXT)
				return ResultNode = text;
			else
				return ResultNode = 0;
		case OP_MODULO:
			if (type == NODE_INT || type == NODE_CHAR) {
				if (n == 0) {
					return ResultNode = 0x7fff;
				} else {
					return ResultNode = number % n;
				}
			} else if (type == NODE_TEXT)
				return ResultNode = text;
			else
				return ResultNode = 0;
		}
	}
	if (otherType == NODE_TEXT) {
		switch (op) {
		case OP_ADD:
			if (type == NODE_INT || type == NODE_CHAR) {
				if (ParseWholeNumber(&value, s.str)) {
					ResultNode = number + value;
					return ResultNode;
				} else
					return ResultNode = number;
			} else if (type == NODE_TEXT)
				return ResultNode = text + s;
			else
				return ResultNode = s;
		case OP_SUBTRACT:
			if (type == NODE_INT || type == NODE_CHAR) {
				if (ParseWholeNumber(&value, s.str))
					return ResultNode = number - value;
				else
					return ResultNode = number;
			} else if (type == NODE_TEXT)
				return ResultNode = text;
			else {
				ResultNode.clear();
				return ResultNode;
			}
		case OP_MULTIPLY:
			if (type == NODE_INT || type == NODE_CHAR) {
				if (ParseWholeNumber(&value, s.str))
					return ResultNode = number * value;
				else
					return ResultNode = number;
			} else if (type == NODE_TEXT)
				return ResultNode = text;
			else {
				ResultNode.clear();
				return ResultNode;
			}
		case OP_DIVIDE:
			if (type == NODE_INT || type == NODE_CHAR) {
				if (ParseWholeNumber(&value, s.str)) {
					if (value == 0) {
						return ResultNode = 0x7fff;
					} else {
						return ResultNode = number / value;
					}
				} else
					return ResultNode = number;
			} else if (type == NODE_TEXT)
				return ResultNode = text;
			else {
				ResultNode.clear();
				return ResultNode;
			}
		case OP_MODULO:
			if (type == NODE_INT || type == NODE_CHAR) {
				if (ParseWholeNumber(&value, s.str)) {
					/* tests n, but divides by value */
					if (n == 0) {
						return ResultNode = 0x7fff;
					} else {
						return ResultNode = number % value;
					}
				} else
					return ResultNode = number;
			} else if (type == NODE_TEXT)
				return ResultNode = text;
			else {
				ResultNode.clear();
				return ResultNode;
			}
		}
	} else if (type == NODE_EMPTY) {
		ResultNode.clear();
		return ResultNode;
	} else {
		ResultNode = *this;
		return ResultNode;
	}
	/* only an op the interpreter never passes gets here */
	return ResultNode;
}

/* this op other, for the comparison opcodes */
uint8_t Node::compare(uint8_t op, Node &other)
{
	String s;
	int16_t n = other.number;
	int8_t otherType;
	char buf[7];
	int16_t value;

	s = other.text.str;
	otherType = other.type;
	if (otherType == NODE_INT || otherType == NODE_CHAR) {
		switch (op) {
		case OP_EQUAL:
			if (type == NODE_INT || type == NODE_CHAR)
				return number == n;
			else if (type == NODE_TEXT) {
				itoa(n, buf, 10);
				return !_fstrcmp(buf, text.str);
			} else
				return n == 0;
		case OP_GREATER:
			if (type == NODE_INT || type == NODE_CHAR)
				return number > n;
			else if (type == NODE_TEXT) {
				itoa(n, buf, 10);
				return _fstrcmp(text.str, buf) > 0;
			} else
				return n < 0;
		case OP_LESS:
			if (type == NODE_INT || type == NODE_CHAR)
				return number < n;
			else if (type == NODE_TEXT) {
				itoa(n, buf, 10);
				return _fstrcmp(text.str, buf) < 0;
			} else
				return n > 0;
		case OP_GREATER_EQUAL:
			if (type == NODE_INT || type == NODE_CHAR)
				return number >= n;
			else if (type == NODE_TEXT) {
				itoa(n, buf, 10);
				return _fstrcmp(text.str, buf) >= 0;
			} else
				return n <= 0;
		case OP_LESS_EQUAL:
			if (type == NODE_INT || type == NODE_CHAR)
				return number <= n;
			else if (type == NODE_TEXT) {
				itoa(n, buf, 10);
				return _fstrcmp(text.str, buf) <= 0;
			} else
				return n >= 0;
		case OP_NOT_EQUAL:
			if (type == NODE_INT || type == NODE_CHAR)
				return number != n;
			else if (type == NODE_TEXT) {
				itoa(n, buf, 10);
				return _fstrcmp(buf, text.str) != 0;
			} else
				return n != 0;
		}
	}
	if (otherType == NODE_TEXT) {
		switch (op) {
		case OP_EQUAL:
			if (type == NODE_INT || type == NODE_CHAR) {
				if (ParseWholeNumber(&value, s.str)) {
					return number == value;
				} else {
					return 0;
				}
			} else if (type == NODE_TEXT)
				return !_fstrcmp(text.str, s.str);
			else
				return s.isEmpty();
		case OP_GREATER:
			if (type == NODE_INT || type == NODE_CHAR) {
				if (ParseWholeNumber(&value, s.str)) {
					return number > value;
				} else {
					return 0;
				}
			} else if (type == NODE_TEXT)
				return _fstrcmp(text.str, s.str) > 0;
			else
				return 0;
		case OP_LESS:
			if (type == NODE_INT || type == NODE_CHAR) {
				if (ParseWholeNumber(&value, s.str)) {
					return number < value;
				} else {
					return 0;
				}
			} else if (type == NODE_TEXT)
				return _fstrcmp(text.str, s.str) < 0;
			else
				return !s.isEmpty();
		case OP_GREATER_EQUAL:
			if (type == NODE_INT || type == NODE_CHAR) {
				if (ParseWholeNumber(&value, s.str)) {
					return number >= value;
				} else {
					return 0;
				}
			} else if (type == NODE_TEXT)
				return _fstrcmp(text.str, s.str) >= 0;
			else
				return s.isEmpty();
		case OP_LESS_EQUAL:
			if (type == NODE_INT || type == NODE_CHAR) {
				if (ParseWholeNumber(&value, s.str)) {
					return number <= value;
				} else {
					return 0;
				}
			} else if (type == NODE_TEXT)
				return _fstrcmp(text.str, s.str) <= 0;
			else
				return 1;
		case OP_NOT_EQUAL:
			if (type == NODE_INT || type == NODE_CHAR) {
				if (ParseWholeNumber(&value, s.str)) {
					return number != value;
				} else {
					return 1;
				}
			} else if (type == NODE_TEXT)
				return _fstrcmp(s.str, text.str) == 0 ? 0 : 1;
			else
				return !s.isEmpty();
		}
	} else {
		switch (op) {
		case OP_EQUAL:
			if (type == NODE_INT || type == NODE_CHAR) {
				return number == 0;
			} else if (type == NODE_TEXT) {
				return text.isEmpty();
			} else
				return 1;
		case OP_GREATER:
			if (type == NODE_INT || type == NODE_CHAR) {
				return number > 0;
			} else if (type == NODE_TEXT) {
				return !text.isEmpty();
			} else
				return 0;
		case OP_LESS:
			if (type == NODE_INT || type == NODE_CHAR) {
				return number < 0;
			} else
				return 0;
		case OP_GREATER_EQUAL:
			if (type == NODE_INT || type == NODE_CHAR) {
				return number >= 0;
			} else
				return 1;
		case OP_LESS_EQUAL:
			if (type == NODE_INT || type == NODE_CHAR) {
				return number <= 0;
			} else if (type == NODE_TEXT) {
				return text.isEmpty();
			} else
				return 1;
		case OP_NOT_EQUAL:
			if (type == NODE_INT || type == NODE_CHAR) {
				return number != 0;
			} else if (type == NODE_TEXT) {
				return !text.isEmpty();
			} else
				return 0;
		}
	}
	/* only an op the interpreter never passes gets here */
	return 0;
}

int16_t Node::toInt()
{
	int16_t n;

	switch (type) {
	case NODE_INT:
	case NODE_CHAR:
		return number;
	case NODE_TEXT:
		if (ParseWholeNumber(&n, text.str))
			return n;
		return 0;
	case NODE_EMPTY:
		return 0;
	}
	/* Borland returned the type the switch tested. */
	return type;
}

Value::Value()
{
	head = tail = 0;
}

void Value::appendEmpty()
{
	Node *n = new Node;

	LinkList_append(this, n);
}

void Value::appendNode(Node &n)
{
	Node *p = new Node(n);

	LinkList_append(this, p);
}

void Value::appendString(String &s)
{
	Node *p = new Node(s);

	LinkList_append(this, p);
}

void Value::appendInt(int16_t n)
{
	Node *p = new Node(n);

	LinkList_append(this, p);
}

void Value::appendChar(int8_t c)
{
	Node *p = new Node(c);

	LinkList_append(this, p);
}

void Value::appendFarString(char *s)
{
	Node *p = new Node(s);

	LinkList_append(this, p);
}

void Value::appendLinearString(uint32_t address)
{
	Node *p = new Node(address);

	LinkList_append(this, p);
}

/* put other's nodes in place of node index, padding the list out to it first */
void Value::setElement(Value *other, int16_t index)
{
	Link *cur = 0;
	Link *n;
	int16_t i;

	for (i = 1; i <= index; i++) {
		LinkList_stepForward(this, &cur);
		if (cur == 0) {
			for (i = index - i + 1; i; i--)
				appendEmpty();
			cur = tail;
			break;
		}
	}
	*(Node *) cur = *GetListNode(other, 1);
	/* other's first node is in place; copy the rest after it */
	n = 0;
	LinkList_stepForward(other, &n);
	while (LinkList_stepForward(other, &n)) {
		Node *copy = new Node(*(Node *) n);
		LinkList_insertChainAfter(this, cur, copy);
		cur = copy;
	}
}

Value &Value::appendList(Value *other)
{
	Link *n = 0;

	while (LinkList_stepForward(other, &n))
		appendNode(*(Node *) n);
	return *this;
}

/* node index, counting from 1; an emptied ResultNode when there is none */
Node *GetListNode(Value *value, int16_t index)
{
	Link *n;
	int16_t i;

	if (index < 1 || value->head == 0) {
		ResultNode.clear();
		return &ResultNode;
	}
	n = value->head;
	for (i = 1; i < index; i++) {
		n = n->next;
		if (n == 0) {
			ResultNode.clear();
			return &ResultNode;
		}
	}
	return (Node *) n;
}

Value &Value::operator=(Value &v)
{
	Link *n;

	deleteNodes();
	for (n = v.head; n; n = n->next)
		appendNode(*(Node *) n);
	return *this;
}

Value &Value::operator=(Node &n)
{
	deleteNodes();
	appendNode(n);
	return *this;
}

/* 1 when some node equals n */
int16_t Value::contains(Node &n)
{
	Link *p;

	for (p = head; p; p = p->next)
		if (((Node *) p)->compare(OP_EQUAL, n))
			return 1;
	return 0;
}

void Value::deleteNodes()
{
	Link *n = head;
	Link *next;

	while (n) {
		next = n->next;
		delete n;
		n = next;
	}
	head = tail = 0;
}

void Value::clear()
{
	deleteNodes();
	head = tail = 0;
}
