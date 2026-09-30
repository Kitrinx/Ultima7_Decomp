#ifndef U7_STACK_FLOW_H
#define U7_STACK_FLOW_H

#include "u7/cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* What a call consumes and whether it leaves a result behind. */
typedef struct U7CallTarget {
	uint16_t argument_count;
	bool returns_value;
} U7CallTarget;

/* Answers for one callee; false when the target cannot be resolved. */
typedef bool (*U7CallResolver)(void *context, uint16_t function_id,
		U7CallTarget *out_target);

typedef struct U7StackEffect {
	uint8_t pops;
	uint8_t pushes;
	bool known;
} U7StackEffect;

/*
 * The stack effect of one instruction.
 *
 * Effects come from the sealed gameplay runtime's own dispatch. A handful of
 * opcodes have no established effect and are reported as unknown rather than
 * guessed, because a wrong effect silently corrupts every later depth.
 */
U7Result u7_stack_effect(const U7Function *function,
		const U7Instruction *instruction, U7CallResolver resolver,
		void *context, U7StackEffect *out_effect);

typedef struct U7StackReport {
	bool analyzed;          /* No unknown effect and every call resolved. */
	bool balanced;          /* Every join agrees on depth. */
	size_t max_depth;
	size_t exit_depth;      /* Depth reaching the first terminator. */
	bool exit_depths_agree; /* Every terminator sees the same depth. */
	size_t conflict_offset;
	uint8_t blocking_opcode;
	uint16_t unresolved_function;
} U7StackReport;

/*
 * Propagates stack depth through the control-flow graph from an empty entry.
 *
 * A depth that differs between two paths into one block is a conflict: either
 * the effect table is wrong or the code is not what it appears to be. Either
 * way it is reported, never averaged away.
 */
U7Result u7_stack_depths(const U7Function *function, const U7Cfg *cfg,
		U7CallResolver resolver, void *context, U7StackReport *out_report);

#ifdef __cplusplus
}
#endif

#endif
