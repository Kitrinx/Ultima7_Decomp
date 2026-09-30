#ifndef U7_STRUCTURE_H
#define U7_STRUCTURE_H

#include "u7/expression.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum U7RegionKind {
	U7_REGION_BLOCK = 0,   /* Straight-line statements. */
	U7_REGION_SEQUENCE,
	U7_REGION_IF,          /* Condition with one arm. */
	U7_REGION_IF_ELSE,
	U7_REGION_WHILE,       /* Test at the top. */
	U7_REGION_DO_WHILE,    /* Test at the bottom. */
	U7_REGION_FOREACH,     /* The array loop pair. */
	U7_REGION_CONVERSE,    /* The conversation loop. */
	U7_REGION_CASE,        /* One answer arm of a conversation. */
	U7_REGION_BREAK,       /* Leaves the enclosing loop. */
	U7_REGION_CONTINUE,    /* Returns to the enclosing loop's test. */
	U7_REGION_GOTO,        /* What could not be structured. */
	U7_REGION_LABEL
} U7RegionKind;

typedef struct U7Region {
	U7RegionKind kind;
	size_t block;              /* Source block, for blocks and gotos. */
	size_t target;             /* Branch target block, for gotos and labels; the
	                            * test block, for a do-while. */
	bool negated;              /* The condition is tested for falsehood. */
	size_t tests;              /* A condition's test blocks from block on; more
	                            * than one are joined by a short-circuit and. */
	U7Expr *condition;
	struct U7Region **children;
	size_t child_count;
} U7Region;

typedef struct U7Structured {
	U7Arena arena;
	U7Region *root;
	size_t goto_count;
	size_t label_count;
	size_t loop_count;
	size_t conditional_count;
	size_t converse_count;
	size_t foreach_count;
	size_t blocks_missing;    /* Reachable blocks the structure never placed. */
	size_t blocks_repeated;   /* Blocks placed twice that are not lone returns. */
	size_t edges_checked;     /* Blocks whose successors the structure implies. */
	size_t edges_misrendered; /* Blocks whose implied successors are wrong. */
	int first_misrendered;    /* The first such block, for inspection. */
	size_t goto_revisit;      /* Target already placed elsewhere. */
	size_t goto_loop_escape;  /* Leaves the enclosing loop elsewhere. */
	size_t goto_unstructured; /* A branch whose arms do not rejoin. */
	size_t break_count;
	size_t continue_count;
	bool complete;             /* Structured with no residual goto. */
} U7Structured;

void u7_structured_release(U7Structured *structured);

/* Per-block analysis results, for inspecting why a shape was not admitted. */
typedef struct U7BlockFacts {
	int idom;
	int ipdom;
	int loop_header;
	int loop_exit;
	bool is_header;
} U7BlockFacts;

U7Result u7_block_facts(const U7Cfg *cfg, U7BlockFacts *out_facts, size_t capacity);

/*
 * Recovers control structures from the graph.
 *
 * Only shapes the graph actually proves are admitted: a loop needs a back edge
 * to a header that dominates it, a conditional needs its two arms to rejoin at
 * its immediate post-dominator. Anything else stays an explicit goto, which is
 * counted rather than hidden.
 */
U7Result u7_structure_function(const U7Function *function, const U7Cfg *cfg,
		const U7Lifted *lifted, U7Structured *out_structured);

#ifdef __cplusplus
}
#endif

#endif
