#include "u7/structure.h"

#include <stdlib.h>
#include <string.h>

typedef struct Graph {
	const U7Cfg *cfg;
	const U7Lifted *lifted;
	size_t count;       /* Blocks, plus one virtual exit at index count. */
	int *rpo;           /* Reverse postorder sequence. */
	int *position;      /* Block index to its place in that sequence. */
	int *idom;
	int *ipdom;
	int *loop_header;
	int *loop_parent;   /* For a header, the innermost loop enclosing it. */
	int *loop_exit;     /* The single block each loop leaves to, or -1. */
	bool *is_header;
	bool *emitted;
	int entering;       /* An untested loop's header, entered as its body. */

	int *pred;          /* Flattened predecessor lists. */
	size_t *pred_start;
	size_t *pred_count;
} Graph;

static void graph_release(Graph *graph) {
	free(graph->rpo);
	free(graph->position);
	free(graph->idom);
	free(graph->ipdom);
	free(graph->loop_header);
	free(graph->loop_parent);
	free(graph->loop_exit);
	free(graph->is_header);
	free(graph->emitted);
	free(graph->pred);
	free(graph->pred_start);
	free(graph->pred_count);
	memset(graph, 0, sizeof *graph);
}

static void successors_of(const U7Cfg *cfg, size_t block, int out[2], size_t *count) {
	*count = 0;
	if (block >= cfg->block_count) {
		return; /* The virtual exit leads nowhere. */
	}
	if (cfg->blocks[block].taken != U7_NO_BLOCK) {
		out[(*count)++] = cfg->blocks[block].taken;
	}
	if (cfg->blocks[block].fallthrough != U7_NO_BLOCK) {
		out[(*count)++] = cfg->blocks[block].fallthrough;
	}
}

/* Every block with no successor flows to the virtual exit instead. */
static void exit_successors(const Graph *graph, size_t block, int out[2],
		size_t *count) {
	successors_of(graph->cfg, block, out, count);
	if (*count == 0 && block < graph->cfg->block_count) {
		out[(*count)++] = (int) graph->cfg->block_count;
	}
}

static bool build_predecessors(Graph *graph) {
	size_t nodes = graph->count;
	graph->pred_count = calloc(nodes, sizeof *graph->pred_count);
	graph->pred_start = calloc(nodes, sizeof *graph->pred_start);
	if (graph->pred_count == NULL || graph->pred_start == NULL) {
		return false;
	}

	size_t total = 0;
	for (size_t b = 0; b < nodes; ++b) {
		int edges[2];
		size_t edge_count = 0;
		exit_successors(graph, b, edges, &edge_count);
		for (size_t e = 0; e < edge_count; ++e) {
			graph->pred_count[edges[e]] += 1U;
			total += 1U;
		}
	}

	graph->pred = malloc((total > 0 ? total : 1U) * sizeof *graph->pred);
	if (graph->pred == NULL) {
		return false;
	}
	size_t at = 0;
	for (size_t b = 0; b < nodes; ++b) {
		graph->pred_start[b] = at;
		at += graph->pred_count[b];
		graph->pred_count[b] = 0;
	}
	for (size_t b = 0; b < nodes; ++b) {
		int edges[2];
		size_t edge_count = 0;
		exit_successors(graph, b, edges, &edge_count);
		for (size_t e = 0; e < edge_count; ++e) {
			int target = edges[e];
			graph->pred[graph->pred_start[target] + graph->pred_count[target]] = (int) b;
			graph->pred_count[target] += 1U;
		}
	}
	return true;
}

/* Iterative depth-first postorder from one root, over a chosen edge set. */
static bool depth_first_order(const Graph *graph, int root, bool reversed,
		int *out_rpo, int *out_position) {
	size_t nodes = graph->count;
	int *stack = malloc(nodes * sizeof *stack);
	size_t *cursor = calloc(nodes, sizeof *cursor);
	bool *seen = calloc(nodes, sizeof *seen);
	int *postorder = malloc(nodes * sizeof *postorder);
	if (stack == NULL || cursor == NULL || seen == NULL || postorder == NULL) {
		free(stack); free(cursor); free(seen); free(postorder);
		return false;
	}

	size_t top = 0;
	size_t produced = 0;
	stack[top++] = root;
	seen[root] = true;

	while (top > 0) {
		int node = stack[top - 1U];
		int edges[2];
		size_t edge_count = 0;
		const int *neighbours = edges;
		if (reversed) {
			neighbours = graph->pred + graph->pred_start[node];
			edge_count = graph->pred_count[node];
		} else {
			exit_successors(graph, (size_t) node, edges, &edge_count);
		}

		if (cursor[node] < edge_count) {
			int next = neighbours[cursor[node]++];
			if (!seen[next]) {
				seen[next] = true;
				stack[top++] = next;
			}
		} else {
			postorder[produced++] = node;
			top -= 1U;
		}
	}

	for (size_t i = 0; i < nodes; ++i) {
		out_position[i] = -1;
	}
	for (size_t i = 0; i < produced; ++i) {
		out_rpo[i] = postorder[produced - 1U - i];
		out_position[out_rpo[i]] = (int) i;
	}
	for (size_t i = produced; i < nodes; ++i) {
		out_rpo[i] = -1;
	}

	free(stack); free(cursor); free(seen); free(postorder);
	return true;
}

static int intersect(const int *idom, const int *position, int a, int b) {
	while (a != b && a >= 0 && b >= 0) {
		while (a >= 0 && b >= 0 && position[a] > position[b]) {
			a = idom[a];
		}
		while (a >= 0 && b >= 0 && position[b] > position[a]) {
			b = idom[b];
		}
	}
	return a >= 0 ? a : b;
}

/*
 * Cooper, Harvey and Kennedy: walk the reverse postorder until the immediate
 * dominators stop moving. Reversed runs it on the predecessor graph from the
 * virtual exit, which gives post-dominators.
 */
static bool compute_dominators(Graph *graph, bool reversed, int *out) {
	size_t nodes = graph->count;
	int *rpo = malloc(nodes * sizeof *rpo);
	int *position = malloc(nodes * sizeof *position);
	if (rpo == NULL || position == NULL) {
		free(rpo); free(position);
		return false;
	}

	int root = reversed ? (int) graph->cfg->block_count : 0;
	if (!depth_first_order(graph, root, reversed, rpo, position)) {
		free(rpo); free(position);
		return false;
	}

	for (size_t i = 0; i < nodes; ++i) {
		out[i] = -1;
	}
	out[root] = root;

	bool changed = true;
	while (changed) {
		changed = false;
		for (size_t i = 0; i < nodes; ++i) {
			int node = rpo[i];
			if (node < 0 || node == root) {
				continue;
			}
			int edges[2];
			size_t edge_count = 0;
			const int *neighbours = edges;
			if (reversed) {
				exit_successors(graph, (size_t) node, edges, &edge_count);
			} else {
				neighbours = graph->pred + graph->pred_start[node];
				edge_count = graph->pred_count[node];
			}

			int candidate = -1;
			for (size_t e = 0; e < edge_count; ++e) {
				int other = neighbours[e];
				if (position[other] < 0 || out[other] < 0) {
					continue;
				}
				candidate = candidate < 0 ? other
						: intersect(out, position, candidate, other);
			}
			if (candidate >= 0 && out[node] != candidate) {
				out[node] = candidate;
				changed = true;
			}
		}
	}

	if (!reversed) {
		memcpy(graph->rpo, rpo, nodes * sizeof *rpo);
		memcpy(graph->position, position, nodes * sizeof *position);
	}
	free(rpo); free(position);
	return true;
}

static bool in_loop(const Graph *graph, int block, int header);

static bool dominates(const int *idom, int ancestor, int block) {
	while (block >= 0) {
		if (block == ancestor) {
			return true;
		}
		int next = idom[block];
		if (next == block) {
			return false;
		}
		block = next;
	}
	return false;
}

/*
 * Natural loops: a back edge is one whose target dominates its source, and the
 * loop body is everything that reaches the latch without passing the header.
 */
static bool find_loops(Graph *graph) {
	size_t nodes = graph->count;
	int *worklist = malloc(nodes * sizeof *worklist);
	bool *member = malloc(nodes * sizeof *member);
	if (worklist == NULL || member == NULL) {
		free(worklist); free(member);
		return false;
	}

	for (size_t i = 0; i < nodes; ++i) {
		graph->loop_header[i] = -1;
		graph->loop_parent[i] = -1;
		graph->is_header[i] = false;
	}

	for (size_t latch = 0; latch < graph->cfg->block_count; ++latch) {
		if (graph->position[latch] < 0) {
			continue;
		}
		int edges[2];
		size_t edge_count = 0;
		successors_of(graph->cfg, latch, edges, &edge_count);
		for (size_t e = 0; e < edge_count; ++e) {
			int header = edges[e];
			if (!dominates(graph->idom, header, (int) latch)) {
				continue;
			}
			graph->is_header[header] = true;

			memset(member, 0, nodes * sizeof *member);
			size_t top = 0;
			member[header] = true;
			if (!member[latch]) {
				member[latch] = true;
				worklist[top++] = (int) latch;
			}
			while (top > 0) {
				int node = worklist[--top];
				for (size_t p = 0; p < graph->pred_count[node]; ++p) {
					int predecessor = graph->pred[graph->pred_start[node] + p];
					if (!member[predecessor]) {
						member[predecessor] = true;
						worklist[top++] = predecessor;
					}
				}
			}

			/* An inner header wins over an outer one, and a nested loop
			 * remembers the loop it sits in so a body can still be told
			 * apart from the code after it. */
			for (size_t m = 0; m < graph->cfg->block_count; ++m) {
				if (!member[m]) {
					continue;
				}
				if (graph->is_header[m] && (int) m != header) {
					int parent = graph->loop_parent[m];
					if (parent < 0 || graph->position[parent] < graph->position[header]) {
						graph->loop_parent[m] = header;
					}
				}
				int current = graph->loop_header[m];
				if (current < 0 || graph->position[current] < graph->position[header]) {
					graph->loop_header[m] = header;
				}
			}
		}
	}

	/*
	 * The compiler emits ask only for converse and the iteration opcode only
	 * for Foreach, so either marks a loop head whether or not anything loops
	 * back. When every pass through the body returns or leaves, the back edge
	 * the compiler still writes is unreachable, and no natural loop is found.
	 * Such a loop's body is everything its body successor dominates.
	 */
	for (size_t h = 0; h < graph->cfg->block_count; ++h) {
		if (graph->is_header[h] || graph->position[h] < 0) {
			continue;
		}
		const U7Block *head = &graph->cfg->blocks[h];
		uint8_t opcode = graph->cfg->instructions[head->first_instruction +
				head->instruction_count - 1U].info->opcode;
		if (opcode != 0x04U && opcode != 0x02U) {
			continue;
		}
		int body = graph->cfg->blocks[h].fallthrough;
		if (body == U7_NO_BLOCK || graph->position[body] < 0) {
			continue;
		}
		int outer = graph->loop_header[h];
		graph->is_header[h] = true;
		graph->loop_parent[h] = outer;
		graph->loop_header[h] = (int) h;
		for (size_t m = 0; m < graph->cfg->block_count; ++m) {
			if (m == h || graph->position[m] < 0 ||
					!dominates(graph->idom, body, (int) m)) {
				continue;
			}
			if (graph->loop_header[m] == outer) {
				graph->loop_header[m] = (int) h;
			} else if (graph->is_header[m] && graph->loop_parent[m] == outer) {
				graph->loop_parent[m] = (int) h;
			}
		}
	}

	/* Where a loop's paths reconverge is where it goes when it ends, so a
	 * jump there is a break however many ways out the loop has. */
	for (size_t header = 0; header < graph->cfg->block_count; ++header) {
		graph->loop_exit[header] = U7_NO_BLOCK;
		if (!graph->is_header[header]) {
			continue;
		}
		/* A loop ends where its own test sends it, and a break goes to the
		 * same place. When the header's test has one edge out of the loop,
		 * that edge is the follow; the guesses below are only for loops
		 * whose header does not test. */
		int edges[2];
		size_t edge_count = 0;
		successors_of(graph->cfg, header, edges, &edge_count);
		if (edge_count == 2U) {
			bool first_in = in_loop(graph, edges[0], (int) header);
			bool second_in = in_loop(graph, edges[1], (int) header);
			if (first_in != second_in) {
				graph->loop_exit[header] = first_in ? edges[1] : edges[0];
				continue;
			}
		}
		int follow = graph->ipdom[header];
		if (follow >= 0 && (size_t) follow < graph->cfg->block_count &&
				!in_loop(graph, follow, (int) header)) {
			graph->loop_exit[header] = follow;
			continue;
		}
		/* Otherwise the way out most edges take is the loop's follow, and
		 * the rarer exits stay explicit jumps. */
		int best = U7_NO_BLOCK;
		size_t best_edges = 0;
		for (size_t candidate = 0; candidate < graph->cfg->block_count; ++candidate) {
			if (in_loop(graph, (int) candidate, (int) header)) {
				continue;
			}
			size_t edges_in = 0;
			for (size_t p = 0; p < graph->pred_count[candidate]; ++p) {
				int source = graph->pred[graph->pred_start[candidate] + p];
				if (in_loop(graph, source, (int) header)) {
					edges_in += 1U;
				}
			}
			if (edges_in > best_edges ||
					(edges_in == best_edges && edges_in > 0 && best >= 0 &&
					 graph->position[candidate] < graph->position[best])) {
				best = (int) candidate;
				best_edges = edges_in;
			}
		}
		graph->loop_exit[header] = best_edges > 0 ? best : U7_NO_BLOCK;
	}

	free(worklist); free(member);
	return true;
}

/* Walks the nest outward: a block inside an inner loop is inside its parents. */
static bool in_loop(const Graph *graph, int block, int header) {
	if (header < 0 || block < 0) {
		return false;
	}
	int current = block == header ? header : graph->loop_header[block];
	while (current >= 0) {
		if (current == header) {
			return true;
		}
		current = graph->loop_parent[current];
	}
	return false;
}

static U7Region *make_region(U7Structured *structured, U7RegionKind kind) {
	U7Region *region = u7_arena_alloc(&structured->arena, sizeof *region);
	if (region != NULL) {
		region->kind = kind;
	}
	return region;
}

static bool add_child(U7Structured *structured, U7Region *parent, U7Region *child) {
	if (parent == NULL || child == NULL) {
		return false;
	}
	U7Region **grown = u7_arena_alloc(&structured->arena,
			(parent->child_count + 1U) * sizeof *grown);
	if (grown == NULL) {
		return false;
	}
	memcpy(grown, parent->children, parent->child_count * sizeof *grown);
	grown[parent->child_count] = child;
	parent->children = grown;
	parent->child_count += 1U;
	return true;
}

/* The statement that ends a block, which carries its branch condition. */
static const U7Stmt *last_statement(const U7Lifted *lifted, size_t block) {
	const U7Stmt *statement = lifted->blocks[block].first;
	const U7Stmt *last = NULL;
	while (statement != NULL) {
		last = statement;
		statement = statement->next;
	}
	return last;
}

static bool is_conversation_head(const U7Lifted *lifted, size_t block) {
	const U7Stmt *last = last_statement(lifted, block);
	return last != NULL && last->kind == U7_STMT_ASK;
}

static bool is_foreach_head(const U7Lifted *lifted, size_t block) {
	const U7Stmt *last = last_statement(lifted, block);
	return last != NULL && last->kind == U7_STMT_LOOP;
}

/*
 * A range ends where it stops. In one the compiler's rules laid out, only a
 * then-arm's jump past its else, from the block before the else, may jump
 * there; any other jump there is a goto. U7_NO_BLOCK accepts any such jump.
 */
enum { NO_ELSE_JUMP = -2 };

static U7Region *structure_range(U7Structured *structured, Graph *graph,
		int block, int stop, int loop_header, int loop_exit, int else_jump);

/* Emits the straight-line part of a block, without its branch. */
static U7Region *emit_block(U7Structured *structured, size_t block) {
	U7Region *region = make_region(structured, U7_REGION_BLOCK);
	if (region != NULL) {
		region->block = block;
	}
	return region;
}

/*
 * A test alone in its block, reached only from the test before it and jumping
 * where that one does: the compiler's short-circuit and.
 *
 *     if a && b {X} else {Y}    a ->E  b ->E  X  jump end  E: Y  end:
 */
static bool chained_test(const Graph *graph, int block, int target) {
	if (block < 0 || (size_t) block >= graph->cfg->block_count ||
			graph->cfg->blocks[block].taken != target || graph->pred_count[block] != 1U ||
			graph->is_header[block] || graph->cfg->blocks[block - 1].fallthrough != block) {
		return false;
	}
	const U7Stmt *first = graph->lifted->blocks[block].first;
	return first != NULL && first->next == NULL && first->kind == U7_STMT_BRANCH_IF_FALSE;
}

/* A jump the structure could not absorb; cause counts why, when it stays one. */
static U7Region *emit_goto(U7Structured *structured, int target, size_t *cause) {
	U7Region *region = make_region(structured, U7_REGION_GOTO);
	if (region != NULL) {
		region->target = (size_t) target;
		structured->goto_count += 1U;
		*cause += 1U;
	}
	return region;
}

/* The block after this one in the original order, while it is still the range's. */
static int next_in_range(const Graph *graph, int block, int stop, int loop_exit) {
	int end = stop != U7_NO_BLOCK ? stop
			: loop_exit != U7_NO_BLOCK ? loop_exit : (int) graph->cfg->block_count;
	int next = block + 1;
	return next < end && !graph->emitted[next] ? next : U7_NO_BLOCK;
}

/* Whether code in [from, to) goes past to other than by break or continue. */
static bool leaves_arm(const Graph *graph, int from, int to, int loop_header, int loop_exit) {
	for (int b = from; b < to; ++b) {
		const U7Block *block = &graph->cfg->blocks[b];
		int next[2] = {block->taken, block->fallthrough};
		for (int k = 0; k < 2; ++k) {
			int t = next[k];
			if (t != U7_NO_BLOCK && (t < from || t > to) && t != loop_exit &&
					t != loop_header) {
				return true;
			}
		}
	}
	return false;
}

/* Whether code in [from, to) also jumps to target, so ends there with it. */
static bool shares_target(const Graph *graph, int from, int to, int target) {
	for (int b = from; b < to; ++b) {
		if (graph->cfg->blocks[b].taken == target) {
			return true;
		}
	}
	return false;
}

/*
 * Code the source wrote inside a loop: its natural body, and whatever the
 * compiler laid between its header and its exit, such as an arm that returns
 * and so never loops back.
 */
static bool inside_loop(const Graph *graph, int block, int header, int exit_block) {
	return in_loop(graph, block, header) ||
			(exit_block > header && block > header && block < exit_block);
}

/* Whether a join lies inside the range and loop being structured. */
static bool fits(const Graph *graph, int join, int stop, int loop_header, int loop_exit) {
	return (stop == U7_NO_BLOCK || join <= stop) &&
			(loop_header < 0 || inside_loop(graph, join, loop_header, loop_exit));
}

/*
 * The compiler lays an if out as its test, the then-arm and any else-arm, in
 * that order, and ends the then-arm with a jump past the else only when there
 * is one:
 *
 *     if c {A}            test ->T  A  T:
 *     if c {A} else {B}   test ->T  A  jump E  T: B  E:
 *
 * So the block just before where the test jumps says which was written, and
 * where it ends, unless code inside the arm also jumps to T: that ends the
 * arm's last statement, and the jump is its own. A jump that leaves the
 * loop is the arm's own break. One that an else could not end at, because it
 * lies outside the enclosing range or live code in between jumps past it, is
 * a return or a goto; a jump nothing reaches is always the compiler's. A jump
 * to the shared return that could end an else reads either way and compiles
 * the same. Gives U7_NO_BLOCK where the layout is not one the compiler makes,
 * leaving dominance to decide.
 */
static int layout_join(const Graph *graph, int block, int last_test, int stop,
		int else_jump, int loop_header, int loop_exit, bool *out_else) {
	int target = graph->cfg->blocks[block].taken;
	*out_else = false;
	/* Inside a then-arm, everything ends by its jump past the else. */
	if (else_jump >= 0) {
		stop = else_jump;
	}
	if (target <= last_test) {
		return U7_NO_BLOCK;
	}
	int join = target;
	int before = target - 1;
	const U7Stmt *last = last_statement(graph->lifted, (size_t) before);
	if (before != last_test && last != NULL && last->kind == U7_STMT_BRANCH) {
		int end = graph->cfg->blocks[before].taken;
		if (end >= target && end != loop_exit && fits(graph, end, stop, loop_header, loop_exit) &&
				(graph->position[before] < 0 ||
				 !leaves_arm(graph, target, end, loop_header, loop_exit)) &&
				!shares_target(graph, last_test + 1, before, target)) {
			join = end;
			*out_else = true;
		}
	}
	if (join != loop_exit && !fits(graph, join, stop, loop_header, loop_exit)) {
		return U7_NO_BLOCK;
	}
	return join;
}

/*
 * Code reached only from inside a loop's body, which then leaves the loop. Its
 * dominators lead back into the body, not through the header: code after the
 * loop is dominated by the header or by the loop's exit instead, and never
 * qualifies, so it is never pulled in.
 */
static bool is_break_tail(const Graph *graph, int block, int header, int exit_block) {
	int current = block;
	while (current >= 0 && (size_t) current < graph->cfg->block_count) {
		if (current == exit_block) {
			return false;
		}
		if (in_loop(graph, current, header)) {
			return current != header;
		}
		int next = graph->idom[current];
		if (next == current) {
			return false;
		}
		current = next;
	}
	return false;
}

/*
 * The one jump back to a loop's header, when it follows a test that leaves
 * the loop: the compiler's do-while.
 *
 *     do {A} while c      top: A  c ->exit  jump top  exit:
 *
 * A header whose own test leaves the loop is a while loop's instead.
 */
static int do_while_latch(const Graph *graph, int header) {
	int latch = U7_NO_BLOCK;
	for (size_t i = 0; i < graph->pred_count[header]; ++i) {
		int from = graph->pred[graph->pred_start[header] + i];
		if (from >= header) {
			if (latch != U7_NO_BLOCK) {
				return U7_NO_BLOCK;
			}
			latch = from;
		}
	}
	if (latch <= header) {
		return U7_NO_BLOCK;
	}
	const U7Stmt *jump = graph->lifted->blocks[latch].first;
	const U7Stmt *test = last_statement(graph->lifted, (size_t) latch - 1U);
	const U7Stmt *head = last_statement(graph->lifted, (size_t) header);
	if (jump == NULL || jump->next != NULL || jump->kind != U7_STMT_BRANCH ||
			test == NULL || test->kind != U7_STMT_BRANCH_IF_FALSE ||
			graph->cfg->blocks[latch - 1].taken != latch + 1 ||
			graph->cfg->blocks[latch - 1].fallthrough != latch) {
		return U7_NO_BLOCK;
	}
	if (header != latch - 1 && head != NULL && head->kind == U7_STMT_BRANCH_IF_FALSE &&
			graph->cfg->blocks[header].taken == latch + 1) {
		return U7_NO_BLOCK;
	}
	return latch;
}

static U7Region *structure_loop(U7Structured *structured, Graph *graph,
		int header, int *out_exit) {
	const U7Block *block = &graph->cfg->blocks[header];
	bool converse = is_conversation_head(graph->lifted, (size_t) header);
	bool iterate = is_foreach_head(graph->lifted, (size_t) header);

	int latch = converse || iterate ? U7_NO_BLOCK : do_while_latch(graph, header);
	if (latch != U7_NO_BLOCK) {
		U7Region *loop = make_region(structured, U7_REGION_DO_WHILE);
		if (loop == NULL) {
			return NULL;
		}
		int test = latch - 1;
		loop->block = (size_t) header;
		loop->target = (size_t) test;
		loop->condition = last_statement(graph->lifted, (size_t) test)->value;
		structured->loop_count += 1U;
		graph->emitted[test] = true;
		graph->emitted[latch] = true;
		/* The body runs from the header up to the test, which the loop owns. */
		graph->entering = header;
		U7Region *body = structure_range(structured, graph, header, test, header,
				latch + 1, U7_NO_BLOCK);
		if (!add_child(structured, loop, body)) {
			return NULL;
		}
		if (out_exit != NULL) {
			*out_exit = latch + 1;
		}
		return loop;
	}

	U7RegionKind kind = converse ? U7_REGION_CONVERSE
			: iterate ? U7_REGION_FOREACH : U7_REGION_WHILE;
	U7Region *loop = make_region(structured, kind);
	if (loop == NULL) {
		return NULL;
	}
	loop->block = (size_t) header;
	structured->loop_count += 1U;
	if (converse) {
		structured->converse_count += 1U;
	}
	if (iterate) {
		structured->foreach_count += 1U;
	}

	/* The header's taken edge leaves the loop; the other edge is the body. */
	int exit_edge = block->taken;
	int body_edge = block->fallthrough;
	if (exit_edge != U7_NO_BLOCK && (size_t) exit_edge >= graph->cfg->block_count) {
		exit_edge = U7_NO_BLOCK;
	}
	if (graph->loop_exit[header] != U7_NO_BLOCK) {
		exit_edge = graph->loop_exit[header];
		if (block->taken == exit_edge) {
			body_edge = block->fallthrough;
		} else if (block->fallthrough == exit_edge) {
			body_edge = block->taken;
		}
	}
	if (exit_edge != U7_NO_BLOCK && in_loop(graph, exit_edge, header)) {
		int swap = exit_edge;
		exit_edge = body_edge;
		body_edge = swap;
	}

	const U7Stmt *last = last_statement(graph->lifted, (size_t) header);
	/* A test is the loop's only when exactly one of its edges leaves. One that
	 * picks between two paths inside the loop is an ordinary conditional at
	 * the top of an untested loop, which the body then carries itself. */
	bool leaves = block->taken != U7_NO_BLOCK && block->fallthrough != U7_NO_BLOCK &&
			in_loop(graph, block->taken, header) !=
			in_loop(graph, block->fallthrough, header);
	if (last != NULL && last->kind == U7_STMT_BRANCH_IF_FALSE && leaves) {
		loop->condition = last->value;
	}

	if (out_exit != NULL) {
		*out_exit = exit_edge;
	}
	if (!converse && !iterate && loop->condition == NULL) {
		graph->entering = header;
		U7Region *body = structure_range(structured, graph, header, U7_NO_BLOCK,
				header, exit_edge, U7_NO_BLOCK);
		if (!add_child(structured, loop, body)) {
			return NULL;
		}
		return loop;
	}
	if (body_edge != U7_NO_BLOCK) {
		/* The body has no stop: it leaves by break, continue or return. */
		U7Region *body = structure_range(structured, graph, body_edge, U7_NO_BLOCK,
				header, exit_edge, U7_NO_BLOCK);
		if (!add_child(structured, loop, body)) {
			return NULL;
		}
	}
	return loop;
}

static U7Region *structure_range(U7Structured *structured, Graph *graph,
		int block, int stop, int loop_header, int loop_exit, int else_jump) {
	U7Region *sequence = make_region(structured, U7_REGION_SEQUENCE);
	if (sequence == NULL) {
		return NULL;
	}

	/* The virtual exit is not a block; reaching it ends the range. */
	while (block != U7_NO_BLOCK && (size_t) block < graph->cfg->block_count) {
		bool entering = block == graph->entering;
		if (entering) {
			graph->entering = U7_NO_BLOCK;
		}
		/* A loop body never falls into the loop's exit; reaching it is a break.
		 * The one exception is an arm of a conditional whose arms both go
		 * there: it stops at that shared join, and whatever continues from the
		 * join says break once for both. */
		if (block == loop_exit && block != stop) {
			U7Region *region = make_region(structured, U7_REGION_BREAK);
			structured->break_count += 1U;
			if (!add_child(structured, sequence, region)) {
				return NULL;
			}
			break;
		}
		if (block == stop) {
			break;
		}
		/* Re-entering the loop's test has its own spelling too. */
		if (block == loop_header && !entering) {
			U7Region *region = make_region(structured, U7_REGION_CONTINUE);
			structured->continue_count += 1U;
			if (!add_child(structured, sequence, region)) {
				return NULL;
			}
			break;
		}
		if (graph->emitted[block] && !entering) {
			/* Already placed elsewhere: only a jump can reach it again. */
			if (!add_child(structured, sequence, emit_goto(structured, block,
					&structured->goto_revisit))) {
				return NULL;
			}
			break;
		}
		/* Leaving the loop this range belongs to is a jump, not a fallthrough,
		 * unless the block is code the source wrote inside the loop ahead of a
		 * break: the compiler lays a break out as a jump to the loop's follow,
		 * so what runs first never loops back and sits outside the natural loop
		 * even though the loop body owns it. */
		if (loop_header >= 0 && !inside_loop(graph, block, loop_header, loop_exit) &&
				!is_break_tail(graph, block, loop_header, loop_exit)) {
			if (!add_child(structured, sequence, emit_goto(structured, block,
					&structured->goto_loop_escape))) {
				return NULL;
			}
			break;
		}

		graph->emitted[block] = true;

		if (graph->is_header[block] && block != loop_header) {
			int after = U7_NO_BLOCK;
			U7Region *loop = structure_loop(structured, graph, block, &after);
			if (!add_child(structured, sequence, loop)) {
				return NULL;
			}
			block = after;
			continue;
		}

		if (!add_child(structured, sequence, emit_block(structured, (size_t) block))) {
			return NULL;
		}

		const U7Block *current = &graph->cfg->blocks[block];
		if (current->taken == U7_NO_BLOCK) {
			/* Code after a return still sits where it was written. */
			block = current->fallthrough != U7_NO_BLOCK ? current->fallthrough
					: next_in_range(graph, block, stop, loop_exit);
			continue;
		}
		if (current->fallthrough == U7_NO_BLOCK) {
			/* An unconditional jump is the end of this range, a break or a
			 * continue; any other is a goto, and what follows it in the
			 * original order still belongs here. */
			int target = current->taken;
			bool ends = target == stop && (else_jump == U7_NO_BLOCK || block == else_jump);
			if (ends || target == block + 1) {
				block = target;
				continue;
			}
			/* A break or continue; what follows still sits where it was written. */
			if (target == loop_exit || (target == loop_header && loop_header >= 0)) {
				bool leaving = target == loop_exit;
				if (!add_child(structured, sequence, make_region(structured,
						leaving ? U7_REGION_BREAK : U7_REGION_CONTINUE))) {
					return NULL;
				}
				if (leaving) {
					structured->break_count += 1U;
				} else {
					structured->continue_count += 1U;
				}
				block = next_in_range(graph, block, stop, loop_exit);
				continue;
			}
			if (!add_child(structured, sequence, emit_goto(structured, target,
					&structured->goto_unstructured))) {
				return NULL;
			}
			block = next_in_range(graph, block, stop, loop_exit);
			continue;
		}

		/* A two-way branch structures only when both arms rejoin. */
		/* Post-dominating only the virtual exit means the arms never rejoin,
		 * which is an if/else where both sides end the function. */
		int join = graph->ipdom[block];
		bool arms_never_rejoin = join >= 0 &&
				(size_t) join >= graph->cfg->block_count;
		bool arm_is_taken = false;
		if (arms_never_rejoin) {
			join = U7_NO_BLOCK;
			/* One arm ending the function hides an ordinary one-armed test:
			 * the side other paths also merge into is where this rejoins. */
			int taken = current->taken;
			int other = current->fallthrough;
			if (graph->pred_count[taken] > 1U && graph->pred_count[other] == 1U) {
				join = taken;
				arms_never_rejoin = false;
			} else if (graph->pred_count[other] > 1U && graph->pred_count[taken] == 1U) {
				join = other;
				arm_is_taken = true;
				arms_never_rejoin = false;
			}
		}
		/* Inside a loop, arms that rejoin only after the loop mean one arm
		 * leaves it. The join is then the arm that stays, and the other ends
		 * in a break when it reaches the loop's exit — otherwise it would stop
		 * at that exit silently and read as though the loop carried on. */
		if (loop_header >= 0 && join >= 0 && !arms_never_rejoin &&
				(size_t) join < graph->cfg->block_count &&
				!in_loop(graph, join, loop_header)) {
			bool taken_stays = in_loop(graph, current->taken, loop_header);
			bool other_stays = in_loop(graph, current->fallthrough, loop_header);
			if (taken_stays && !other_stays) {
				join = current->taken;
				arm_is_taken = false;
			} else if (other_stays && !taken_stays) {
				join = current->fallthrough;
				arm_is_taken = true;
			}
		}
		const U7Stmt *last = last_statement(graph->lifted, (size_t) block);
		/* An answer test and a default reply branch the same way a condition
		 * does, so they structure as arms rather than as jumps. */
		bool branches = last != NULL &&
				(last->kind == U7_STMT_BRANCH_IF_FALSE ||
				 last->kind == U7_STMT_COMPARE_ANSWERS ||
				 last->kind == U7_STMT_DEFAULT_ANSWER);
		int last_test = block;
		if (branches && last->kind == U7_STMT_BRANCH_IF_FALSE) {
			while (chained_test(graph, last_test + 1, current->taken)) {
				last_test += 1;
			}
		}
		bool laid_else = false;
		int laid = branches ? layout_join(graph, block, last_test, stop, else_jump,
				loop_header, loop_exit, &laid_else) : U7_NO_BLOCK;
		if (laid == U7_NO_BLOCK) {
			last_test = block;
		}
		if (laid != U7_NO_BLOCK) {
			join = laid;
			arm_is_taken = false;
			arms_never_rejoin = false;
		}
		if ((join < 0 && !arms_never_rejoin) || join == block || !branches) {
			if (!add_child(structured, sequence, emit_goto(structured,
					current->taken, &structured->goto_unstructured))) {
				return NULL;
			}
			block = current->fallthrough;
			continue;
		}

		bool has_else = laid != U7_NO_BLOCK ? laid_else : arms_never_rejoin ||
				(arm_is_taken ? current->fallthrough != join : current->taken != join);
		bool is_case = last->kind != U7_STMT_BRANCH_IF_FALSE;
		U7Region *conditional = make_region(structured,
				is_case ? U7_REGION_CASE : has_else ? U7_REGION_IF_ELSE : U7_REGION_IF);
		if (conditional == NULL) {
			return NULL;
		}
		conditional->condition = last->value;
		/* A branch-if-false falls through when its test holds, so the arm
		 * shown first is the true one unless the arms were swapped. */
		conditional->negated = !is_case && arm_is_taken;
		conditional->block = (size_t) block;
		conditional->tests = (size_t) (last_test - block) + 1U;
		for (int b = block + 1; b <= last_test; ++b) {
			graph->emitted[b] = true;
		}
		structured->conditional_count += 1U;

		int then_edge = arm_is_taken ? current->taken
				: graph->cfg->blocks[last_test].fallthrough;
		int else_edge = arm_is_taken ? current->fallthrough : current->taken;
		/* Arms that never rejoin each other can still reach the point where the
		 * enclosing range ends, and must stop there rather than run past it. */
		int arm_stop = join != U7_NO_BLOCK ? join : stop;
		int then_jump = laid == U7_NO_BLOCK ? U7_NO_BLOCK
				: has_else ? current->taken - 1 : NO_ELSE_JUMP;
		U7Region *then_arm = structure_range(structured, graph, then_edge, arm_stop,
				loop_header, loop_exit, then_jump);
		if (!add_child(structured, conditional, then_arm)) {
			return NULL;
		}
		if (has_else) {
			U7Region *else_arm = structure_range(structured, graph, else_edge,
					arm_stop, loop_header, loop_exit,
					laid == U7_NO_BLOCK ? U7_NO_BLOCK : NO_ELSE_JUMP);
			if (!add_child(structured, conditional, else_arm)) {
				return NULL;
			}
		}
		if (!add_child(structured, sequence, conditional)) {
			return NULL;
		}
		block = join;
	}

	return sequence;
}

static U7Result build_graph(const U7Cfg *cfg, Graph *graph);

U7Result u7_block_facts(const U7Cfg *cfg, U7BlockFacts *out_facts, size_t capacity) {
	if (cfg == NULL || out_facts == NULL || capacity < cfg->block_count) {
		return U7_ERROR_ARGUMENT;
	}
	Graph graph;
	U7Result result = build_graph(cfg, &graph);
	if (result != U7_OK) {
		return result;
	}
	for (size_t b = 0; b < cfg->block_count; ++b) {
		out_facts[b].idom = graph.idom[b];
		out_facts[b].ipdom = graph.ipdom[b];
		out_facts[b].loop_header = graph.loop_header[b];
		out_facts[b].loop_exit = graph.loop_exit[b];
		out_facts[b].is_header = graph.is_header[b];
	}
	graph_release(&graph);
	return U7_OK;
}

static U7Result build_graph(const U7Cfg *cfg, Graph *graph) {
	memset(graph, 0, sizeof *graph);
	graph->cfg = cfg;
	graph->count = cfg->block_count + 1U;   /* Plus the virtual exit. */
	graph->rpo = malloc(graph->count * sizeof *graph->rpo);
	graph->position = malloc(graph->count * sizeof *graph->position);
	graph->idom = malloc(graph->count * sizeof *graph->idom);
	graph->ipdom = malloc(graph->count * sizeof *graph->ipdom);
	graph->loop_header = malloc(graph->count * sizeof *graph->loop_header);
	graph->loop_parent = malloc(graph->count * sizeof *graph->loop_parent);
	graph->loop_exit = malloc(graph->count * sizeof *graph->loop_exit);
	graph->is_header = malloc(graph->count * sizeof *graph->is_header);
	graph->emitted = calloc(graph->count, sizeof *graph->emitted);
	graph->entering = U7_NO_BLOCK;
	if (graph->rpo == NULL || graph->position == NULL || graph->idom == NULL ||
			graph->ipdom == NULL || graph->loop_header == NULL ||
			graph->loop_exit == NULL || graph->loop_parent == NULL ||
			graph->is_header == NULL || graph->emitted == NULL ||
			!build_predecessors(graph) ||
			!compute_dominators(graph, false, graph->idom) ||
			!compute_dominators(graph, true, graph->ipdom) ||
			!find_loops(graph)) {
		graph_release(graph);
		return U7_ERROR_CAPACITY;
	}
	return U7_OK;
}

/* Counts how many times each block was placed, as a check on the result. */
static void count_placements(const U7Region *region, size_t *counts, size_t limit) {
	if (region == NULL) {
		return;
	}
	/* A loop is placed through its header block, as a plain block is, unless
	 * its body starts at that header and places it there instead. */
	bool loop = region->kind == U7_REGION_WHILE || region->kind == U7_REGION_DO_WHILE ||
			region->kind == U7_REGION_FOREACH || region->kind == U7_REGION_CONVERSE;
	bool body_owns_header = loop && ((region->kind == U7_REGION_WHILE &&
			region->condition == NULL) || region->kind == U7_REGION_DO_WHILE);
	if (region->kind == U7_REGION_DO_WHILE && region->target + 1U < limit) {
		counts[region->target] += 1U;
		counts[region->target + 1U] += 1U;
	}
	if ((region->kind == U7_REGION_BLOCK || (loop && !body_owns_header)) &&
			region->block < limit) {
		counts[region->block] += 1U;
	}
	for (size_t k = 1; k < region->tests && region->block + k < limit; ++k) {
		counts[region->block + k] += 1U;
	}
	for (size_t i = 0; i < region->child_count; ++i) {
		count_placements(region->children[i], counts, limit);
	}
}


/*
 * Checks the structure against the graph it came from. Reading the structure
 * back as a graph — where each block goes next, given the sequence, the
 * conditional, the loop, break and continue around it — must give exactly the
 * successors the block really has. A structure that looks tidy but routes
 * control somewhere else is caught here rather than by a reader.
 */
typedef struct Implied {
	int successor[4];
	size_t count;
	bool seen;
} Implied;

static void imply(Implied *table, size_t limit, int block, int target) {
	if (block < 0 || (size_t) block >= limit) {
		return;
	}
	Implied *entry = &table[block];
	entry->seen = true;
	if (target < 0) {
		return;
	}
	for (size_t i = 0; i < entry->count; ++i) {
		if (entry->successor[i] == target) {
			return;
		}
	}
	if (entry->count < 4U) {
		entry->successor[entry->count++] = target;
	}
}

static bool is_conditional(const U7Region *region) {
	return region != NULL && (region->kind == U7_REGION_IF ||
			region->kind == U7_REGION_IF_ELSE || region->kind == U7_REGION_CASE);
}

static bool is_loop(const U7Region *region) {
	return region != NULL && (region->kind == U7_REGION_WHILE ||
			region->kind == U7_REGION_DO_WHILE || region->kind == U7_REGION_FOREACH ||
			region->kind == U7_REGION_CONVERSE);
}

static int entry_of(const U7Region *region, int next, int follow, int header);

/* The block control reaches on entering children[from..] of a sequence. */
static int entry_of_rest(const U7Region *sequence, size_t from, int next,
		int follow, int header) {
	for (size_t i = from; i < sequence->child_count; ++i) {
		const U7Region *child = sequence->children[i];
		if (child->kind == U7_REGION_LABEL) {
			continue;
		}
		int after = entry_of_rest(sequence, i + 1U, next, follow, header);
		return entry_of(child, after, follow, header);
	}
	return next;
}

static int entry_of(const U7Region *region, int next, int follow, int header) {
	if (region == NULL) {
		return next;
	}
	switch (region->kind) {
		case U7_REGION_BLOCK:
			return (int) region->block;
		case U7_REGION_SEQUENCE:
			return entry_of_rest(region, 0, next, follow, header);
		case U7_REGION_BREAK:
			return follow;
		case U7_REGION_CONTINUE:
			return header;
		case U7_REGION_GOTO:
			return (int) region->target;
		case U7_REGION_LABEL:
			return next;
		default:
			if (is_loop(region)) {
				return (int) region->block;
			}
			return next;   /* A conditional runs from its test block, placed before it. */
	}
}

static bool ends_in_branch(const U7Lifted *lifted, size_t block) {
	const U7Stmt *last = last_statement(lifted, block);
	return last != NULL && (last->kind == U7_STMT_BRANCH_IF_FALSE ||
			last->kind == U7_STMT_COMPARE_ANSWERS ||
			last->kind == U7_STMT_DEFAULT_ANSWER || last->kind == U7_STMT_ASK ||
			last->kind == U7_STMT_LOOP || last->kind == U7_STMT_BRANCH);
}

static bool ends_in_terminator(const U7Lifted *lifted, size_t block) {
	const U7Stmt *last = last_statement(lifted, block);
	return last != NULL && (last->kind == U7_STMT_RETURN || last->kind == U7_STMT_ABORT);
}

static void walk_implied(const U7Region *region, const U7Lifted *lifted,
		Implied *table, size_t limit, int next, int follow, int header);

static void walk_sequence(const U7Region *sequence, const U7Lifted *lifted,
		Implied *table, size_t limit, int next, int follow, int header) {
	for (size_t i = 0; i < sequence->child_count; ++i) {
		const U7Region *child = sequence->children[i];
		int after = entry_of_rest(sequence, i + 1U, next, follow, header);

		if (child->kind == U7_REGION_BLOCK) {
			size_t block = child->block;
			/* A test block's successors belong to the conditional after it. */
			const U7Region *following = i + 1U < sequence->child_count
					? sequence->children[i + 1U] : NULL;
			if (is_conditional(following) && following->block == block) {
				continue;
			}
			if (ends_in_terminator(lifted, block)) {
				imply(table, limit, (int) block, -1);
			} else if (ends_in_branch(lifted, block) &&
					last_statement(lifted, block)->kind != U7_STMT_BRANCH) {
				/* A two-way test with no conditional to own it: not implied. */
				imply(table, limit, (int) block, after);
			} else {
				imply(table, limit, (int) block, after);
			}
			continue;
		}
		if (is_conditional(child)) {
			int yes = entry_of(child->child_count > 0 ? child->children[0] : NULL,
					after, follow, header);
			int no = child->child_count > 1
					? entry_of(child->children[1], after, follow, header) : after;
			/* Each test of a chain fails to the same place and passes to the next. */
			size_t tests = child->tests > 1U ? child->tests : 1U;
			for (size_t k = 0; k < tests; ++k) {
				int test = (int) (child->block + k);
				imply(table, limit, test, k + 1U < tests ? test + 1 : yes);
				imply(table, limit, test, no);
			}
			for (size_t k = 0; k < child->child_count; ++k) {
				walk_implied(child->children[k], lifted, table, limit, after, follow, header);
			}
			continue;
		}
		walk_implied(child, lifted, table, limit, after, follow, header);
	}
}

static void walk_implied(const U7Region *region, const U7Lifted *lifted,
		Implied *table, size_t limit, int next, int follow, int header) {
	if (region == NULL) {
		return;
	}
	if (region->kind == U7_REGION_SEQUENCE) {
		walk_sequence(region, lifted, table, limit, next, follow, header);
		return;
	}
	if (region->kind == U7_REGION_DO_WHILE) {
		/* The body falls into the test, which leaves or jumps back to the top. */
		int test = (int) region->target;
		imply(table, limit, test, test + 1);
		imply(table, limit, test, next);
		imply(table, limit, test + 1, (int) region->block);
		if (region->child_count > 0) {
			walk_implied(region->children[0], lifted, table, limit, test, next,
					(int) region->block);
		}
		return;
	}
	if (is_loop(region)) {
		int loop_header = (int) region->block;
		/* The header runs the test: into the body, or on to what follows. */
		int body = entry_of(region->child_count > 0 ? region->children[0] : NULL,
				loop_header, next, loop_header);
		/* A body that starts at the header carries the header's own edges. */
		if (body != loop_header) {
			imply(table, limit, loop_header, body);
			/* Only a header that tests can leave the loop on its own. */
			if (region->condition != NULL || region->kind != U7_REGION_WHILE) {
				imply(table, limit, loop_header, next);
			}
		}
		if (region->child_count > 0) {
			walk_implied(region->children[0], lifted, table, limit, loop_header, next,
					loop_header);
		}
		return;
	}
	if (region->kind == U7_REGION_BLOCK) {
		if (ends_in_terminator(lifted, region->block)) {
			imply(table, limit, (int) region->block, -1);
		} else {
			imply(table, limit, (int) region->block, next);
		}
	}
}

static void verify_structure(const U7Cfg *cfg, const U7Lifted *lifted,
		const Graph *graph, U7Structured *out) {
	size_t limit = cfg->block_count;
	Implied *table = calloc(limit, sizeof *table);
	if (table == NULL) {
		return;
	}
	walk_implied(out->root, lifted, table, limit, -1, -1, -1);

	for (size_t b = 0; b < limit; ++b) {
		if (graph->position[b] < 0 || !table[b].seen) {
			continue;
		}
		int real[2];
		size_t real_count = 0;
		successors_of(cfg, b, real, &real_count);
		out->edges_checked += 1U;

		bool same = real_count == table[b].count;
		for (size_t i = 0; same && i < real_count; ++i) {
			bool found = false;
			for (size_t j = 0; j < table[b].count; ++j) {
				found = found || table[b].successor[j] == real[i];
			}
			same = found;
		}
		if (!same) {
			if (out->edges_misrendered == 0) {
				out->first_misrendered = (int) b;
			}
			out->edges_misrendered += 1U;
		}
	}
	free(table);
}

void u7_structured_release(U7Structured *structured) {
	if (structured == NULL) {
		return;
	}
	u7_arena_release(&structured->arena);
	memset(structured, 0, sizeof *structured);
}

U7Result u7_structure_function(const U7Function *function, const U7Cfg *cfg,
		const U7Lifted *lifted, U7Structured *out_structured) {
	(void) function;
	if (cfg == NULL || lifted == NULL || out_structured == NULL) {
		return U7_ERROR_ARGUMENT;
	}
	memset(out_structured, 0, sizeof *out_structured);
	if (cfg->block_count == 0) {
		out_structured->complete = true;
		return U7_OK;
	}

	Graph graph;
	U7Result setup = build_graph(cfg, &graph);
	if (setup != U7_OK) {
		return setup;
	}
	graph.lifted = lifted;

	U7Region *root = structure_range(out_structured, &graph, 0, U7_NO_BLOCK,
			U7_NO_BLOCK, U7_NO_BLOCK, U7_NO_BLOCK);

	/* Anything the traversal could not reach still belongs in the output, so
	 * it is placed after it, each under the label its jumps name. */
	for (size_t b = 0; root != NULL && b < cfg->block_count; ++b) {
		if (graph.emitted[b]) {
			continue;
		}
		U7Region *label = make_region(out_structured, U7_REGION_LABEL);
		if (label == NULL || !add_child(out_structured, root, label)) {
			root = NULL;
			break;
		}
		label->target = b;
		out_structured->label_count += 1U;
		U7Region *rest = structure_range(out_structured, &graph, (int) b,
				U7_NO_BLOCK, U7_NO_BLOCK, U7_NO_BLOCK, U7_NO_BLOCK);
		if (!add_child(out_structured, root, rest)) {
			root = NULL;
			break;
		}
	}
	out_structured->root = root;

	/* Every reachable block must appear, once; anything else means the
	 * structure lost or copied code. */
	size_t *placed = calloc(cfg->block_count, sizeof *placed);
	if (placed != NULL && root != NULL) {
		count_placements(root, placed, cfg->block_count);
		for (size_t b = 0; b < cfg->block_count; ++b) {
			bool reachable = graph.position[b] >= 0;
			if (reachable && placed[b] == 0) {
				out_structured->blocks_missing += 1U;
			} else if (placed[b] > 1U) {
				out_structured->blocks_repeated += 1U;
			}
		}
	}
	free(placed);

	if (root != NULL) {
		verify_structure(cfg, lifted, &graph, out_structured);
	}
	out_structured->complete = out_structured->root != NULL &&
			out_structured->goto_count == 0;

	U7Result result = out_structured->root != NULL ? U7_OK : U7_ERROR_CAPACITY;
	graph_release(&graph);
	return result;
}
