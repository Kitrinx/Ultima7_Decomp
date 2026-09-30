#ifndef UCLIST_H
#define UCLIST_H

#include "ucvalue.h"

extern Node ResultNode;
extern Value ScratchValue;
int8_t ParseWholeNumber(int16_t *result, char *s);
Node *GetListNode(Value *value, int16_t index);

#endif
