#include "u7/generate.h"

#include <stdlib.h>
#include <string.h>

/*
 * Output blocks are built in emission order. A jump names a label, and a label
 * is bound to the next block started, so the assembler sees block numbers.
 */
typedef struct Generator {
	const U7Lifted *lifted;
	U7Arena arena;
	U7LiftedBlock *blocks;
	size_t block_count;
	size_t block_capacity;
	bool open;                /* The last block can take more statements. */
	U7Stmt **tail;
	int *labels;              /* Label to output block, or -1 while unbound. */
	size_t label_count;
	size_t label_capacity;
	size_t *pending;          /* Labels waiting for the next block. */
	size_t pending_count;
	size_t pending_capacity;
	int *block_label;         /* Original block to its label, for jumps to it. */
	bool *reachable;          /* Original blocks the entry reaches. */
	U7Stmt **jumps;           /* Every jump emitted, to resolve at the end. */
	size_t jump_count;
	size_t jump_capacity;
	bool failed;
} Generator;

typedef struct Loop {
	size_t top;
	size_t end;
} Loop;

static bool grow(void **array, size_t *capacity, size_t count, size_t size) {
	if (count < *capacity) {
		return true;
	}
	size_t next = *capacity > 0 ? *capacity * 2U : 64U;
	void *grown = realloc(*array, next * size);
	if (grown == NULL) {
		return false;
	}
	*array = grown;
	*capacity = next;
	return true;
}

static size_t new_label(Generator *g) {
	if (!grow((void **) &g->labels, &g->label_capacity, g->label_count, sizeof *g->labels)) {
		g->failed = true;
		return 0;
	}
	g->labels[g->label_count] = -1;
	return g->label_count++;
}

static bool is_bound(const Generator *g, size_t label) {
	if (g->labels[label] >= 0) {
		return true;
	}
	for (size_t i = 0; i < g->pending_count; ++i) {
		if (g->pending[i] == label) {
			return true;
		}
	}
	return false;
}

/* The label marks whatever is emitted next. */
static void bind(Generator *g, size_t label) {
	if (g->failed) {
		return;
	}
	if (g->open && g->blocks[g->block_count - 1U].first == NULL) {
		g->labels[label] = (int) (g->block_count - 1U);
		return;
	}
	g->open = false;
	if (!grow((void **) &g->pending, &g->pending_capacity, g->pending_count, sizeof *g->pending)) {
		g->failed = true;
		return;
	}
	g->pending[g->pending_count++] = label;
}

static void start_block(Generator *g) {
	if (!grow((void **) &g->blocks, &g->block_capacity, g->block_count, sizeof *g->blocks)) {
		g->failed = true;
		return;
	}
	U7LiftedBlock *block = &g->blocks[g->block_count++];
	memset(block, 0, sizeof *block);
	g->tail = &block->first;
	g->open = true;
	for (size_t i = 0; i < g->pending_count; ++i) {
		g->labels[g->pending[i]] = (int) (g->block_count - 1U);
	}
	g->pending_count = 0;
}

static bool ends_block(const U7Stmt *statement) {
	switch (statement->kind) {
		case U7_STMT_BRANCH: case U7_STMT_BRANCH_IF_FALSE: case U7_STMT_ASK:
		case U7_STMT_COMPARE_ANSWERS: case U7_STMT_DEFAULT_ANSWER: case U7_STMT_LOOP:
		case U7_STMT_RETURN: case U7_STMT_ABORT:
			return true;
		default:
			return false;
	}
}

/* A copy of a statement at the end of the current block. */
static U7Stmt *emit(Generator *g, const U7Stmt *statement) {
	if (g->failed) {
		return NULL;
	}
	if (!g->open) {
		start_block(g);
	}
	U7Stmt *copy = u7_arena_alloc(&g->arena, sizeof *copy);
	if (copy == NULL || g->failed) {
		g->failed = true;
		return NULL;
	}
	*copy = *statement;
	copy->next = NULL;
	*g->tail = copy;
	g->tail = &copy->next;
	g->blocks[g->block_count - 1U].statement_count += 1U;
	if (ends_block(copy)) {
		g->open = false;
	}
	return copy;
}

/* A statement that transfers control, aimed at a label. */
static void emit_jump(Generator *g, const U7Stmt *model, size_t label) {
	U7Stmt *copy = emit(g, model);
	if (copy == NULL ||
			!grow((void **) &g->jumps, &g->jump_capacity, g->jump_count, sizeof *g->jumps)) {
		g->failed = true;
		return;
	}
	copy->branch_target = label;
	g->jumps[g->jump_count++] = copy;
}

static void emit_goto(Generator *g, size_t label) {
	U7Stmt jump;
	memset(&jump, 0, sizeof jump);
	jump.kind = U7_STMT_BRANCH;
	jump.opcode = 0x06U;
	emit_jump(g, &jump, label);
}

static bool is_control(const U7Stmt *statement) {
	return statement->kind != U7_STMT_RETURN && statement->kind != U7_STMT_ABORT &&
			ends_block(statement);
}

/* The statement that ends a block's own test, where it has one. */
static const U7Stmt *control_of(const Generator *g, size_t block) {
	const U7Stmt *found = NULL;
	for (const U7Stmt *s = g->lifted->blocks[block].first; s != NULL; s = s->next) {
		if (is_control(s)) {
			found = s;
		}
	}
	return found;
}

/* A block that only jumps. */
static bool lone_jump(const Generator *g, size_t block) {
	const U7Stmt *first = g->lifted->blocks[block].first;
	while (first != NULL && first->kind == U7_STMT_DEBUG) {
		first = first->next;
	}
	return first != NULL && first->kind == U7_STMT_BRANCH && first->next == NULL;
}

/* A block nothing reaches that only jumps: the compiler's jump past an else. */
static bool lone_dead_jump(const Generator *g, size_t block) {
	return !g->reachable[block] && lone_jump(g, block);
}

static size_t label_of_block(Generator *g, size_t block) {
	if (g->block_label[block] < 0) {
		g->block_label[block] = (int) new_label(g);
	}
	return (size_t) g->block_label[block];
}

static void generate(Generator *g, const U7Region *region, const Loop *loop);

static void bind_block(Generator *g, size_t block) {
	if (g->block_label[block] >= 0 && !is_bound(g, (size_t) g->block_label[block])) {
		bind(g, (size_t) g->block_label[block]);
	}
}

static void generate_block(Generator *g, size_t block) {
	bind_block(g, block);
	for (const U7Stmt *s = g->lifted->blocks[block].first; s != NULL; s = s->next) {
		if (!is_control(s)) {
			emit(g, s);
		}
	}
}

static void generate_conditional(Generator *g, const U7Region *region, const Loop *loop) {
	const U7Stmt *test = control_of(g, region->block);
	const U7Region *first = region->child_count > 0 ? region->children[0] : NULL;
	const U7Region *second = region->child_count > 1 ? region->children[1] : NULL;
	if (test == NULL) {
		g->failed = true;
		return;
	}
	size_t after_first = new_label(g);
	if (region->kind == U7_REGION_CASE) {
		/* A key that does not match goes on to the next; a match runs its arm. */
		emit_jump(g, test, after_first);
		generate(g, first, loop);
		bind(g, after_first);
		generate(g, second, loop);
		return;
	}
	size_t end = new_label(g);
	if (!region->negated) {
		emit_jump(g, test, after_first);
		/* The rest of a short-circuit and fail to the same place. */
		for (size_t k = 1; k < region->tests; ++k) {
			size_t block = region->block + k;
			for (const U7Stmt *s = g->lifted->blocks[block].first; s != NULL; s = s->next) {
				emit_jump(g, s, after_first);
			}
		}
		generate(g, first, loop);
		if (region->kind == U7_REGION_IF_ELSE) {
			emit_goto(g, end);
			bind(g, after_first);
			generate(g, second, loop);
		} else {
			bind(g, after_first);
		}
		bind(g, end);
		return;
	}
	/* The arm shown first sits where the test jumps, after the fall-through
	 * arm, which may be empty, and a jump over it. */
	emit_jump(g, test, after_first);
	generate(g, second, loop);
	emit_goto(g, end);
	bind(g, after_first);
	generate(g, first, loop);
	bind(g, end);
}

static void generate_loop(Generator *g, const U7Region *region) {
	Loop loop = {new_label(g), new_label(g)};
	const U7Region *body = region->child_count > 0 ? region->children[0] : NULL;
	bind(g, loop.top);
	if (region->kind == U7_REGION_DO_WHILE) {
		generate(g, body, &loop);
		generate_block(g, region->target);
		emit_jump(g, control_of(g, region->target), loop.end);
		emit_goto(g, loop.top);
		bind(g, loop.end);
		return;
	}
	if (region->kind == U7_REGION_WHILE && region->condition == NULL) {
		/* An untested loop's header is the first block of its own body. */
		generate(g, body, &loop);
		bind(g, loop.end);
		return;
	}
	bind_block(g, region->block);
	const U7Stmt *test = control_of(g, region->block);
	for (const U7Stmt *s = g->lifted->blocks[region->block].first; s != NULL; s = s->next) {
		if (!is_control(s)) {
			emit(g, s);
		}
	}
	if (test == NULL) {
		g->failed = true;
		return;
	}
	emit_jump(g, test, loop.end);
	generate(g, body, &loop);
	bind(g, loop.end);
}

static void generate(Generator *g, const U7Region *region, const Loop *loop) {
	if (region == NULL || g->failed) {
		return;
	}
	switch (region->kind) {
		case U7_REGION_BLOCK:
			generate_block(g, region->block);
			return;
		case U7_REGION_SEQUENCE:
			for (size_t i = 0; i < region->child_count; ++i) {
				const U7Region *child = region->children[i];
				if (child->kind == U7_REGION_BLOCK && lone_dead_jump(g, child->block)) {
					/* Rebuilt by its if/else, along with where it went. */
					if (i + 1U < region->child_count &&
							region->children[i + 1U]->kind == U7_REGION_GOTO) {
						i += 1U;
					}
					continue;
				}
				generate(g, child, loop);
			}
			return;
		case U7_REGION_IF:
		case U7_REGION_IF_ELSE:
		case U7_REGION_CASE:
			generate_conditional(g, region, loop);
			return;
		case U7_REGION_WHILE:
		case U7_REGION_DO_WHILE:
		case U7_REGION_FOREACH:
		case U7_REGION_CONVERSE:
			generate_loop(g, region);
			return;
		case U7_REGION_BREAK:
		case U7_REGION_CONTINUE:
			if (loop == NULL) {
				g->failed = true;
				return;
			}
			emit_goto(g, region->kind == U7_REGION_BREAK ? loop->end : loop->top);
			return;
		case U7_REGION_GOTO:
			emit_goto(g, label_of_block(g, region->target));
			return;
		case U7_REGION_LABEL:
			return;
	}
}

/* Marks what the entry reaches, to tell the compiler's dead jumps apart. */
static bool find_reachable(const U7Cfg *cfg, bool *reachable) {
	size_t *stack = malloc((cfg->block_count + 1U) * sizeof *stack);
	if (stack == NULL) {
		return false;
	}
	size_t top = 0;
	reachable[0] = true;
	stack[top++] = 0;
	while (top > 0) {
		const U7Block *block = &cfg->blocks[stack[--top]];
		int next[2] = {block->taken, block->fallthrough};
		for (int k = 0; k < 2; ++k) {
			if (next[k] >= 0 && (size_t) next[k] < cfg->block_count && !reachable[next[k]]) {
				reachable[next[k]] = true;
				stack[top++] = (size_t) next[k];
			}
		}
	}
	free(stack);
	return true;
}

U7Result u7_generate_function(const U7Function *function, const U7Cfg *cfg,
		const U7Lifted *lifted, const U7Structured *structured,
		uint8_t *out, size_t capacity, U7LowerReport *out_report) {
	if (function == NULL || cfg == NULL || lifted == NULL || structured == NULL ||
			out == NULL || out_report == NULL || structured->root == NULL ||
			lifted->block_count == 0) {
		return U7_ERROR_ARGUMENT;
	}
	memset(out_report, 0, sizeof *out_report);
	Generator g;
	memset(&g, 0, sizeof g);
	g.lifted = lifted;
	g.block_label = malloc(lifted->block_count * sizeof *g.block_label);
	g.reachable = calloc(lifted->block_count, sizeof *g.reachable);
	uint16_t *links = malloc((function->link_count + 1U) * sizeof *links);
	U7Result result = U7_ERROR_CAPACITY;
	if (g.block_label == NULL || g.reachable == NULL || links == NULL ||
			!find_reachable(cfg, g.reachable)) {
		goto done;
	}
	for (size_t b = 0; b < lifted->block_count; ++b) {
		g.block_label[b] = -1;
	}

	/* Any block may be jumped to before it is placed, so each target has a label. */
	for (size_t b = 0; b < cfg->block_count; ++b) {
		if (cfg->blocks[b].taken >= 0 && (size_t) cfg->blocks[b].taken < cfg->block_count) {
			(void) label_of_block(&g, (size_t) cfg->blocks[b].taken);
		}
	}
	generate(&g, structured->root, NULL);
	if (g.pending_count > 0) {
		start_block(&g);
	}
	for (size_t i = 0; !g.failed && i < g.jump_count; ++i) {
		int block = g.labels[g.jumps[i]->branch_target];
		if (block < 0) {
			g.failed = true;      /* A jump to code that was never placed. */
			break;
		}
		g.jumps[i]->branch_target = (size_t) block;
	}
	if (g.failed) {
		result = U7_ERROR_MISMATCH;
		goto done;
	}

	for (size_t i = 0; i < function->link_count; ++i) {
		(void) u7_function_link(function, i, &links[i]);
	}
	U7Unit unit = {
		.function_id = function->function_id,
		.argument_count = function->argument_count,
		.local_count = function->local_count,
		.links = links,
		.link_count = function->link_count,
		.blocks = g.blocks,
		.block_count = g.block_count,
		.block_at_offset = NULL,
	};
	result = u7_assemble_unit(&unit, out, capacity, out_report);
	if (result == U7_OK && out_report->code_size == function->record_size) {
		const uint8_t *original = function->data - 6U;
		size_t where = 0;
		while (where < out_report->code_size && out[where] == original[where]) {
			where += 1U;
		}
		out_report->identical = where == out_report->code_size;
		out_report->first_difference = where;
	}
done:
	u7_arena_release(&g.arena);
	free(g.blocks);
	free(g.labels);
	free(g.pending);
	free(g.block_label);
	free(g.reachable);
	free(g.jumps);
	free(links);
	return result;
}
