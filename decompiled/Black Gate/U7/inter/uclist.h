#ifndef UCLIST_H
#define UCLIST_H

#include "ucvalue.h"

extern Node ResultNode;
extern Value ScratchValue;
char ParseWholeNumber(int *result, char *s);
Node *GetListNode(Value *value, int index);

#endif
