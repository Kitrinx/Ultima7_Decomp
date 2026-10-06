/* Serpent Isle SI.EXE, overlay segment 300 (file offsets 0x081250 to 0x0815cf, 895 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "dosio.h"
#include "ucstack.h"

/* push a copy of v */
void ValueStack::push(Value *v)
{
	if (count + 1 >= STACK_SIZE)
		reportOverflow();
	values[count].clear();
	values[count++] = *v;
}

void ValueStack::pushNode(Node *n)
{
	if (count + 1 >= STACK_SIZE)
		reportOverflow();
	values[count].clear();
	values[count++].appendNode(*n);
}

void ValueStack::pushString(String &s)
{
	if (count + 1 >= STACK_SIZE)
		reportOverflow();
	values[count].clear();
	values[count++].appendString(s);
}

void ValueStack::pushInt(int n)
{
	if (count + 1 >= STACK_SIZE)
		reportOverflow();
	values[count].clear();
	values[count++].appendInt(n);
}

void ValueStack::pushChar(unsigned char c)
{
	if (count + 1 >= STACK_SIZE)
		reportOverflow();
	values[count].clear();
	values[count++].appendChar(c);
}

void ValueStack::pushFarString(char far *s)
{
	if (count + 1 >= STACK_SIZE)
		reportOverflow();
	values[count].clear();
	values[count++].appendFarString(s);
}

void ValueStack::pushLinearString(char far *s)
{
	if (count + 1 >= STACK_SIZE)
		reportOverflow();
	values[count].clear();
	values[count++].appendLinearString((unsigned long)s);
}

/* pop the top value into v */
void ValueStack::pop(Value *v)
{
	if (count - 1 < 0)
		ReportError(0x6600);    /* underflow */
	count--;
	*v = values[count];
	values[count].clear();
}

void ValueStack::reportOverflow()
{
	ReportError(0x6601);
}

/* grow or shrink the stack by n empty values and return the old count; no count when n is 0 */
int ValueStack::reserve(int n)
{
	int old;
	int i;

	if (count + n + 1 > STACK_SIZE || count + n + 1 <= 0)
		reportOverflow();
	if (n > 0) {
		old = count;
		count += n;
		for (i = old; i < count; i++)
			values[i].clear();
		return old;
	} else if (n < 0) {
		old = count;
		count += n;
		for (i = count; i < old; i++)
			values[i].clear();
		return old;
	}
}

/* drop the top n values */
void ValueStack::drop(int n)
{
	int i;

	if (count - n < 0)
		ReportError(0x6600);    /* underflow */
	for (i = 0; i < n; i++) {
		count--;
		values[count].clear();
	}
}

/* empty the stack */
void ValueStack::clear()
{
	int i;

	for (i = 0; i < count; i++)
		values[i].clear();
	count = 0;
}
