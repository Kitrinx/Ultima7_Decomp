#ifndef U7_NORMALIZE_H
#define U7_NORMALIZE_H

#include "u7/archive.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A growable buffer holding one canonical function body. */
typedef struct U7Canonical {
	uint8_t *bytes;
	size_t size;
	size_t capacity;
} U7Canonical;

void u7_canonical_release(U7Canonical *canonical);

/*
 * Serializes a function body into a form two builds can be compared in.
 *
 * Debug records are dropped, jump displacements become indices into the
 * surviving instruction sequence, data offsets become the string they
 * address, and link indices become the function they resolve to. What
 * remains differs only when the code itself differs.
 */
U7Result u7_canonical_body(const U7Function *function, U7Canonical *out_canonical);

#ifdef __cplusplus
}
#endif

#endif
