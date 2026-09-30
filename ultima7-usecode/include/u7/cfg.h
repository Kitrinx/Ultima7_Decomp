#ifndef U7_CFG_H
#define U7_CFG_H

#include "u7/archive.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { U7_NO_BLOCK = -1 };

typedef struct U7Block {
	size_t first_instruction;
	size_t instruction_count;
	size_t start_offset;
	size_t end_offset;
	int taken;      /* Branch target, or U7_NO_BLOCK. */
	int fallthrough;/* Next block, or U7_NO_BLOCK for a terminator. */
} U7Block;

typedef struct U7Cfg {
	U7Instruction *instructions;
	size_t instruction_count;
	U7Block *blocks;
	size_t block_count;
	int *block_at_offset;   /* One entry per code byte, plus end of code. */
} U7Cfg;

void u7_cfg_release(U7Cfg *cfg);

/*
 * Splits one function into basic blocks.
 *
 * Leaders are the entry, every branch target, and the instruction after any
 * branch or terminator. A branch target that is not an instruction boundary
 * is an error, not a silently accepted edge.
 */
U7Result u7_cfg_build(const U7Function *function, U7Cfg *out_cfg);

/* True for opcodes that end a block by returning or aborting. */
bool u7_opcode_is_terminator(uint8_t opcode);

/* True for opcodes whose branch is always taken. */
bool u7_opcode_is_unconditional_branch(uint8_t opcode);

#ifdef __cplusplus
}
#endif

#endif
