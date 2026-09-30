#ifndef U7_DEBUG_RECORDS_H
#define U7_DEBUG_RECORDS_H

#include "u7/archive.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The Serpent Isle beta's debug records.
 *
 * Every function opens with dbgfunc, whose data offset addresses one
 * NUL-terminated name per local slot, in slot order. Data offset zero holds
 * an "&NAME" string naming the original source module. dbgline records carry
 * original source line numbers and appear throughout the code.
 */
typedef struct U7DebugFunction {
	uint16_t function_id;
	const char *module;        /* Without the leading '&'; NULL when absent. */
	size_t module_length;
	uint16_t local_count;
	size_t name_offset;        /* dbgfunc's data operand. */
	uint16_t dbgfunc_word;     /* dbgfunc's first operand; meaning unresolved. */
	size_t line_count;
	uint16_t first_line;
	uint16_t last_line;
	bool names_complete;       /* One readable name for every local slot. */
} U7DebugFunction;

/* Reads the name of one local slot. Slot indices are zero-based. */
U7Result u7_debug_local_name(const U7Function *function,
		const U7DebugFunction *debug, uint16_t slot, const char **out_text,
		size_t *out_length);

/*
 * Reads the debug records of one function. U7_END when it carries none.
 */
U7Result u7_debug_read(const U7Function *function, U7DebugFunction *out_debug);

/* One source line marker: the code offset it precedes, and its line. */
typedef struct U7DebugLine {
	size_t code_offset;
	uint16_t line;
} U7DebugLine;

U7Result u7_debug_lines(const U7Function *function, U7DebugLine *out_lines,
		size_t capacity, size_t *out_count);

#ifdef __cplusplus
}
#endif

#endif
