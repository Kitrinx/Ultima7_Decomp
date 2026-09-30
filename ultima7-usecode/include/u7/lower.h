#ifndef U7_LOWER_H
#define U7_LOWER_H

#include "u7/expression.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct U7LowerReport {
	size_t code_size;
	bool identical;        /* Byte for byte with the function it came from. */
	size_t first_difference;
	size_t instruction_count;
	uint8_t failing_opcode;
} U7LowerReport;

/*
 * Turns the recovered statements back into bytecode.
 *
 * Blocks are emitted in their original order and branch displacements are
 * recomputed from the stream being built, never copied from the input, so a
 * match is evidence that nothing about the code was lost on the way in.
 */
U7Result u7_lower_function(const U7Function *function, const U7Cfg *cfg,
		const U7Lifted *lifted, uint8_t *out, size_t capacity,
		U7LowerReport *out_report);

/*
 * Rebuilds the whole record: data segment, counts, link table and code.
 *
 * The data segment is laid out from the strings the code refers to, in the
 * order it first refers to them, and the link table from the functions it
 * first calls. Neither is copied from the input, so a match says the layout
 * rules are right and not merely preserved.
 */
U7Result u7_lower_record(const U7Function *function, const U7Cfg *cfg,
		const U7Lifted *lifted, uint8_t *out, size_t capacity,
		U7LowerReport *out_report);

/*
 * One function to assemble, however it was obtained.
 *
 * A statement's branch target is a code offset when block_at_offset maps
 * them, and a block number when it does not.
 */
typedef struct U7Unit {
	uint16_t function_id;
	uint16_t argument_count;
	uint16_t local_count;
	const uint16_t *links;
	size_t link_count;
	const U7LiftedBlock *blocks;
	size_t block_count;
	const int *block_at_offset;
} U7Unit;

U7Result u7_assemble_unit(const U7Unit *unit, uint8_t *out, size_t capacity,
		U7LowerReport *out_report);

#ifdef __cplusplus
}
#endif

#endif
