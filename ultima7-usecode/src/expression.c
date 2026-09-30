#include "u7/expression.h"

#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

struct U7ArenaBlock {
	U7ArenaBlock *next;
	size_t used;
	size_t capacity;
	uint8_t data[];
};

enum { U7_ARENA_BLOCK = 16U * 1024U };

void *u7_arena_alloc(U7Arena *arena, size_t size) {
	if (arena == NULL || size == 0) {
		return NULL;
	}
	size_t aligned = (size + alignof(max_align_t) - 1U) & ~(alignof(max_align_t) - 1U);

	if (arena->head == NULL || arena->head->capacity - arena->head->used < aligned) {
		size_t capacity = aligned > U7_ARENA_BLOCK ? aligned : U7_ARENA_BLOCK;
		U7ArenaBlock *block = malloc(sizeof *block + capacity);
		if (block == NULL) {
			return NULL;
		}
		block->next = arena->head;
		block->used = 0;
		block->capacity = capacity;
		arena->head = block;
	}

	void *result = arena->head->data + arena->head->used;
	arena->head->used += aligned;
	memset(result, 0, size);
	return result;
}

void u7_arena_release(U7Arena *arena) {
	if (arena == NULL) {
		return;
	}
	U7ArenaBlock *block = arena->head;
	while (block != NULL) {
		U7ArenaBlock *next = block->next;
		free(block);
		block = next;
	}
	arena->head = NULL;
}

void u7_lifted_release(U7Lifted *lifted) {
	if (lifted == NULL) {
		return;
	}
	u7_arena_release(&lifted->arena);
	free(lifted->blocks);
	memset(lifted, 0, sizeof *lifted);
}

/* One block's working state while its instructions are folded into trees. */
typedef struct Lifter {
	U7Lifted *lifted;
	const U7Function *function;
	size_t current_offset;
	uint8_t current_opcode;
	int32_t current_operands[U7_MAX_OPERANDS];
	U7Expr **stack;
	size_t depth;
	size_t capacity;
	size_t consumed_incoming;
	U7Stmt *first;
	U7Stmt *last;
	size_t statement_count;
	U7Stmt *pending_first;   /* Debug records awaiting the next instruction. */
	U7Stmt *pending_last;
} Lifter;

/* Hands the waiting debug records to whatever is built next. */
static U7Stmt *take_pending(Lifter *lifter) {
	U7Stmt *chain = lifter->pending_first;
	lifter->pending_first = NULL;
	lifter->pending_last = NULL;
	return chain;
}

static U7Expr *make_expr(Lifter *lifter, U7ExprKind kind, uint8_t opcode) {
	U7Expr *expr = u7_arena_alloc(&lifter->lifted->arena, sizeof *expr);
	if (expr != NULL) {
		expr->kind = kind;
		expr->opcode = opcode;
		expr->origin = lifter->current_offset;
		expr->debug = take_pending(lifter);
	}
	return expr;
}

static bool push_expr(Lifter *lifter, U7Expr *expr) {
	if (expr == NULL || lifter->depth == lifter->capacity) {
		return false;
	}
	lifter->stack[lifter->depth++] = expr;
	return true;
}

static U7Expr *pop_expr(Lifter *lifter) {
	if (lifter->depth == 0) {
		return NULL;
	}
	U7Expr *expr = lifter->stack[--lifter->depth];
	if (expr->kind == U7_EXPR_INCOMING) {
		lifter->consumed_incoming += 1U;
	}
	return expr;
}

/* Collects count operands in source order: the deepest value came first. */
static U7Expr **pop_operands(Lifter *lifter, size_t count) {
	if (count > lifter->depth) {
		return NULL;
	}
	U7Expr **operands = u7_arena_alloc(&lifter->lifted->arena,
			count * sizeof *operands);
	if (operands == NULL && count > 0) {
		return NULL;
	}
	for (size_t i = 0; i < count; ++i) {
		operands[count - 1U - i] = pop_expr(lifter);
	}
	return operands;
}

/* The earliest instruction any part of this expression came from. */
static size_t earliest_origin(const U7Expr *expr, size_t best) {
	if (expr == NULL) {
		return best;
	}
	if (expr->kind != U7_EXPR_INCOMING && expr->origin < best) {
		best = expr->origin;
	}
	for (size_t i = 0; i < expr->operand_count; ++i) {
		best = earliest_origin(expr->operands[i], best);
	}
	return best;
}

static U7Stmt *add_statement(Lifter *lifter, U7StmtKind kind, size_t offset) {
	U7Stmt *statement = u7_arena_alloc(&lifter->lifted->arena, sizeof *statement);
	if (statement == NULL) {
		return NULL;
	}
	statement->kind = kind;
	statement->offset = offset;
	statement->first_offset = offset;
	statement->opcode = lifter->current_opcode;
	statement->debug = take_pending(lifter);
	memcpy(statement->operands, lifter->current_operands,
			sizeof statement->operands);
	if (lifter->last == NULL) {
		lifter->first = statement;
	} else {
		lifter->last->next = statement;
	}
	lifter->last = statement;
	lifter->statement_count += 1U;
	return statement;
}

static size_t jump_target(const U7Instruction *instruction) {
	for (size_t i = 0; i < instruction->info->operand_count; ++i) {
		if (instruction->info->operands[i].kind != U7_OPERAND_RELATIVE_JUMP) {
			continue;
		}
		int32_t displacement = 0;
		if (u7_instruction_operand(instruction, i, &displacement) != U7_OK) {
			return 0;
		}
		return (size_t) ((int64_t) instruction->offset +
				instruction->encoded_size + displacement);
	}
	return 0;
}

static bool lift_instruction(Lifter *lifter, const U7Instruction *instruction,
		U7CallResolver resolver, void *context) {
	uint8_t opcode = instruction->info->opcode;
	int32_t first = 0;
	int32_t second = 0;
	if (instruction->info->operand_count > 0) {
		(void) u7_instruction_operand(instruction, 0, &first);
	}
	if (instruction->info->operand_count > 1) {
		(void) u7_instruction_operand(instruction, 1, &second);
	}

	switch (opcode) {
		case 0x4CU: /* A source line or name record. It can sit in the middle
		             * of an expression, so it waits for the instruction it
		             * precedes rather than becoming a statement of its own. */
		case 0x4DU: {
			U7Stmt *record = u7_arena_alloc(&lifter->lifted->arena, sizeof *record);
			if (record == NULL) {
				return false;
			}
			record->kind = U7_STMT_DEBUG;
			record->opcode = opcode;
			record->offset = instruction->offset;
			record->first_offset = instruction->offset;
			memcpy(record->operands, lifter->current_operands,
					sizeof record->operands);
			if (lifter->pending_last == NULL) {
				lifter->pending_first = record;
			} else {
				lifter->pending_last->next = record;
			}
			lifter->pending_last = record;
			return true;
		}

		case 0x13U:
		case 0x14U:
		case 0x1FU:
		case 0x44U: {
			U7Expr *expr = make_expr(lifter, U7_EXPR_CONSTANT, opcode);
			if (expr == NULL) {
				return false;
			}
			expr->value = opcode == 0x13U ? 1 : opcode == 0x14U ? 0 : first;
			return push_expr(lifter, expr);
		}

		case 0x1DU: { /* Pushes a string from the data segment. */
			U7Expr *expr = make_expr(lifter, U7_EXPR_STRING, opcode);
			if (expr != NULL) {
				expr->detail = first;
			}
			if (expr == NULL ||
					u7_function_string(lifter->function, (size_t) first,
							&expr->text, &expr->text_length) != U7_OK) {
				return false;
			}
			return push_expr(lifter, expr);
		}

		case 0x21U: { /* Pushes a local. */
			U7Expr *expr = make_expr(lifter, U7_EXPR_LOCAL, opcode);
			if (expr == NULL) {
				return false;
			}
			expr->value = first;
			return push_expr(lifter, expr);
		}

		case 0x42U: {
			U7Expr *expr = make_expr(lifter, U7_EXPR_FLAG, opcode);
			if (expr == NULL) {
				return false;
			}
			expr->value = first;
			return push_expr(lifter, expr);
		}

		case 0x48U: {
			U7Expr *expr = make_expr(lifter, U7_EXPR_EVENT, opcode);
			return expr != NULL && push_expr(lifter, expr);
		}

		case 0x3EU: {
			U7Expr *expr = make_expr(lifter, U7_EXPR_ITEM, opcode);
			return expr != NULL && push_expr(lifter, expr);
		}

		case 0x26U: { /* One-based subscript of a local. */
			U7Expr *index = pop_expr(lifter);
			U7Expr *expr = make_expr(lifter, U7_EXPR_ELEMENT, opcode);
			if (index == NULL || expr == NULL) {
				return false;
			}
			expr->value = first;
			expr->operands = u7_arena_alloc(&lifter->lifted->arena, sizeof *expr->operands);
			if (expr->operands == NULL) {
				return false;
			}
			expr->operands[0] = index;
			expr->operand_count = 1U;
			return push_expr(lifter, expr);
		}

		case 0x09U: case 0x0AU: case 0x0BU: case 0x0CU: case 0x0DU:
		case 0x0EU: case 0x0FU: case 0x16U: case 0x17U: case 0x18U:
		case 0x19U: case 0x1AU: case 0x22U: case 0x30U: {
			U7Expr **operands = pop_operands(lifter, 2U);
			U7Expr *expr = make_expr(lifter, U7_EXPR_BINARY, opcode);
			if (operands == NULL || expr == NULL ||
					operands[0] == NULL || operands[1] == NULL) {
				return false;
			}
			expr->operands = operands;
			expr->operand_count = 2U;
			return push_expr(lifter, expr);
		}

		case 0x10U: {
			U7Expr *operand = pop_expr(lifter);
			U7Expr *expr = make_expr(lifter, U7_EXPR_UNARY, opcode);
			if (operand == NULL || expr == NULL) {
				return false;
			}
			expr->operands = u7_arena_alloc(&lifter->lifted->arena, sizeof *expr->operands);
			if (expr->operands == NULL) {
				return false;
			}
			expr->operands[0] = operand;
			expr->operand_count = 1U;
			return push_expr(lifter, expr);
		}

		case 0x1EU: { /* Builds an array from the top values. */
			U7Expr **operands = pop_operands(lifter, (size_t) first);
			U7Expr *expr = make_expr(lifter, U7_EXPR_ARRAY, opcode);
			if ((operands == NULL && first > 0) || expr == NULL) {
				return false;
			}
			/* Unlike a call, the array's first element is the value pushed
			 * last, so what came off the stack reads back to front. */
			for (size_t i = 0, j = (size_t) first; i < j; ++i) {
				j -= 1U;
				U7Expr *swap = operands[i];
				operands[i] = operands[j];
				operands[j] = swap;
			}
			expr->operands = operands;
			expr->operand_count = (size_t) first;
			return push_expr(lifter, expr);
		}

		case 0x4AU: {
			U7Expr **operands = pop_operands(lifter, 2U);
			U7Expr *expr = make_expr(lifter, U7_EXPR_JOIN, opcode);
			if (operands == NULL || expr == NULL ||
					operands[0] == NULL || operands[1] == NULL) {
				return false;
			}
			expr->operands = operands;
			expr->operand_count = 2U;
			return push_expr(lifter, expr);
		}

		case 0x38U:
		case 0x39U: {
			U7Expr **operands = pop_operands(lifter, (size_t) second);
			U7Expr *expr = make_expr(lifter, U7_EXPR_INTRINSIC, opcode);
			if ((operands == NULL && second > 0) || expr == NULL) {
				return false;
			}
			expr->value = first;
			expr->detail = second;
			expr->operands = operands;
			expr->operand_count = (size_t) second;
			if (opcode == 0x38U) {
				return push_expr(lifter, expr);
			}
			U7Stmt *statement = add_statement(lifter, U7_STMT_CALL, instruction->offset);
			if (statement == NULL) {
				return false;
			}
			statement->value = expr;
			return true;
		}

		case 0x24U:
		case 0x47U: {
			uint16_t target_id = (uint16_t) first;
			if (opcode == 0x24U && u7_function_link(lifter->function,
					(size_t) first, &target_id) != U7_OK) {
				return false;
			}
			U7CallTarget target;
			memset(&target, 0, sizeof target);
			if (resolver == NULL || !resolver(context, target_id, &target)) {
				return false;
			}
			U7Expr **operands = pop_operands(lifter, target.argument_count);
			U7Expr *expr = make_expr(lifter, U7_EXPR_CALL, opcode);
			if ((operands == NULL && target.argument_count > 0) || expr == NULL) {
				return false;
			}
			expr->value = target_id;
			expr->detail = first;
			expr->operands = operands;
			expr->operand_count = target.argument_count;
			if (target.returns_value) {
				return push_expr(lifter, expr);
			}
			U7Stmt *statement = add_statement(lifter, U7_STMT_CALL, instruction->offset);
			if (statement == NULL) {
				return false;
			}
			statement->value = expr;
			return true;
		}

		case 0x12U:
		case 0x43U:
		case 0x4BU: {
			U7Expr *value = pop_expr(lifter);
			U7StmtKind kind = opcode == 0x12U ? U7_STMT_ASSIGN_LOCAL
					: opcode == 0x43U ? U7_STMT_ASSIGN_FLAG : U7_STMT_ASSIGN_EVENT;
			U7Stmt *statement = add_statement(lifter, kind, instruction->offset);
			if (value == NULL || statement == NULL) {
				return false;
			}
			statement->slot = first;
			statement->value = value;
			return true;
		}

		case 0x46U: { /* Stores into a one-based element of a local. */
			U7Expr *index = pop_expr(lifter);
			U7Expr *value = pop_expr(lifter);
			U7Stmt *statement = add_statement(lifter, U7_STMT_ASSIGN_ELEMENT,
					instruction->offset);
			if (index == NULL || value == NULL || statement == NULL) {
				return false;
			}
			statement->slot = first;
			statement->subscript = index;
			statement->value = value;
			return true;
		}

		case 0x1CU:
		case 0x2FU: { /* Adds a string or a local to the message. */
			U7Stmt *statement = add_statement(lifter, U7_STMT_APPEND, instruction->offset);
			if (statement == NULL) {
				return false;
			}
			statement->slot = first;
			if (opcode == 0x1CU) {
				U7Expr *expr = make_expr(lifter, U7_EXPR_STRING, opcode);
				if (expr == NULL || u7_function_string(lifter->function,
						(size_t) first, &expr->text, &expr->text_length) != U7_OK) {
					return false;
				}
				expr->detail = first;
				statement->value = expr;
			} else {
				U7Expr *expr = make_expr(lifter, U7_EXPR_LOCAL, opcode);
				if (expr == NULL) {
					return false;
				}
				expr->value = first;
				statement->value = expr;
			}
			return true;
		}

		case 0x33U:
			return add_statement(lifter, U7_STMT_SAY, instruction->offset) != NULL;

		case 0x25U:
		case 0x2CU:
		case 0x32U:
			return add_statement(lifter, U7_STMT_RETURN, instruction->offset) != NULL;

		case 0x2DU: { /* Returns the value on top. */
			U7Expr *value = pop_expr(lifter);
			U7Stmt *statement = add_statement(lifter, U7_STMT_RETURN, instruction->offset);
			if (value == NULL || statement == NULL) {
				return false;
			}
			statement->value = value;
			return true;
		}

		case 0x3FU:
			return add_statement(lifter, U7_STMT_ABORT, instruction->offset) != NULL;

		case 0x05U: {
			U7Expr *condition = pop_expr(lifter);
			U7Stmt *statement = add_statement(lifter, U7_STMT_BRANCH_IF_FALSE,
					instruction->offset);
			if (condition == NULL || statement == NULL) {
				return false;
			}
			statement->value = condition;
			statement->branch_target = jump_target(instruction);
			return true;
		}

		case 0x06U: {
			U7Stmt *statement = add_statement(lifter, U7_STMT_BRANCH, instruction->offset);
			if (statement == NULL) {
				return false;
			}
			statement->branch_target = jump_target(instruction);
			return true;
		}

		case 0x04U:
		case 0x31U: {
			U7Stmt *statement = add_statement(lifter,
					opcode == 0x04U ? U7_STMT_ASK : U7_STMT_DEFAULT_ANSWER,
					instruction->offset);
			if (statement == NULL) {
				return false;
			}
			statement->slot = opcode == 0x31U ? first : 0;
			statement->branch_target = jump_target(instruction);
			return true;
		}

		case 0x07U: {
			U7Expr **operands = pop_operands(lifter, (size_t) first);
			U7Stmt *statement = add_statement(lifter, U7_STMT_COMPARE_ANSWERS,
					instruction->offset);
			if ((operands == NULL && first > 0) || statement == NULL) {
				return false;
			}
			U7Expr *expr = make_expr(lifter, U7_EXPR_ARRAY, opcode);
			if (expr == NULL) {
				return false;
			}
			expr->operands = operands;
			expr->operand_count = (size_t) first;
			statement->value = expr;
			statement->branch_target = jump_target(instruction);
			return true;
		}

		case 0x02U: {
			U7Stmt *statement = add_statement(lifter, U7_STMT_LOOP, instruction->offset);
			if (statement == NULL) {
				return false;
			}
			statement->branch_target = jump_target(instruction);
			return true;
		}

		case 0x2EU:
			return add_statement(lifter, U7_STMT_LOOP_START, instruction->offset) != NULL;

		case 0x40U:
			return add_statement(lifter, U7_STMT_CALL, instruction->offset) != NULL;

		default:
			return false;
	}
}

U7Result u7_lift_function(const U7Function *function, const U7Cfg *cfg,
		U7CallResolver resolver, void *context, U7Lifted *out_lifted) {
	if (function == NULL || cfg == NULL || out_lifted == NULL) {
		return U7_ERROR_ARGUMENT;
	}
	memset(out_lifted, 0, sizeof *out_lifted);
	out_lifted->complete = true;
	out_lifted->self_contained = true;
	if (cfg->block_count == 0) {
		return U7_OK;
	}

	out_lifted->blocks = calloc(cfg->block_count, sizeof *out_lifted->blocks);
	if (out_lifted->blocks == NULL) {
		return U7_ERROR_CAPACITY;
	}
	out_lifted->block_count = cfg->block_count;

	/* Any block may be entered carrying values, so give every one a deep
	 * enough working stack and seed it with placeholders. */
	enum { WORKING = 256U };
	U7Expr *slots[WORKING];

	for (size_t b = 0; b < cfg->block_count; ++b) {
		const U7Block *block = &cfg->blocks[b];
		Lifter lifter;
		memset(&lifter, 0, sizeof lifter);
		lifter.lifted = out_lifted;
		lifter.function = function;
		lifter.stack = slots;
		lifter.capacity = WORKING;

		/* Placeholders stand in for whatever predecessors left behind. */
		enum { SEED = 16U };
		for (size_t i = 0; i < SEED; ++i) {
			U7Expr *incoming = make_expr(&lifter, U7_EXPR_INCOMING, 0);
			if (incoming == NULL) {
				u7_lifted_release(out_lifted);
				return U7_ERROR_CAPACITY;
			}
			incoming->value = (int32_t) i;
			lifter.stack[lifter.depth++] = incoming;
		}

		bool ok = true;
		for (size_t i = 0; i < block->instruction_count && ok; ++i) {
			const U7Instruction *instruction =
					&cfg->instructions[block->first_instruction + i];
			lifter.current_offset = instruction->offset;
			lifter.current_opcode = instruction->info->opcode;
			memset(lifter.current_operands, 0, sizeof lifter.current_operands);
			for (size_t k = 0; k < instruction->info->operand_count; ++k) {
				(void) u7_instruction_operand(instruction, k,
						&lifter.current_operands[k]);
			}
			ok = lift_instruction(&lifter, instruction, resolver, context);
			if (ok && lifter.last != NULL && lifter.last->offset == instruction->offset) {
				U7Stmt *statement = lifter.last;
				size_t first = earliest_origin(statement->value, statement->offset);
				first = earliest_origin(statement->subscript, first);
				statement->first_offset = first;
			}
			if (!ok) {
				out_lifted->complete = false;
				out_lifted->failure_offset = instruction->offset;
				out_lifted->failure_opcode = instruction->info->opcode;
			}
		}

		if (ok && lifter.pending_first != NULL) {
			U7Stmt *trailing = take_pending(&lifter);
			while (trailing != NULL) {
				U7Stmt *next = trailing->next;
				trailing->next = NULL;
				if (lifter.last == NULL) {
					lifter.first = trailing;
				} else {
					lifter.last->next = trailing;
				}
				lifter.last = trailing;
				lifter.statement_count += 1U;
				trailing = next;
			}
		}

		U7LiftedBlock *lifted_block = &out_lifted->blocks[b];
		lifted_block->first = lifter.first;
		lifted_block->statement_count = lifter.statement_count;
		lifted_block->incoming_count = lifter.consumed_incoming;
		out_lifted->consumed_incoming += lifter.consumed_incoming;
		if (lifter.consumed_incoming > 0) {
			out_lifted->self_contained = false;
		}

		/* A value pushed and never consumed still cost instructions, so it
		 * is kept as a statement placed where its own instruction ran. */
		for (size_t i = 0; i < lifter.depth; ++i) {
			U7Expr *leftover = lifter.stack[i];
			if (leftover == NULL || leftover->kind == U7_EXPR_INCOMING) {
				continue;
			}
			U7Stmt *discard = u7_arena_alloc(&out_lifted->arena, sizeof *discard);
			if (discard == NULL) {
				u7_lifted_release(out_lifted);
				return U7_ERROR_CAPACITY;
			}
			discard->kind = U7_STMT_DISCARD;
			discard->offset = leftover->origin;
			discard->first_offset = leftover->origin;
			discard->value = leftover;

			U7Stmt **link = &lifter.first;
			while (*link != NULL && (*link)->offset <= discard->offset) {
				link = &(*link)->next;
			}
			discard->next = *link;
			*link = discard;
			if (discard->next == NULL) {
				lifter.last = discard;
			}
			lifter.statement_count += 1U;
		}

		size_t left = lifter.depth > SEED ? lifter.depth - SEED : 0;
		if (left > 0) {
			lifted_block->outgoing = u7_arena_alloc(&out_lifted->arena,
					left * sizeof *lifted_block->outgoing);
			if (lifted_block->outgoing == NULL) {
				u7_lifted_release(out_lifted);
				return U7_ERROR_CAPACITY;
			}
			memcpy(lifted_block->outgoing, lifter.stack + SEED,
					left * sizeof *lifted_block->outgoing);
			lifted_block->outgoing_count = left;
		}
	}
	return U7_OK;
}
