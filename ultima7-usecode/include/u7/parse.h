#ifndef U7_PARSE_H
#define U7_PARSE_H

#include "u7/lower.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct U7ParsedFunction {
	uint16_t function_id;
	uint16_t argument_count;
	uint16_t local_count;
	uint16_t *links;
	size_t link_count;
	U7LiftedBlock *blocks;
	size_t block_count;
	/* A record carried as its bytes, written #bytes, where no form of the source
	 * says what it holds. */
	const uint8_t *record;
	size_t record_size;
} U7ParsedFunction;

typedef struct U7Parsed {
	U7Arena arena;
	U7ParsedFunction *functions;
	size_t count;
	size_t error_line;
	char error[192];
} U7Parsed;

void u7_parsed_release(U7Parsed *parsed);

/*
 * Reads the linear source form back into the representation the assembler
 * takes. It is deliberately strict: anything it cannot account for is an
 * error with a line number, never a silent omission.
 */
U7Result u7_parse_source(const char *text, size_t length, U7Parsed *out_parsed);

/* Fills in a unit for one parsed function, ready for u7_assemble_unit. */
void u7_parsed_unit(const U7ParsedFunction *function, U7Unit *out_unit);

/* What a name in the readable form stands for; each is looked up without its sigil. */
typedef enum U7NameKind {
	U7_NAME_FUNCTION,   /* A usecode function, by its declared name, or #Name as a value. */
	U7_NAME_INTRINSIC,  /* An engine call. */
	U7_NAME_FLAG,       /* $Name. */
	U7_NAME_ACTION,     /* _Name that is a script action code, pushed as a byte. */
	U7_NAME_CONSTANT,   /* Any other _Name, pushed as a word. */
	U7_NAME_ARGUMENTS,  /* How many arguments a usecode function takes, by its name. */
	U7_NAME_RETURNS     /* 1 where a usecode function returns a value, 0 where not. */
} U7NameKind;

typedef bool (*U7NameLookup)(const void *context, U7NameKind kind, const char *name,
		size_t length, int32_t *out_value);

/*
 * Compiles the readable form, laying each construct out as Origin's compiler
 * did: an if's test jumps past its arm, an if/else's first arm always ends in a
 * jump past the second, a loop's body ends in a jump back to its top, and every
 * function closes with a return the source leaves out. Functions written in
 * the linear form's own statements compile as well.
 *
 * What the engine relies on is kept whatever the source says: every function
 * a function calls is in its link table, added after those #uses lists; the
 * local count covers every slot the code uses; and where the lookup knows a
 * callee, its argument count and whether it returns a value are checked.
 */
U7Result u7_compile_source(const char *text, size_t length, U7NameLookup lookup,
		const void *context, U7Parsed *out_parsed);

#ifdef __cplusplus
}
#endif

#endif
