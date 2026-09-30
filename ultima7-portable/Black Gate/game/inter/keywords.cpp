/* Black Gate U7.EXE, overlay segment 241 (file offsets 0x06f3d0 to 0x06f7d2, 1026 bytes).
 * Borland C++ 2.0 -mm -O -P -Y rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
#include <new>
#include <string.h>
#include "dosio.h"
#include "oops.h"
#include "keywords.h"

int32_t KeywordChecksum = 0;       /* the answers' character sum when last compared */
AnswerList OfferedAnswers;
char ChosenAnswer[ANSWER_SIZE];

Answer::Answer(char *s)
{
	int16_t size;

	if (strlen(s) >= ANSWER_SIZE - 1)
		size = ANSWER_SIZE;
	else
		size = strlen(s) + 1;
	text = new char[size];
	if (text == 0)
		ReportOutOfNearMemory();
	strncpy(text, s, size);
	text[size - 1] = 0;
}

void *Answer::operator new(size_t)
{
	Answer *p = ::new Answer;

	if (p == 0)
		ReportOutOfNearMemory();
	return p;
}

Answer::~Answer()
{
	if (text)
		delete text;
}

void AnswerList::append(char *s)
{
	Answer *a = new Answer(s);

	LinkList_append(this, a);
	count++;
	if (mark == 0)
		mark = a;
}

/* Offer s, unless it is on offer already. */
void AnswerList::add(char *s)
{
	Answer *a = 0;

	while (next(&a))
		if (strcmp(a->text, s) == 0)
			return;
	append(s);
}

void AnswerList::push()
{
	if (depth + 1 > ANSWER_DEPTH)
		ReportError(0x7202);
	saved[depth++] = mark;
	count = 0;
	mark = 0;
}

void AnswerList::pop()
{
	Answer *a;

	truncate();
	if (depth - 1 >= 0) {
		depth--;
		mark = saved[depth];
	}
	a = 0;
	while (next(&a))
		count++;
}

int16_t AnswerList::next(Answer **a)
{
	*a = (Answer *) (*a == 0 ? mark : (*a)->next);
	return *a != 0;
}

/* Drop the answers of the current set: those from the mark on. */
void AnswerList::truncate()
{
	Link *n = head;
	Link *prev = 0;
	Link *after;

	while (mark != n && n != 0) {
		prev = n;
		n = n->next;
	}
	if (prev)
		prev->next = 0;
	if (mark == head)
		head = 0;
	tail = prev;
	for (n = mark; n; n = after) {
		after = n->next;
		delete n;
	}
	mark = 0;
	count = 0;
}

void AnswerList::remove(Answer *a, Answer *prev)
{
	LinkList_unlink(this, a, prev);
	delete a;
	count--;
}

void AnswerList::remove(char *s)
{
	Answer *a = 0;
	Answer *prev = 0;

	while (next(&a)) {
		if (strcmp(a->text, s) == 0) {
			if (mark == a)
				mark = mark->next;
			remove(a, prev);
			return;
		}
		prev = a;
	}
}

void AnswerList::clear()
{
	while (head) {
		Link *n = head->next;

		delete head;
		head = n;
	}
	head = tail = 0;
	mark = 0;
	depth = count = 0;
}

/* Whether the answers are the ones on offer when last asked, by the sum of their characters. */
int16_t AnswerList::unchanged()
{
	int32_t sum = 0;
	Answer *a;
	int16_t i;

	for (a = (Answer *) head; a; a = (Answer *) a->next) {
		i = 0;
		while (a->text[i])
			sum += a->text[i++];
	}
	if (sum != KeywordChecksum) {
		KeywordChecksum = sum;
		return 0;
	}
	KeywordChecksum = sum;
	return 1;
}

extern "C" void ResetKeywordsGlobals(void)
{
	KeywordChecksum = 0;
	memset((void *)&OfferedAnswers, 0, sizeof(OfferedAnswers));
	memset(ChosenAnswer, 0, sizeof(ChosenAnswer));
}

extern "C" void ConstructKeywordsGlobals(void)
{
	new (&OfferedAnswers) AnswerList();
}
