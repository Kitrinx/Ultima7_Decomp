#ifndef U7_CORPUS_H
#define U7_CORPUS_H

#include "u7/stack_flow.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One whole USECODE file, parsed into functions that borrow its bytes. */
typedef struct U7Corpus {
	uint8_t *bytes;
	size_t size;
	U7Function *functions;
	bool *returns_value;
	size_t count;
} U7Corpus;

U7Result u7_corpus_load(const char *path, U7Corpus *out_corpus);
/* Parses a USECODE already in memory; the corpus takes the bytes, which must be malloc'd. */
U7Result u7_corpus_parse(uint8_t *bytes, size_t size, U7Corpus *out_corpus);
void u7_corpus_release(U7Corpus *corpus);

const U7Function *u7_corpus_find(const U7Corpus *corpus, uint16_t function_id);

/* A U7CallResolver over a loaded corpus; pass the corpus as its context. */
bool u7_corpus_resolve(void *context, uint16_t function_id, U7CallTarget *out_target);

#ifdef __cplusplus
}
#endif

#endif
