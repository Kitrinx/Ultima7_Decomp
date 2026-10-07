#ifndef KEYWORDS_H
#define KEYWORDS_H

/* The answers offered in a conversation. */

#include <stddef.h>
#include "colbuf.h"

/* an answer's longest text, with its terminating zero */
#define ANSWER_SIZE     30

/* how many sets of answers can be pushed */
#define ANSWER_DEPTH    5

/* one conversation answer */
struct Answer : Link {
	char *text;
	Answer() {}
	Answer(char *s);
	~Answer();
	void *operator new(size_t);
};

/* the answers on offer, with a stack of the sets pushed before them */
struct AnswerList : List {
	Link *saved[ANSWER_DEPTH];
	Link *mark;
	int16_t depth;
	int16_t count;
	AnswerList() { depth = 0; count = 0; mark = 0; }
	~AnswerList() { clear(); }
	void append(char *s);
	void add(char *s);
	void push();
	void pop();
	int16_t next(Answer **a);
	void truncate();
	void remove(Answer *a, Answer *prev);
	void remove(char *s);
	void clear();
	int16_t unchanged();
};

extern AnswerList OfferedAnswers;

/* the text of the answer picked */
extern char ChosenAnswer[ANSWER_SIZE];

extern int32_t KeywordChecksum;

#endif
