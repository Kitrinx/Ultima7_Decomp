#ifndef U7_GENERATE_H
#define U7_GENERATE_H

#include "u7/lower.h"
#include "u7/structure.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Compiles a structured function as Origin's compiler laid code out: statements
 * in source order, every jump rebuilt from the construct that implies it, a
 * jump to the end after the first arm of an if/else, and a closing return.
 * Nothing about the original layout or its jumps is read; a match with the
 * original code says those rules are the compiler's.
 */
U7Result u7_generate_function(const U7Function *function, const U7Cfg *cfg,
		const U7Lifted *lifted, const U7Structured *structured,
		uint8_t *out, size_t capacity, U7LowerReport *out_report);

#ifdef __cplusplus
}
#endif

#endif
