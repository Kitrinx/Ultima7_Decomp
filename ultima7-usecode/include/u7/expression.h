#ifndef U7_EXPRESSION_H
#define U7_EXPRESSION_H

#include "u7/stack_flow.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Bump allocator owning one function's lifted nodes. */
typedef struct U7ArenaBlock U7ArenaBlock;
typedef struct U7Arena {
	U7ArenaBlock *head;
} U7Arena;

void *u7_arena_alloc(U7Arena *arena, size_t size);
void u7_arena_release(U7Arena *arena);

typedef enum U7ExprKind {
	U7_EXPR_INCOMING = 0,  /* A value this block was handed by a predecessor. */
	U7_EXPR_CONSTANT,
	U7_EXPR_STRING,
	U7_EXPR_LOCAL,
	U7_EXPR_FLAG,
	U7_EXPR_EVENT,
	U7_EXPR_ITEM,
	U7_EXPR_ELEMENT,       /* One-based subscript of a local. */
	U7_EXPR_UNARY,
	U7_EXPR_BINARY,
	U7_EXPR_ARRAY,
	U7_EXPR_JOIN,          /* Two arrays concatenated. */
	U7_EXPR_CALL,
	U7_EXPR_INTRINSIC
} U7ExprKind;

typedef struct U7Expr {
	U7ExprKind kind;
	uint8_t opcode;        /* The instruction that produced it. */
	size_t origin;         /* That instruction's code offset. */
	int32_t value;         /* Constant, slot, flag, callee or intrinsic. */
	int32_t detail;        /* The instruction's own operand: data offset,
	                        * link index, or intrinsic argument count. */
	const char *text;
	size_t text_length;
	struct U7Expr **operands;
	size_t operand_count;
	/* Debug records that sat immediately before this node's instruction,
	 * chained through their own next pointer. */
	struct U7Stmt *debug;
} U7Expr;

typedef enum U7StmtKind {
	U7_STMT_ASSIGN_LOCAL = 0,
	U7_STMT_ASSIGN_FLAG,
	U7_STMT_ASSIGN_ELEMENT,
	U7_STMT_ASSIGN_EVENT,
	U7_STMT_CALL,          /* A call or intrinsic whose result is discarded. */
	U7_STMT_APPEND,        /* Adds to the message being built. */
	U7_STMT_SAY,
	U7_STMT_RETURN,
	U7_STMT_ABORT,
	U7_STMT_BRANCH,
	U7_STMT_BRANCH_IF_FALSE,
	U7_STMT_ASK,
	U7_STMT_COMPARE_ANSWERS,
	U7_STMT_DEFAULT_ANSWER,
	U7_STMT_LOOP,
	U7_STMT_LOOP_START,
	U7_STMT_DEBUG,         /* A source line or name record, carried through. */
	U7_STMT_DISCARD        /* A value computed and then left on the stack. */
} U7StmtKind;

typedef struct U7Stmt {
	U7StmtKind kind;
	uint8_t opcode;        /* The instruction that ended it. */
	int32_t operands[U7_MAX_OPERANDS];
	size_t offset;         /* Code offset of the instruction that ended it. */
	size_t first_offset;   /* Code offset of its earliest contributing one. */
	int32_t slot;          /* Local slot or flag index, where one applies. */
	size_t branch_target;  /* Code offset, for branching statements. */
	U7Expr *value;         /* Assigned, said, returned or tested value. */
	U7Expr *subscript;     /* Element index, for an element assignment. */
	struct U7Stmt *debug;  /* As above, for this statement's own instruction. */
	struct U7Stmt *next;
} U7Stmt;

typedef struct U7LiftedBlock {
	U7Stmt *first;
	size_t statement_count;
	size_t incoming_count;  /* Values expected from predecessors. */
	size_t outgoing_count;  /* Values left for successors. */
	U7Expr **outgoing;
} U7LiftedBlock;

typedef struct U7Lifted {
	U7Arena arena;
	U7LiftedBlock *blocks;
	size_t block_count;
	bool complete;            /* Every block lifted. */
	bool self_contained;      /* No block consumed an incoming value. */
	size_t consumed_incoming;
	size_t failure_offset;
	uint8_t failure_opcode;
} U7Lifted;

void u7_lifted_release(U7Lifted *lifted);

/*
 * Rebuilds expressions from the stack machine, one block at a time.
 *
 * Each block starts with a symbolic stack of placeholders for whatever its
 * predecessors left, so a block that needs none is recovered exactly. A block
 * that consumes a placeholder is recorded rather than patched over.
 */
U7Result u7_lift_function(const U7Function *function, const U7Cfg *cfg,
		U7CallResolver resolver, void *context, U7Lifted *out_lifted);

#ifdef __cplusplus
}
#endif

#endif
