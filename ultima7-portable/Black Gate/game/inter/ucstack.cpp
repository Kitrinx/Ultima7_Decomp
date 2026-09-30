/* Black Gate U7.EXE, overlay segment 321 (file offsets 0x0946e0 to 0x094a5f, 895 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
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

void ValueStack::pushInt(int16_t n)
{
	if (count + 1 >= STACK_SIZE)
		reportOverflow();
	values[count].clear();
	values[count++].appendInt(n);
}

void ValueStack::pushChar(uint8_t c)
{
	if (count + 1 >= STACK_SIZE)
		reportOverflow();
	values[count].clear();
	values[count++].appendChar(c);
}

void ValueStack::pushFarString(char *s)
{
	if (count + 1 >= STACK_SIZE)
		reportOverflow();
	values[count].clear();
	values[count++].appendFarString(s);
}

void ValueStack::pushLinearString(int32_t address)
{
	if (count + 1 >= STACK_SIZE)
		reportOverflow();
	values[count].clear();
	values[count++].appendLinearString(address);
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
int16_t ValueStack::reserve(int16_t n)
{
	int16_t old;
	int16_t i;

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
	/* Borland returned count + n + 1 from the overflow check. */
	return count + 1;
}

/* drop the top n values */
void ValueStack::drop(int16_t n)
{
	int16_t i;

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
	int16_t i;

	for (i = 0; i < count; i++)
		values[i].clear();
	count = 0;
}
