#include "u7/debug_records.h"
#include "u7/structure.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Corpus {
	uint8_t *bytes;
	size_t size;
	U7Function *functions;
	bool *returns_value;
	size_t count;
} Corpus;

static bool scan_returns_value(const U7Function *function) {
	size_t at = 0;
	while (at < function->code_size) {
		U7Instruction instruction;
		if (u7_decode_instruction(function->code, function->code_size, at,
				&instruction) != U7_OK) {
			return false;
		}
		if (instruction.info->opcode == 0x2DU || instruction.info->opcode == 0x32U) {
			return true;
		}
		at += instruction.encoded_size;
	}
	return false;
}

static bool resolve(void *context, uint16_t function_id, U7CallTarget *out_target) {
	const Corpus *corpus = context;
	for (size_t i = 0; i < corpus->count; ++i) {
		if (corpus->functions[i].function_id != function_id) {
			continue;
		}
		out_target->argument_count = corpus->functions[i].argument_count;
		out_target->returns_value = corpus->returns_value[i];
		return true;
	}
	return false;
}

static void corpus_release(Corpus *corpus) {
	free(corpus->bytes);
	free(corpus->functions);
	free(corpus->returns_value);
	memset(corpus, 0, sizeof *corpus);
}

static int corpus_load(const char *path, Corpus *out_corpus) {
	memset(out_corpus, 0, sizeof *out_corpus);
	if (u7_read_file(path, &out_corpus->bytes, &out_corpus->size) != U7_OK) {
		fprintf(stderr, "%s: cannot read\n", path);
		return 1;
	}
	size_t capacity = 2048;
	out_corpus->functions = malloc(capacity * sizeof *out_corpus->functions);
	out_corpus->returns_value = malloc(capacity * sizeof *out_corpus->returns_value);
	if (out_corpus->functions == NULL || out_corpus->returns_value == NULL) {
		corpus_release(out_corpus);
		return 1;
	}
	size_t offset = 0;
	while (offset < out_corpus->size) {
		if (out_corpus->count == capacity) {
			capacity *= 2U;
			U7Function *grown = realloc(out_corpus->functions, capacity * sizeof *grown);
			bool *grown_flags = realloc(out_corpus->returns_value,
					capacity * sizeof *grown_flags);
			if (grown == NULL || grown_flags == NULL) {
				free(grown);
				free(grown_flags);
				corpus_release(out_corpus);
				return 1;
			}
			out_corpus->functions = grown;
			out_corpus->returns_value = grown_flags;
		}
		if (u7_parse_function(out_corpus->bytes, out_corpus->size, offset,
				&out_corpus->functions[out_corpus->count]) != U7_OK) {
			corpus_release(out_corpus);
			return 1;
		}
		out_corpus->returns_value[out_corpus->count] =
				scan_returns_value(&out_corpus->functions[out_corpus->count]);
		offset = out_corpus->functions[out_corpus->count].next_offset;
		out_corpus->count += 1U;
	}
	return 0;
}

static void print_operands(const U7Function *function,
		const U7Instruction *instruction) {
	for (size_t i = 0; i < instruction->info->operand_count; ++i) {
		int32_t value = 0;
		if (u7_instruction_operand(instruction, i, &value) != U7_OK) {
			printf(" ?");
			continue;
		}
		switch (instruction->info->operands[i].kind) {
			case U7_OPERAND_RELATIVE_JUMP:
				printf(" ->%04zX", (size_t) ((int64_t) instruction->offset +
						instruction->encoded_size + value));
				break;
			case U7_OPERAND_DATA_OFFSET: {
				const char *text = NULL;
				size_t length = 0;
				if (u7_function_string(function, (size_t) value, &text, &length) == U7_OK) {
					printf(" \"%.*s\"", length > 40U ? 40 : (int) length, text);
				} else {
					printf(" data:%d", value);
				}
				break;
			}
			case U7_OPERAND_LINK_INDEX: {
				uint16_t target = 0;
				if (u7_function_link(function, (size_t) value, &target) == U7_OK) {
					printf(" fn:%04X", target);
				} else {
					printf(" link:%d", value);
				}
				break;
			}
			case U7_OPERAND_LOCAL_INDEX:
				printf(" L%d", value);
				break;
			default:
				printf(" %d", value);
				break;
		}
	}
}

static void print_expr(const U7Expr *expr);
static void print_local(int slot);

static char g_intrinsic_names[256][48];

/* Optional sidecar of intrinsic names, keyed by game and ordinal. */
static bool use_origin;

static void load_intrinsic_names(const char *path, const char *game) {
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		return;
	}
	char line[160];
	while (fgets(line, sizeof line, file) != NULL) {
		char which[8];
		unsigned ordinal = 0;
		char arity[16];
		char result[16];
		char name[48];
		char origin[48] = "-";
		int fields = sscanf(line, "%7s %x %15s %15s %47s %47s", which, &ordinal,
				arity, result, name, origin);
		if (fields < 5 || name[0] == '-') {
			continue;
		}
		/* A candidate original name is used only when asked for, because the
		 * mapping onto this ordinal is inferred rather than proved. */
		if (use_origin && fields == 6 && origin[0] != '-') {
			snprintf(name, sizeof name, "%s", origin);
		}
		if (ordinal < 256U && strcmp(which, game) == 0) {
			snprintf(g_intrinsic_names[ordinal], sizeof g_intrinsic_names[0], "%s", name);
		}
	}
	fclose(file);
}

static const char *binary_name(uint8_t opcode) {
	switch (opcode) {
		case 0x09U: return "+";
		case 0x0AU: return "-";
		case 0x0BU: return "/";
		case 0x0CU: return "*";
		case 0x0DU: return "%";
		case 0x0EU: return "&&";
		case 0x0FU: return "||";
		case 0x16U: return ">";
		case 0x17U: return "<";
		case 0x18U: return ">=";
		case 0x19U: return "<=";
		case 0x1AU: return "!=";
		case 0x22U: return "==";
		case 0x30U: return "in";
		default: return "?";
	}
}

static void print_expr(const U7Expr *expr) {
	if (expr == NULL) {
		printf("<none>");
		return;
	}
	switch (expr->kind) {
		case U7_EXPR_INCOMING: printf("<incoming %d>", expr->value); break;
		case U7_EXPR_CONSTANT: printf("%d", expr->value); break;
		case U7_EXPR_STRING:
			printf("\"%.*s\"", expr->text_length > 30U ? 30 : (int) expr->text_length,
					expr->text);
			break;
		case U7_EXPR_LOCAL: print_local(expr->value); break;
		case U7_EXPR_FLAG: printf("flag[%d]", expr->value); break;
		case U7_EXPR_EVENT: printf("event"); break;
		case U7_EXPR_ITEM: printf("item"); break;
		case U7_EXPR_ELEMENT:
			print_local(expr->value);
			printf("[");
			print_expr(expr->operands[0]);
			printf("]");
			break;
		case U7_EXPR_UNARY:
			printf("!");
			print_expr(expr->operands[0]);
			break;
		case U7_EXPR_BINARY:
			printf("(");
			print_expr(expr->operands[0]);
			printf(" %s ", binary_name(expr->opcode));
			print_expr(expr->operands[1]);
			printf(")");
			break;
		case U7_EXPR_JOIN:
			print_expr(expr->operands[0]);
			printf(" & ");
			print_expr(expr->operands[1]);
			break;
		case U7_EXPR_ARRAY:
		case U7_EXPR_CALL:
		case U7_EXPR_INTRINSIC:
			if (expr->kind == U7_EXPR_CALL) {
				printf("fn%04X", (unsigned) expr->value);
			} else if (expr->kind == U7_EXPR_INTRINSIC) {
				unsigned ordinal = (unsigned) expr->value & 0xFFU;
				if (g_intrinsic_names[ordinal][0] != 0) {
					printf("%s", g_intrinsic_names[ordinal]);
				} else {
					printf("intrinsic%02X", ordinal);
				}
			}
			printf(expr->kind == U7_EXPR_ARRAY ? "[" : "(");
			for (size_t i = 0; i < expr->operand_count; ++i) {
				if (i > 0) {
					printf(", ");
				}
				print_expr(expr->operands[i]);
			}
			printf(expr->kind == U7_EXPR_ARRAY ? "]" : ")");
			break;
	}
}

static void indent(int depth) {
	for (int i = 0; i < depth; ++i) {
		printf("    ");
	}
}

static const U7DebugLine *g_lines;
static size_t g_line_count;
static const U7Function *g_function;
static const U7DebugFunction *g_debug;

/* The original name of a local slot, when the corpus carries one. */
static void print_local(int slot) {
	if (g_debug != NULL && slot >= 0 && slot < (int) g_debug->local_count) {
		const char *text = NULL;
		size_t length = 0;
		if (u7_debug_local_name(g_function, g_debug, (uint16_t) slot, &text,
				&length) == U7_OK && length > 0) {
			printf("%.*s", (int) length, text);
			return;
		}
	}
	printf("L%d", slot);
}

/* The original source line the given code offset belongs to, or zero. */
static unsigned line_at(size_t offset) {
	unsigned line = 0;
	for (size_t i = 0; i < g_line_count; ++i) {
		if (g_lines[i].code_offset <= offset) {
			line = g_lines[i].line;
		}
	}
	return line;
}

static void print_statements(const U7Stmt *statement, int depth) {
	while (statement != NULL) {
		/* Branches are shown by the structure that encloses them. */
		if (statement->kind == U7_STMT_BRANCH ||
				statement->kind == U7_STMT_BRANCH_IF_FALSE ||
				statement->kind == U7_STMT_COMPARE_ANSWERS ||
				statement->kind == U7_STMT_DEFAULT_ANSWER ||
				statement->kind == U7_STMT_ASK ||
				statement->kind == U7_STMT_LOOP ||
				statement->kind == U7_STMT_LOOP_START) {
			statement = statement->next;
			continue;
		}
		indent(depth);
		if (g_line_count > 0) {
			unsigned first = line_at(statement->first_offset);
			unsigned last = line_at(statement->offset);
			if (first == last) {
				printf("/* %u */ ", first);
			} else {
				printf("/* %u-%u */ ", first, last);
			}
		}
		switch (statement->kind) {
			case U7_STMT_ASSIGN_LOCAL:
				print_local(statement->slot); printf(" = ");
				print_expr(statement->value); printf(";");
				break;
			case U7_STMT_ASSIGN_FLAG:
				printf("flag[%d] = ", statement->slot); print_expr(statement->value); printf(";");
				break;
			case U7_STMT_ASSIGN_ELEMENT:
				print_local(statement->slot); printf("[");
				print_expr(statement->subscript);
				printf("] = "); print_expr(statement->value); printf(";");
				break;
			case U7_STMT_ASSIGN_EVENT:
				printf("event = "); print_expr(statement->value); printf(";");
				break;
			case U7_STMT_CALL: print_expr(statement->value); printf(";"); break;
			case U7_STMT_APPEND: printf("append "); print_expr(statement->value); printf(";"); break;
			case U7_STMT_SAY: printf("say;"); break;
			case U7_STMT_RETURN:
				printf("return");
				if (statement->value != NULL) { printf(" "); print_expr(statement->value); }
				printf(";");
				break;
			case U7_STMT_ABORT: printf("terminate;"); break;
			case U7_STMT_BRANCH: printf("/* jump */"); break;
			case U7_STMT_BRANCH_IF_FALSE: printf("/* test */"); break;
			case U7_STMT_ASK: printf("/* ask */"); break;
			case U7_STMT_COMPARE_ANSWERS: printf("/* answer test */"); break;
			case U7_STMT_DEFAULT_ANSWER: printf("/* default */"); break;
			case U7_STMT_LOOP: printf("/* iterate */"); break;
			case U7_STMT_LOOP_START: printf("/* loop setup */"); break;
			case U7_STMT_DEBUG: printf("/* debug record */"); break;
			case U7_STMT_DISCARD: printf("/* value left on the stack */"); break;
		}
		printf("\n");
		statement = statement->next;
	}
}

static void print_region(const U7Region *region, const U7Lifted *lifted, int depth) {
	if (region == NULL) {
		return;
	}
	switch (region->kind) {
		case U7_REGION_BLOCK:
			print_statements(lifted->blocks[region->block].first, depth);
			return;
		case U7_REGION_SEQUENCE:
			for (size_t i = 0; i < region->child_count; ++i) {
				print_region(region->children[i], lifted, depth);
			}
			return;
		case U7_REGION_GOTO:
			indent(depth); printf("goto block%zu;\n", region->target);
			return;
		case U7_REGION_BREAK:
			indent(depth); printf("break;\n");
			return;
		case U7_REGION_CONTINUE:
			indent(depth); printf("continue;\n");
			return;
		case U7_REGION_LABEL:
			indent(depth); printf("block%zu:\n", region->target);
			return;
		default:
			break;
	}

	indent(depth);
	switch (region->kind) {
		case U7_REGION_IF:
		case U7_REGION_IF_ELSE:
			printf("if ("); print_expr(region->condition); printf(") {\n");
			break;
		case U7_REGION_WHILE:
			printf("while ("); print_expr(region->condition); printf(") {\n");
			break;
		case U7_REGION_DO_WHILE: printf("do {\n"); break;
		case U7_REGION_FOREACH: printf("foreach {\n"); break;
		case U7_REGION_CONVERSE: printf("converse {\n"); break;
		case U7_REGION_CASE:
			printf("case "); print_expr(region->condition); printf(" {\n");
			break;
		default: printf("{\n"); break;
	}
	if (region->child_count > 0) {
		print_region(region->children[0], lifted, depth + 1);
	}
	indent(depth);
	if (region->child_count > 1) {
		printf("} else {\n");
		print_region(region->children[1], lifted, depth + 1);
		indent(depth);
	}
	printf("}\n");
}

int main(int argc, char **argv) {
	const char *names_path = getenv("U7_INTRINSIC_NAMES");
	const char *game = getenv("U7_GAME");
	use_origin = getenv("U7_ORIGIN_NAMES") != NULL;
	if (names_path != NULL) {
		load_intrinsic_names(names_path, game != NULL ? game : "bg");
	}

	bool source = argc > 1 && strcmp(argv[1], "--source") == 0;
	if (source) {
		argv[1] = argv[0];
		argv += 1;
		argc -= 1;
	}
	if (argc != 3) {
		fprintf(stderr, "usage: u7ucdis [--source] <USECODE> <hex function id>\n");
		return 2;
	}
	Corpus corpus;
	if (corpus_load(argv[1], &corpus) != 0) {
		return 1;
	}

	uint16_t wanted = (uint16_t) strtoul(argv[2], NULL, 16);
	const U7Function *function = NULL;
	for (size_t i = 0; i < corpus.count; ++i) {
		if (corpus.functions[i].function_id == wanted) {
			function = &corpus.functions[i];
			break;
		}
	}
	if (function == NULL) {
		fprintf(stderr, "function %04X not found\n", wanted);
		corpus_release(&corpus);
		return 1;
	}

	printf("function %04X: args=%u locals=%u links=%u code=%zu bytes\n",
			function->function_id, function->argument_count,
			function->local_count, function->link_count, function->code_size);

	U7Cfg cfg;
	if (u7_cfg_build(function, &cfg) != U7_OK) {
		fprintf(stderr, "control-flow graph failed\n");
		corpus_release(&corpus);
		return 1;
	}

	U7StackReport report;
	u7_stack_depths(function, &cfg, resolve, &corpus, &report);
	printf("  analyzed=%s balanced=%s max depth=%zu",
			report.analyzed ? "yes" : "no", report.balanced ? "yes" : "no",
			report.max_depth);
	if (!report.balanced || !report.analyzed) {
		printf(" first problem at +0x%zX", report.conflict_offset);
	}
	printf("\n");

	if (source) {
		U7Lifted lifted;
		U7Structured structured;
		static U7DebugLine lines[4096];
		static U7DebugFunction debug;
		size_t line_count = 0;
		if (u7_debug_lines(function, lines, 4096U, &line_count) == U7_OK &&
				line_count > 0) {
			g_lines = lines;
			g_line_count = line_count;
		}
		g_function = function;
		if (u7_debug_read(function, &debug) == U7_OK) {
			g_debug = &debug;
			printf("  source module %.*s\n",
					debug.module_length > 0 ? (int) debug.module_length : 1,
					debug.module != NULL ? debug.module : "-");
		}
		if (u7_lift_function(function, &cfg, resolve, &corpus, &lifted) == U7_OK &&
				u7_structure_function(function, &cfg, &lifted, &structured) == U7_OK) {
			printf("  %zu gotos, %zu loops, %zu conditionals\n",
					structured.goto_count, structured.loop_count,
					structured.conditional_count);
			print_region(structured.root, &lifted, 1);
		}
		u7_structured_release(&structured);
		u7_lifted_release(&lifted);
		u7_cfg_release(&cfg);
		corpus_release(&corpus);
		return 0;
	}

	U7BlockFacts *facts = calloc(cfg.block_count, sizeof *facts);
	if (facts != NULL) {
		(void) u7_block_facts(&cfg, facts, cfg.block_count);
	}

	for (size_t b = 0; b < cfg.block_count; ++b) {
		const U7Block *block = &cfg.blocks[b];
		printf("block %zu  [%04zX..%04zX)", b, block->start_offset, block->end_offset);
		if (facts != NULL) {
			printf("  idom=%d ipdom=%d loop=%d exit=%d%s", facts[b].idom,
					facts[b].ipdom, facts[b].loop_header, facts[b].loop_exit,
					facts[b].is_header ? " HEADER" : "");
		}
		if (block->taken != U7_NO_BLOCK) {
			printf("  taken->%d", block->taken);
		}
		if (block->fallthrough != U7_NO_BLOCK) {
			printf("  next->%d", block->fallthrough);
		}
		printf("\n");
		for (size_t i = 0; i < block->instruction_count; ++i) {
			const U7Instruction *instruction =
					&cfg.instructions[block->first_instruction + i];
			U7StackEffect effect;
			u7_stack_effect(function, instruction, resolve, &corpus, &effect);
			printf("  %04zX  %-22s", instruction->offset, instruction->info->mnemonic);
			print_operands(function, instruction);
			printf("   [-%u+%u%s]\n", effect.pops, effect.pushes,
					effect.known ? "" : " unknown");
		}
	}

	free(facts);
	u7_cfg_release(&cfg);
	corpus_release(&corpus);
	return 0;
}
