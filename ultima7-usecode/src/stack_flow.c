#include "u7/stack_flow.h"

#include <stdlib.h>
#include <string.h>

#define KNOWN(pop, push) {(pop), (push), true}
#define UNKNOWN {0, 0, false}

/* Indexed by opcode. Anything absent here is unknown by construction. */
static const U7StackEffect U7_EFFECTS[256] = {
	[0x02] = KNOWN(0, 0),        /* Iterates over an array through locals. */
	[0x04] = KNOWN(0, 0),        /* Branches when no answer is left. */
	[0x05] = KNOWN(1, 0),
	[0x06] = KNOWN(0, 0),
	[0x09] = KNOWN(2, 1), [0x0A] = KNOWN(2, 1), [0x0B] = KNOWN(2, 1),
	[0x0C] = KNOWN(2, 1), [0x0D] = KNOWN(2, 1), [0x0E] = KNOWN(2, 1),
	[0x0F] = KNOWN(2, 1),
	[0x10] = KNOWN(1, 1),
	[0x12] = KNOWN(1, 0),
	[0x13] = KNOWN(0, 1), [0x14] = KNOWN(0, 1),
	[0x16] = KNOWN(2, 1), [0x17] = KNOWN(2, 1), [0x18] = KNOWN(2, 1),
	[0x19] = KNOWN(2, 1), [0x1A] = KNOWN(2, 1),
	[0x1C] = KNOWN(0, 0),        /* Appends to the message buffer. */
	[0x1D] = KNOWN(0, 1),
	[0x1F] = KNOWN(0, 1),
	[0x21] = KNOWN(0, 1),
	[0x22] = KNOWN(2, 1),
	[0x25] = KNOWN(0, 0),
	[0x26] = KNOWN(1, 1),
	[0x2C] = KNOWN(0, 0),
	[0x2D] = KNOWN(1, 0),        /* Returns the value on top. */
	[0x2E] = KNOWN(0, 0),
	[0x2F] = KNOWN(0, 0),        /* Appends to the message buffer. */
	[0x30] = KNOWN(2, 1),
	[0x31] = KNOWN(0, 0),        /* Branches; touches no stack value. */
	[0x32] = KNOWN(0, 0),        /* Returns zero. */
	[0x33] = KNOWN(0, 0),        /* Says the message buffer. */
	[0x3E] = KNOWN(0, 1),
	[0x3F] = KNOWN(0, 0),
	[0x40] = KNOWN(0, 0),
	[0x42] = KNOWN(0, 1),
	[0x43] = KNOWN(1, 0),
	[0x44] = KNOWN(0, 1),
	[0x46] = KNOWN(2, 0),
	[0x48] = KNOWN(0, 1),
	[0x4A] = KNOWN(2, 1),        /* Joins the top two arrays. */
	[0x4B] = KNOWN(1, 0),
	[0x4C] = KNOWN(0, 0),
	[0x4D] = KNOWN(0, 0)
};

U7Result u7_stack_effect(const U7Function *function,
		const U7Instruction *instruction, U7CallResolver resolver,
		void *context, U7StackEffect *out_effect) {
	if (function == NULL || instruction == NULL || out_effect == NULL ||
			instruction->info == NULL) {
		return U7_ERROR_ARGUMENT;
	}
	out_effect->pops = 0;
	out_effect->pushes = 0;
	out_effect->known = false;

	uint8_t opcode = instruction->info->opcode;
	int32_t operand = 0;

	switch (opcode) {
		case 0x07U: /* Compares the top count answers. */
		case 0x1EU: /* Builds one array from the top count values. */
			if (u7_instruction_operand(instruction, 0, &operand) != U7_OK ||
					operand < 0 || operand > 0xFF) {
				return U7_ERROR_RANGE;
			}
			out_effect->pops = (uint8_t) operand;
			out_effect->pushes = opcode == 0x1EU ? 1U : 0U;
			out_effect->known = true;
			return U7_OK;

		case 0x38U: /* Intrinsic with a result. */
		case 0x39U: /* Intrinsic without one. */
			if (u7_instruction_operand(instruction, 1, &operand) != U7_OK ||
					operand < 0 || operand > 0xFF) {
				return U7_ERROR_RANGE;
			}
			out_effect->pops = (uint8_t) operand;
			out_effect->pushes = opcode == 0x38U ? 1U : 0U;
			out_effect->known = true;
			return U7_OK;

		case 0x24U:   /* Call through the link table. */
		case 0x47U: { /* Call by function identifier. */
			if (resolver == NULL) {
				return U7_OK;
			}
			if (u7_instruction_operand(instruction, 0, &operand) != U7_OK ||
					operand < 0) {
				return U7_ERROR_RANGE;
			}
			uint16_t target_id = (uint16_t) operand;
			if (opcode == 0x24U &&
					u7_function_link(function, (size_t) operand, &target_id) != U7_OK) {
				return U7_OK;
			}
			U7CallTarget target;
			memset(&target, 0, sizeof target);
			if (!resolver(context, target_id, &target)) {
				return U7_OK;
			}
			if (target.argument_count > 0xFF) {
				return U7_ERROR_RANGE;
			}
			out_effect->pops = (uint8_t) target.argument_count;
			out_effect->pushes = target.returns_value ? 1U : 0U;
			out_effect->known = true;
			return U7_OK;
		}

		default:
			*out_effect = U7_EFFECTS[opcode];
			return U7_OK;
	}
}

U7Result u7_stack_depths(const U7Function *function, const U7Cfg *cfg,
		U7CallResolver resolver, void *context, U7StackReport *out_report) {
	if (function == NULL || cfg == NULL || out_report == NULL) {
		return U7_ERROR_ARGUMENT;
	}
	memset(out_report, 0, sizeof *out_report);
	out_report->analyzed = true;
	out_report->balanced = true;
	out_report->exit_depths_agree = true;
	if (cfg->block_count == 0) {
		return U7_OK;
	}

	int *entry = malloc(cfg->block_count * sizeof *entry);
	size_t *queue = malloc(cfg->block_count * sizeof *queue);
	if (entry == NULL || queue == NULL) {
		free(entry);
		free(queue);
		return U7_ERROR_CAPACITY;
	}
	for (size_t i = 0; i < cfg->block_count; ++i) {
		entry[i] = -1;
	}

	entry[0] = 0;
	size_t head = 0;
	size_t tail = 0;
	queue[tail++] = 0;

	bool have_exit = false;
	U7Result result = U7_OK;

	while (head < tail) {
		size_t index = queue[head++];
		const U7Block *block = &cfg->blocks[index];
		int depth = entry[index];

		for (size_t i = 0; i < block->instruction_count; ++i) {
			const U7Instruction *instruction =
					&cfg->instructions[block->first_instruction + i];
			U7StackEffect effect;
			result = u7_stack_effect(function, instruction, resolver, context,
					&effect);
			if (result != U7_OK) {
				goto done;
			}
			if (!effect.known) {
				out_report->analyzed = false;
				if (out_report->blocking_opcode == 0) {
					out_report->blocking_opcode = instruction->info->opcode;
					out_report->conflict_offset = instruction->offset;
				}
				goto done;
			}
			depth -= effect.pops;
			if (depth < 0) {
				out_report->balanced = false;
				out_report->conflict_offset = instruction->offset;
				goto done;
			}
			depth += effect.pushes;
			if ((size_t) depth > out_report->max_depth) {
				out_report->max_depth = (size_t) depth;
			}
		}

		const U7Instruction *last = &cfg->instructions[block->first_instruction +
				block->instruction_count - 1U];
		if (u7_opcode_is_terminator(last->info->opcode)) {
			if (!have_exit) {
				out_report->exit_depth = (size_t) depth;
				have_exit = true;
			} else if (out_report->exit_depth != (size_t) depth) {
				out_report->exit_depths_agree = false;
			}
		}

		int successors[2] = {block->taken, block->fallthrough};
		for (size_t s = 0; s < 2; ++s) {
			int next = successors[s];
			if (next == U7_NO_BLOCK) {
				continue;
			}
			if (entry[next] == -1) {
				entry[next] = depth;
				queue[tail++] = (size_t) next;
			} else if (entry[next] != depth) {
				out_report->balanced = false;
				out_report->conflict_offset = cfg->blocks[next].start_offset;
				goto done;
			}
		}
	}

done:
	free(entry);
	free(queue);
	return result;
}
