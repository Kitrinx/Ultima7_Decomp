#include "u7/cfg.h"

#include <stdlib.h>
#include <string.h>

bool u7_opcode_is_terminator(uint8_t opcode) {
	/* 0x2D returns the value on top of the stack and 0x32 returns zero; both
	 * leave the frame, as the sealed gameplay runtime established. */
	return opcode == 0x25U || opcode == 0x2CU || opcode == 0x2DU ||
			opcode == 0x32U || opcode == 0x3FU;
}

bool u7_opcode_is_unconditional_branch(uint8_t opcode) {
	return opcode == 0x06U;
}

/* True when the opcode carries a branch displacement. */
static bool opcode_branches(const U7OpcodeInfo *info) {
	for (size_t i = 0; i < info->operand_count; ++i) {
		if (info->operands[i].kind == U7_OPERAND_RELATIVE_JUMP) {
			return true;
		}
	}
	return false;
}

static U7Result branch_target(const U7Instruction *instruction,
		size_t code_size, size_t *out_target) {
	for (size_t i = 0; i < instruction->info->operand_count; ++i) {
		if (instruction->info->operands[i].kind != U7_OPERAND_RELATIVE_JUMP) {
			continue;
		}
		int32_t displacement = 0;
		U7Result result = u7_instruction_operand(instruction, i, &displacement);
		if (result != U7_OK) {
			return result;
		}
		int64_t target = (int64_t) instruction->offset +
				instruction->encoded_size + displacement;
		if (target < 0 || (size_t) target > code_size) {
			return U7_ERROR_RANGE;
		}
		*out_target = (size_t) target;
		return U7_OK;
	}
	return U7_ERROR_ARGUMENT;
}

void u7_cfg_release(U7Cfg *cfg) {
	if (cfg == NULL) {
		return;
	}
	free(cfg->instructions);
	free(cfg->blocks);
	free(cfg->block_at_offset);
	memset(cfg, 0, sizeof *cfg);
}

U7Result u7_cfg_build(const U7Function *function, U7Cfg *out_cfg) {
	if (function == NULL || out_cfg == NULL) {
		return U7_ERROR_ARGUMENT;
	}
	memset(out_cfg, 0, sizeof *out_cfg);
	if (function->code_size == 0) {
		return U7_OK;
	}

	bool *is_leader = calloc(function->code_size + 1U, sizeof *is_leader);
	int *index_at_offset = malloc((function->code_size + 1U) * sizeof *index_at_offset);
	if (is_leader == NULL || index_at_offset == NULL) {
		free(is_leader);
		free(index_at_offset);
		return U7_ERROR_CAPACITY;
	}
	for (size_t i = 0; i <= function->code_size; ++i) {
		index_at_offset[i] = U7_NO_BLOCK;
	}

	/* First pass: decode every instruction and record boundaries. */
	size_t capacity = 64;
	U7Instruction *instructions = malloc(capacity * sizeof *instructions);
	if (instructions == NULL) {
		free(is_leader);
		free(index_at_offset);
		return U7_ERROR_CAPACITY;
	}

	size_t count = 0;
	size_t at = 0;
	U7Result result = U7_OK;
	while (at < function->code_size) {
		if (count == capacity) {
			capacity *= 2U;
			U7Instruction *grown = realloc(instructions, capacity * sizeof *grown);
			if (grown == NULL) {
				result = U7_ERROR_CAPACITY;
				goto done;
			}
			instructions = grown;
		}
		result = u7_decode_instruction(function->code, function->code_size, at,
				&instructions[count]);
		if (result != U7_OK) {
			goto done;
		}
		index_at_offset[at] = (int) count;
		at += instructions[count].encoded_size;
		count += 1U;
	}
	index_at_offset[function->code_size] = (int) count;

	/* Second pass: mark leaders. */
	is_leader[0] = true;
	for (size_t i = 0; i < count; ++i) {
		const U7Instruction *instruction = &instructions[i];
		uint8_t opcode = instruction->info->opcode;
		size_t next = instruction->offset + instruction->encoded_size;

		if (opcode_branches(instruction->info)) {
			size_t target = 0;
			result = branch_target(instruction, function->code_size, &target);
			if (result != U7_OK) {
				goto done;
			}
			if (index_at_offset[target] == U7_NO_BLOCK) {
				result = U7_ERROR_RANGE; /* Target is inside an instruction. */
				goto done;
			}
			is_leader[target] = true;
			if (next < function->code_size) {
				is_leader[next] = true;
			}
		} else if (u7_opcode_is_terminator(opcode) && next < function->code_size) {
			is_leader[next] = true;
		}
	}

	/* Third pass: cut blocks at leaders. */
	size_t block_capacity = 16;
	U7Block *blocks = malloc(block_capacity * sizeof *blocks);
	int *block_at_offset = malloc((function->code_size + 1U) * sizeof *block_at_offset);
	if (blocks == NULL || block_at_offset == NULL) {
		free(blocks);
		free(block_at_offset);
		result = U7_ERROR_CAPACITY;
		goto done;
	}
	for (size_t i = 0; i <= function->code_size; ++i) {
		block_at_offset[i] = U7_NO_BLOCK;
	}

	size_t block_count = 0;
	for (size_t i = 0; i < count; ++i) {
		if (is_leader[instructions[i].offset]) {
			if (block_count == block_capacity) {
				block_capacity *= 2U;
				U7Block *grown = realloc(blocks, block_capacity * sizeof *grown);
				if (grown == NULL) {
					free(blocks);
					free(block_at_offset);
					result = U7_ERROR_CAPACITY;
					goto done;
				}
				blocks = grown;
			}
			blocks[block_count].first_instruction = i;
			blocks[block_count].instruction_count = 0;
			blocks[block_count].start_offset = instructions[i].offset;
			blocks[block_count].taken = U7_NO_BLOCK;
			blocks[block_count].fallthrough = U7_NO_BLOCK;
			block_at_offset[instructions[i].offset] = (int) block_count;
			block_count += 1U;
		}
		blocks[block_count - 1U].instruction_count += 1U;
		blocks[block_count - 1U].end_offset =
				instructions[i].offset + instructions[i].encoded_size;
	}
	block_at_offset[function->code_size] = (int) block_count;

	/* Fourth pass: wire edges from each block's last instruction. */
	for (size_t b = 0; b < block_count; ++b) {
		const U7Instruction *last = &instructions[blocks[b].first_instruction +
				blocks[b].instruction_count - 1U];
		uint8_t opcode = last->info->opcode;

		if (opcode_branches(last->info)) {
			size_t target = 0;
			result = branch_target(last, function->code_size, &target);
			if (result != U7_OK) {
				free(blocks);
				free(block_at_offset);
				goto done;
			}
			blocks[b].taken = block_at_offset[target];
		}
		if (!u7_opcode_is_terminator(opcode) &&
				!u7_opcode_is_unconditional_branch(opcode) &&
				blocks[b].end_offset < function->code_size) {
			blocks[b].fallthrough = block_at_offset[blocks[b].end_offset];
		}
	}

	out_cfg->instructions = instructions;
	out_cfg->instruction_count = count;
	out_cfg->blocks = blocks;
	out_cfg->block_count = block_count;
	out_cfg->block_at_offset = block_at_offset;
	instructions = NULL;

done:
	free(is_leader);
	free(index_at_offset);
	free(instructions);
	return result;
}
