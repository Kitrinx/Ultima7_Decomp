#ifndef U7_NAMES_H
#define U7_NAMES_H

#include "u7/parse.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Names the readable form uses and the numbers they stand for. */
typedef struct U7NameEntry {
	U7NameKind kind;
	char *text;
	int32_t value;
	size_t order;             /* Later entries of one kind and name win. */
} U7NameEntry;

typedef struct U7NameTable {
	U7NameEntry *entries;
	size_t count;
	size_t capacity;
	bool sorted;
} U7NameTable;

bool u7_names_add(U7NameTable *table, U7NameKind kind, const char *text, int32_t value);
void u7_names_release(U7NameTable *table);

/* Looks a name up; the table is the context. Sorts it on first use. */
bool u7_names_lookup(const void *context, U7NameKind kind, const char *name, size_t length,
		int32_t *out_value);

/*
 * Reads a header in Origin's index format:
 *
 *     croutine SetSpeaker;               engine calls, numbered in the order declared
 *     cfunction Random;
 *     usableindex #Iolo 1025;
 *     global flag $SeenTetra = False;    //flag number 0
 *     constant _Avatar 65180;
 *     action _aWAIT_i 39;                a script action code, pushed as a byte
 *
 * A flag takes the number its comment gives, or the one after the last.
 */
U7Result u7_names_read_header(U7NameTable *table, const char *text, size_t length,
		size_t *out_error_line);

#ifdef __cplusplus
}
#endif

#endif
