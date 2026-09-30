#ifndef U7_LINKDEP_H
#define U7_LINKDEP_H

#include "u7/corpus.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Function numbers the dependency files can index. */
enum { U7_LINKDEP_FUNCTIONS = 4096 };

/*
 * What the engine can take. It reads LINKDEP2 with a 16-bit length. To run a
 * function it loads it with everything its link table reaches, 35 functions
 * at most, into one block: their payloads plus 146 bytes, sized in 16 bits.
 */
enum {
	U7_LINKDEP_ENTRIES = 0xFFFF / 4,
	U7_LOAD_FUNCTIONS = 35,
	U7_LOAD_BYTES = 0xFFFF - 146
};

/* What the engine loads to run one function. */
typedef struct U7Load {
	size_t functions;  /* It and everything its link table reaches. */
	size_t bytes;      /* Their payloads. */
} U7Load;

/*
 * Fills out[id] for each function present; out holds U7_LINKDEP_FUNCTIONS
 * entries. The loads' functions add up to the entries LINKDEP2 will hold.
 */
U7Result u7_linkdep_loads(const U7Corpus *corpus, U7Load *out);

/*
 * Writes the two files the engine reads beside USECODE. LINKDEP2 holds, for
 * each function, the record offsets of everything its link table reaches,
 * itself included, sorted by number. LINKDEP1 holds, for each number up to
 * the highest, where that run starts in LINKDEP2 and its total payload, or
 * 0xFFFF where no function has the number, then a terminating pair. Both hold
 * absolute offsets, so they are rebuilt whenever USECODE is.
 */
U7Result u7_write_linkdep(const U7Corpus *corpus, FILE *one, FILE *two, size_t *out_entries);

#ifdef __cplusplus
}
#endif

#endif
