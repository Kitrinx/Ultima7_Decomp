#include "u7/lower.h"

#include <stdlib.h>
#include <string.h>

typedef struct Pending {
	size_t offset;       /* Where the displacement field sits. */
	size_t after;        /* Offset the displacement is measured from. */
	size_t target_block;
} Pending;

typedef struct Fixup {
	size_t offset;   /* Where the data offset was written. */
	size_t entry;    /* Which collected string it refers to. */
} Fixup;

typedef struct Lowerer {
	const int *block_at_offset;
	/* Strings and call targets in the order the code first names them. */
	const U7Expr **strings;   /* One per data slot, in first-reference order. */
	size_t string_count;
	uint16_t *links;
	size_t link_count;
	Fixup *fixups;
	size_t fixup_count;
	bool rebuild;              /* Lay the data and links out afresh. */
	const U7Function *source;
	uint8_t *out;
	size_t capacity;
	size_t size;
	size_t instruction_count;
	Pending *pending;
	size_t pending_count;
	size_t pending_capacity;
	size_t *block_offset;
	bool failed;
	uint8_t failing_opcode;
} Lowerer;

/* Returns the entry for one data slot, adding it when newly seen. */
static size_t intern_string(Lowerer *lowerer, const U7Expr *expr) {
	for (size_t i = 0; i < lowerer->string_count; ++i) {
		if (lowerer->strings[i]->detail == expr->detail) {
			return i;
		}
	}
	const U7Expr **grown = realloc(lowerer->strings,
			(lowerer->string_count + 1U) * sizeof *grown);
	if (grown == NULL) {
		lowerer->failed = true;
		return 0;
	}
	lowerer->strings = grown;
	lowerer->strings[lowerer->string_count] = expr;
	return lowerer->string_count++;
}

/*
 * The link table is declared, not derived: it may list a function the code
 * never calls, and two of the shipped functions list theirs out of order.
 * So a call finds its index by looking its target up in the declaration.
 */
static size_t link_index(Lowerer *lowerer, uint16_t function_id, int32_t declared) {
	/* A table may list one target twice, so the call names its own slot. */
	if (declared >= 0 && (size_t) declared < lowerer->link_count &&
			lowerer->links[declared] == function_id) {
		return (size_t) declared;
	}
	for (size_t i = 0; i < lowerer->link_count; ++i) {
		if (lowerer->links[i] == function_id) {
			return i;
		}
	}
	lowerer->failed = true;
	return 0;
}

/* Notes where a data offset was written so it can be filled in later. */
static bool note_fixup(Lowerer *lowerer, size_t offset, size_t entry) {
	Fixup *grown = realloc(lowerer->fixups,
			(lowerer->fixup_count + 1U) * sizeof *grown);
	if (grown == NULL) {
		lowerer->failed = true;
		return false;
	}
	lowerer->fixups = grown;
	lowerer->fixups[lowerer->fixup_count].offset = offset;
	lowerer->fixups[lowerer->fixup_count].entry = entry;
	lowerer->fixup_count += 1U;
	return true;
}

static bool emit(Lowerer *lowerer, uint8_t opcode, const int32_t *operands,
		size_t operand_count) {
	size_t written = 0;
	if (lowerer->size > lowerer->capacity) {
		lowerer->failed = true;
		return false;
	}
	U7Result result = u7_encode_instruction(opcode, operands, operand_count,
			lowerer->out + lowerer->size, lowerer->capacity - lowerer->size, &written);
	if (result != U7_OK) {
		lowerer->failed = true;
		lowerer->failing_opcode = opcode;
		return false;
	}
	lowerer->size += written;
	lowerer->instruction_count += 1U;
	return true;
}

/* Emits a branch whose displacement is filled in once every block is placed. */
static bool emit_branch(Lowerer *lowerer, uint8_t opcode, const int32_t *operands,
		size_t operand_count, size_t displacement_index, size_t target_block) {
	const U7OpcodeInfo *info = u7_opcode_info(opcode);
	if (info == NULL) {
		lowerer->failed = true;
		return false;
	}
	size_t start = lowerer->size;
	if (!emit(lowerer, opcode, operands, operand_count)) {
		return false;
	}
	if (lowerer->pending_count == lowerer->pending_capacity) {
		size_t capacity = lowerer->pending_capacity == 0 ? 32U
				: lowerer->pending_capacity * 2U;
		Pending *grown = realloc(lowerer->pending, capacity * sizeof *grown);
		if (grown == NULL) {
			lowerer->failed = true;
			return false;
		}
		lowerer->pending = grown;
		lowerer->pending_capacity = capacity;
	}
	lowerer->pending[lowerer->pending_count].offset =
			start + info->operands[displacement_index].byte_offset;
	lowerer->pending[lowerer->pending_count].after = lowerer->size;
	lowerer->pending[lowerer->pending_count].target_block = target_block;
	lowerer->pending_count += 1U;
	return true;
}

static bool lower_expr(Lowerer *lowerer, const U7Expr *expr);

/* Debug records go out immediately before the instruction they preceded. */
static bool emit_debug(Lowerer *lowerer, const U7Stmt *record) {
	for (; record != NULL; record = record->next) {
		const U7OpcodeInfo *info = u7_opcode_info(record->opcode);
		if (info == NULL) {
			lowerer->failed = true;
			return false;
		}
		int32_t operands[U7_MAX_OPERANDS] = {0};
		for (size_t i = 0; i < info->operand_count; ++i) {
			operands[i] = record->operands[i];
		}
		if (!emit(lowerer, record->opcode, operands, info->operand_count)) {
			return false;
		}
	}
	return true;
}

/* Emits one node's own instruction, its debug records first. */
static bool emit_node(Lowerer *lowerer, const U7Expr *expr, const int32_t *operands,
		size_t operand_count) {
	return emit_debug(lowerer, expr->debug) &&
			emit(lowerer, expr->opcode, operands, operand_count);
}

static bool lower_operands(Lowerer *lowerer, const U7Expr *expr, bool reversed) {
	for (size_t i = 0; i < expr->operand_count; ++i) {
		size_t index = reversed ? expr->operand_count - 1U - i : i;
		if (!lower_expr(lowerer, expr->operands[index])) {
			return false;
		}
	}
	return true;
}

static bool lower_expr(Lowerer *lowerer, const U7Expr *expr) {
	if (expr == NULL) {
		lowerer->failed = true;
		return false;
	}
	int32_t operands[U7_MAX_OPERANDS] = {0};

	switch (expr->kind) {
		case U7_EXPR_CONSTANT:
			if (expr->opcode == 0x13U || expr->opcode == 0x14U) {
				return emit_node(lowerer, expr, NULL, 0);
			}
			operands[0] = expr->value;
			return emit_node(lowerer, expr, operands, 1U);

		case U7_EXPR_STRING: {
			const U7OpcodeInfo *info = u7_opcode_info(expr->opcode);
			if (info == NULL) {
				lowerer->failed = true;
				return false;
			}
			size_t entry = 0;
			if (lowerer->rebuild) {
				entry = intern_string(lowerer, expr);
				operands[0] = 0;
			} else {
				operands[0] = expr->detail;
			}
			if (!emit_debug(lowerer, expr->debug)) {
				return false;
			}
			size_t start = lowerer->size;
			if (!emit(lowerer, expr->opcode, operands, 1U)) {
				return false;
			}
			return !lowerer->rebuild ||
					note_fixup(lowerer, start + info->operands[0].byte_offset, entry);
		}

		case U7_EXPR_LOCAL:
		case U7_EXPR_FLAG:
			operands[0] = expr->value;
			return emit_node(lowerer, expr, operands, 1U);

		case U7_EXPR_EVENT:
		case U7_EXPR_ITEM:
			return emit_node(lowerer, expr, NULL, 0);

		case U7_EXPR_ELEMENT:
			if (!lower_expr(lowerer, expr->operands[0])) {
				return false;
			}
			operands[0] = expr->value;
			return emit_node(lowerer, expr, operands, 1U);

		case U7_EXPR_UNARY:
		case U7_EXPR_BINARY:
		case U7_EXPR_JOIN:
			return lower_operands(lowerer, expr, false) &&
					emit_node(lowerer, expr, NULL, 0);

		case U7_EXPR_ARRAY:
			/* Its first element is the value pushed last. */
			if (!lower_operands(lowerer, expr, true)) {
				return false;
			}
			operands[0] = (int32_t) expr->operand_count;
			return emit_node(lowerer, expr, operands, 1U);

		case U7_EXPR_INTRINSIC:
			if (!lower_operands(lowerer, expr, false)) {
				return false;
			}
			operands[0] = expr->value;
			operands[1] = expr->detail;
			return emit_node(lowerer, expr, operands, 2U);

		case U7_EXPR_CALL:
			if (!lower_operands(lowerer, expr, false)) {
				return false;
			}
			operands[0] = expr->opcode == 0x24U && lowerer->rebuild
					? (int32_t) link_index(lowerer, (uint16_t) expr->value, expr->detail)
					: expr->detail;
			return emit_node(lowerer, expr, operands, 1U);

		case U7_EXPR_INCOMING:
			/* Nothing produced it here; a predecessor left it on the stack. */
			return true;
	}
	lowerer->failed = true;
	return false;
}

/* A branch names a block directly, or a code offset the map translates. */
static size_t target_block(const Lowerer *lowerer, size_t branch_target) {
	return lowerer->block_at_offset != NULL
			? (size_t) lowerer->block_at_offset[branch_target]
			: branch_target;
}

static bool lower_statement(Lowerer *lowerer, const U7Cfg *cfg, const U7Stmt *statement) {
	(void) cfg;
	int32_t operands[U7_MAX_OPERANDS] = {0};

	/* Wraps emit so a statement's own debug records precede its opcode. */
	#define EMIT_OWN(count) \
		(emit_debug(lowerer, statement->debug) && \
		 emit(lowerer, statement->opcode, operands, (count)))

	switch (statement->kind) {
		case U7_STMT_ASSIGN_LOCAL:
		case U7_STMT_ASSIGN_FLAG:
			if (!lower_expr(lowerer, statement->value)) {
				return false;
			}
			operands[0] = statement->slot;
			return EMIT_OWN(1U);

		case U7_STMT_ASSIGN_EVENT:
			return lower_expr(lowerer, statement->value) && EMIT_OWN(0);

		case U7_STMT_ASSIGN_ELEMENT:
			/* The value goes down first, then the subscript on top. */
			if (!lower_expr(lowerer, statement->value) ||
					!lower_expr(lowerer, statement->subscript)) {
				return false;
			}
			operands[0] = statement->slot;
			return EMIT_OWN(1U);

		case U7_STMT_CALL:
			if (statement->opcode == 0x40U) {
				return EMIT_OWN(0);
			}
			return emit_debug(lowerer, statement->debug) &&
					lower_expr(lowerer, statement->value);

		case U7_STMT_APPEND: {
			if (statement->opcode != 0x1CU || !lowerer->rebuild) {
				operands[0] = statement->operands[0];
				return EMIT_OWN(1U);
			}
			/* Its operand names a string, so it takes part in the layout. */
			size_t entry = intern_string(lowerer, statement->value);
			operands[0] = 0;
			if (!emit_debug(lowerer, statement->debug)) {
				return false;
			}
			size_t start = lowerer->size;
			if (!emit(lowerer, statement->opcode, operands, 1U)) {
				return false;
			}
			return note_fixup(lowerer, start + 1U, entry);
		}

		case U7_STMT_SAY:
		case U7_STMT_ABORT:
		case U7_STMT_LOOP_START:
			return EMIT_OWN(0);

		case U7_STMT_DEBUG:
			return emit_debug(lowerer, statement);

		case U7_STMT_DISCARD:
			return emit_debug(lowerer, statement->debug) &&
					lower_expr(lowerer, statement->value);

		case U7_STMT_RETURN:
			if (statement->value != NULL && !lower_expr(lowerer, statement->value)) {
				return false;
			}
			return EMIT_OWN(0);

		case U7_STMT_BRANCH_IF_FALSE:
			if (!lower_expr(lowerer, statement->value) ||
					!emit_debug(lowerer, statement->debug)) {
				return false;
			}
			return emit_branch(lowerer, statement->opcode, operands, 1U, 0,
					target_block(lowerer, statement->branch_target));

		case U7_STMT_BRANCH:
		case U7_STMT_ASK:
			if (!emit_debug(lowerer, statement->debug)) {
				return false;
			}
			return emit_branch(lowerer, statement->opcode, operands, 1U, 0,
					target_block(lowerer, statement->branch_target));

		case U7_STMT_COMPARE_ANSWERS:
			if ((statement->value != NULL &&
					!lower_operands(lowerer, statement->value, false)) ||
					!emit_debug(lowerer, statement->debug)) {
				return false;
			}
			operands[0] = statement->operands[0];
			return emit_branch(lowerer, statement->opcode, operands, 2U, 1U,
					target_block(lowerer, statement->branch_target));

		case U7_STMT_DEFAULT_ANSWER:
			if (!emit_debug(lowerer, statement->debug)) {
				return false;
			}
			operands[0] = statement->operands[0];
			return emit_branch(lowerer, statement->opcode, operands, 2U, 1U,
					target_block(lowerer, statement->branch_target));

		case U7_STMT_LOOP:
			if (!emit_debug(lowerer, statement->debug)) {
				return false;
			}
			for (size_t i = 0; i < 4U; ++i) {
				operands[i] = statement->operands[i];
			}
			return emit_branch(lowerer, statement->opcode, operands, 5U, 4U,
					target_block(lowerer, statement->branch_target));
	}
	#undef EMIT_OWN
	lowerer->failed = true;
	return false;
}

/* Emits every block in its original order, then resolves the branches. */
static U7Result run_lowering(Lowerer *lowerer, const U7LiftedBlock *blocks,
		size_t block_count) {
	lowerer->block_offset = calloc(block_count + 1U, sizeof *lowerer->block_offset);
	if (lowerer->block_offset == NULL) {
		return U7_ERROR_CAPACITY;
	}

	for (size_t b = 0; b < block_count && !lowerer->failed; ++b) {
		lowerer->block_offset[b] = lowerer->size;
		for (const U7Stmt *statement = blocks[b].first; statement != NULL;
				statement = statement->next) {
			if (!lower_statement(lowerer, NULL, statement)) {
				break;
			}
		}
	}
	lowerer->block_offset[block_count] = lowerer->size;

	for (size_t i = 0; i < lowerer->pending_count && !lowerer->failed; ++i) {
		const Pending *fixup = &lowerer->pending[i];
		int64_t displacement = (int64_t) lowerer->block_offset[fixup->target_block] -
				(int64_t) fixup->after;
		if (displacement < INT16_MIN || displacement > INT16_MAX) {
			lowerer->failed = true;
			break;
		}
		u7_write_u16(lowerer->out + fixup->offset, (uint16_t) (int16_t) displacement);
	}
	return lowerer->failed ? U7_ERROR_MISMATCH : U7_OK;
}

U7Result u7_lower_function(const U7Function *function, const U7Cfg *cfg,
		const U7Lifted *lifted, uint8_t *out, size_t capacity,
		U7LowerReport *out_report) {
	if (function == NULL || cfg == NULL || lifted == NULL || out == NULL ||
			out_report == NULL) {
		return U7_ERROR_ARGUMENT;
	}
	memset(out_report, 0, sizeof *out_report);

	Lowerer lowerer;
	memset(&lowerer, 0, sizeof lowerer);
	lowerer.out = out;
	lowerer.capacity = capacity;
	lowerer.source = function;

	lowerer.block_at_offset = cfg->block_at_offset;
	U7Result outcome = run_lowering(&lowerer, lifted->blocks, cfg->block_count);
	if (outcome != U7_OK) {
		free(lowerer.pending);
		free(lowerer.block_offset);
		free(lowerer.strings);
		free(lowerer.links);
		free(lowerer.fixups);
		return outcome;
	}

	out_report->code_size = lowerer.size;
	out_report->instruction_count = lowerer.instruction_count;
	out_report->failing_opcode = lowerer.failing_opcode;
	if (!lowerer.failed && lowerer.size == function->code_size) {
		out_report->identical = memcmp(out, function->code, lowerer.size) == 0;
		if (!out_report->identical) {
			size_t at = 0;
			while (at < lowerer.size && out[at] == function->code[at]) {
				at += 1U;
			}
			out_report->first_difference = at;
		}
	}

	free(lowerer.pending);
	free(lowerer.block_offset);
	free(lowerer.strings);
	free(lowerer.links);
	free(lowerer.fixups);
	return lowerer.failed ? U7_ERROR_MISMATCH : U7_OK;
}

U7Result u7_assemble_unit(const U7Unit *unit, uint8_t *out, size_t capacity,
		U7LowerReport *out_report) {
	if (unit == NULL || out == NULL || out_report == NULL) {
		return U7_ERROR_ARGUMENT;
	}
	memset(out_report, 0, sizeof *out_report);

	uint8_t *code = malloc(0x10000U);
	if (code == NULL) {
		return U7_ERROR_CAPACITY;
	}

	Lowerer lowerer;
	memset(&lowerer, 0, sizeof lowerer);
	lowerer.out = code;
	lowerer.capacity = 0x10000U;
	lowerer.rebuild = true;
	lowerer.block_at_offset = unit->block_at_offset;
	lowerer.link_count = unit->link_count;
	size_t *new_offset = NULL;

	if (lowerer.link_count > 0) {
		lowerer.links = malloc(lowerer.link_count * sizeof *lowerer.links);
		if (lowerer.links == NULL) {
			free(code);
			return U7_ERROR_CAPACITY;
		}
		memcpy(lowerer.links, unit->links, lowerer.link_count * sizeof *lowerer.links);
	}

	U7Result result = run_lowering(&lowerer, unit->blocks, unit->block_count);
	if (result != U7_OK) {
		goto done;
	}

	/* Lay the strings out in the order the code first named them. */
	size_t data_size = 0;
	new_offset = calloc(lowerer.string_count + 1U, sizeof *new_offset);
	if (new_offset == NULL) {
		result = U7_ERROR_CAPACITY;
		goto done;
	}
	for (size_t i = 0; i < lowerer.string_count; ++i) {
		new_offset[i] = data_size;
		data_size += lowerer.strings[i]->text_length + 1U;
	}
	for (size_t i = 0; i < lowerer.fixup_count; ++i) {
		u7_write_u16(code + lowerer.fixups[i].offset,
				(uint16_t) new_offset[lowerer.fixups[i].entry]);
	}

	size_t payload = 2U + data_size + 6U + lowerer.link_count * 2U + lowerer.size;
	size_t total = 4U + payload;
	if (total > capacity || payload > 0xFFFFU) {
		result = U7_ERROR_CAPACITY;
		goto done;
	}

	u7_write_u16(out, unit->function_id);
	u7_write_u16(out + 2U, (uint16_t) payload);
	u7_write_u16(out + 4U, (uint16_t) data_size);
	for (size_t i = 0; i < lowerer.string_count; ++i) {
		const U7Expr *entry = lowerer.strings[i];
		memcpy(out + 6U + new_offset[i], entry->text, entry->text_length);
		out[6U + new_offset[i] + entry->text_length] = 0;
	}
	size_t at = 6U + data_size;
	u7_write_u16(out + at, unit->argument_count);
	u7_write_u16(out + at + 2U, unit->local_count);
	u7_write_u16(out + at + 4U, (uint16_t) lowerer.link_count);
	at += 6U;
	for (size_t i = 0; i < lowerer.link_count; ++i) {
		u7_write_u16(out + at + i * 2U, lowerer.links[i]);
	}
	at += lowerer.link_count * 2U;
	memcpy(out + at, code, lowerer.size);
	out_report->code_size = total;
	out_report->instruction_count = lowerer.instruction_count;

done:
	free(new_offset);
	free(code);
	free(lowerer.pending);
	free(lowerer.block_offset);
	free(lowerer.strings);
	free(lowerer.links);
	free(lowerer.fixups);
	if (result == U7_OK && lowerer.failed) {
		result = U7_ERROR_MISMATCH;
	}
	out_report->failing_opcode = lowerer.failing_opcode;
	return result;
}

U7Result u7_lower_record(const U7Function *function, const U7Cfg *cfg,
		const U7Lifted *lifted, uint8_t *out, size_t capacity,
		U7LowerReport *out_report) {
	if (function == NULL || cfg == NULL || lifted == NULL) {
		return U7_ERROR_ARGUMENT;
	}

	uint16_t *links = NULL;
	if (function->link_count > 0) {
		links = malloc(function->link_count * sizeof *links);
		if (links == NULL) {
			return U7_ERROR_CAPACITY;
		}
		for (uint16_t i = 0; i < function->link_count; ++i) {
			(void) u7_function_link(function, i, &links[i]);
		}
	}

	U7Unit unit = {
		.function_id = function->function_id,
		.argument_count = function->argument_count,
		.local_count = function->local_count,
		.links = links,
		.link_count = function->link_count,
		.blocks = lifted->blocks,
		.block_count = cfg->block_count,
		.block_at_offset = cfg->block_at_offset
	};

	U7Result result = u7_assemble_unit(&unit, out, capacity, out_report);
	free(links);
	if (result != U7_OK) {
		return result;
	}

	if (out_report->code_size == function->record_size) {
		const uint8_t *original = function->data - 6U;
		out_report->identical = memcmp(out, original, out_report->code_size) == 0;
		if (!out_report->identical) {
			size_t where = 0;
			while (where < out_report->code_size && out[where] == original[where]) {
				where += 1U;
			}
			out_report->first_difference = where;
		}
	}
	return U7_OK;
}
