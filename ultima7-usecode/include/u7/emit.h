#ifndef U7_EMIT_H
#define U7_EMIT_H

#include "u7/debug_records.h"
#include "u7/structure.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Kinds of constant whose values have names. */
enum {
	U7_CONST_NONE = 0,
	U7_CONST_EVENT,
	U7_CONST_SCRIPT,        /* A script list for the action queue. */
	U7_CONST_ITEM_FLAG,
	U7_CONST_NPC_STAT,
	U7_CONST_WORK_TYPE,
	U7_CONST_DIRECTION,
	U7_CONST_SHAPE,
	U7_CONST_USABLE,        /* A usecode function number, written #Name. */
	U7_CONST_ALIGNMENT,
	U7_CONST_ATTACK_MODE,
	U7_CONST_WEATHER,
	U7_CONST_EGG_CRITERIA,
	U7_CONST_SPECIAL,       /* Values the engine reads the same anywhere, such as -359 for any. */
	U7_CONST_KINDS
};

/* One script action: its name and the kind of each operand that follows it. */
typedef struct U7ScriptAction {
	const char *name;
	unsigned char operand_count;
	unsigned char operand_kinds[4];
} U7ScriptAction;

/* Names and lines an emitted listing may carry, all optional. */
typedef struct U7Names {
	const U7Function *function;
	/* Needed by the linear form, which names blocks rather than offsets. */
	const U7Cfg *cfg;
	const U7DebugFunction *debug;      /* Original local and module names. */
	const U7DebugLine *lines;
	size_t line_count;
	const char *const *intrinsics;     /* 256 entries, any of them NULL. */
	const char *const *functions;      /* 65536 entries by function number. */
	const char *const *flags;          /* 65536 entries by flag number. */
	/* What each constant names where an intrinsic takes an object. */
	const char *const *objects;        /* 65536 entries by value. */
	/* Per intrinsic, bit k: its k-th argument from the last pushed takes an object. */
	const uint8_t *object_arguments;   /* 256 entries. */
	/* Per routine, bit k: its parameter k takes an object. */
	const uint16_t *object_parameters; /* 65536 entries. */
	/* Per function number, nonzero when it returns a value: written ~Name. */
	const uint8_t *returns_value;      /* 65536 entries. */
	/* The name of a constant of a given kind, or NULL. */
	const char *(*constant_name)(const void *context, unsigned kind, unsigned value);
	const void *constant_context;
	/* Per intrinsic, the kind of each argument from the last pushed, and of its result. */
	const uint8_t (*argument_kinds)[8]; /* 256 entries. */
	const uint8_t *result_kinds;        /* 256 entries. */
	const U7ScriptAction *script_actions; /* 256 entries, by code. */
	/* Per routine, the kind each parameter takes; 0 for none. */
	const uint8_t (*parameter_kinds)[16]; /* 65536 entries. */
	/* Leave a named function's number to the header or the build order, as a
	 * project's sources do. */
	bool without_ids;
} U7Names;

typedef enum U7EmitStyle {
	/* Slot-numbered names and no annotations: what the compiler reads back. */
	U7_EMIT_CANONICAL = 0,
	/* Original names, source lines and intrinsic names: what a person reads. */
	U7_EMIT_ANNOTATED,
	/*
	 * Blocks in file order under their own labels, every branch written out.
	 * Less pleasant to read and exactly what the file says, so it is the form
	 * the compiler reads back.
	 */
	U7_EMIT_LINEAR
} U7EmitStyle;

U7Result u7_emit_function(FILE *out, const U7Function *function,
		const U7Lifted *lifted, const U7Structured *structured,
		const U7Names *names, U7EmitStyle style);

#ifdef __cplusplus
}
#endif

#endif
