#include "u7/corpus.h"
#include "u7/emit.h"
#include "u7/names.h"
#include "u7/sha256.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

static char g_intrinsic_storage[256][48];
static const char *g_intrinsics[256];

static const char *g_functions[65536];
static const char *g_flags[65536];
static const char *g_objects[65536];
static uint8_t g_object_arguments[256];

/* Community names, used only where the symbol table has none. */
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
		int fields = sscanf(line, "%7s %x %15s %15s %47s", which, &ordinal, arity,
				result, name);
		if (fields < 5 || name[0] == '-' || strcmp(name, "UNKNOWN") == 0) {
			continue;
		}
		if (ordinal < 256U && strcmp(which, game) == 0) {
			snprintf(g_intrinsic_storage[ordinal], sizeof g_intrinsic_storage[0],
					"%s", name);
			g_intrinsics[ordinal] = g_intrinsic_storage[ordinal];
		}
	}
	fclose(file);
}

/* Named values by kind, sorted by value once loaded. */
typedef struct Constant {
	unsigned value;
	const char *name;
} Constant;

static Constant *g_constants[U7_CONST_KINDS];
static size_t g_constant_counts[U7_CONST_KINDS];
static uint8_t g_argument_kinds[256][8];
static uint8_t g_result_kinds[256];
static U7ScriptAction g_script_actions[256];

static const char *const g_kind_names[U7_CONST_KINDS] = {
	"", "event", "script", "item_flag", "npc_stat", "work_type", "direction", "shape", "usable",
	"alignment", "attack_mode", "weather", "egg_criteria", "special"
};

static unsigned kind_named(const char *name) {
	for (unsigned k = 1; k < U7_CONST_KINDS; ++k) {
		if (strcmp(name, g_kind_names[k]) == 0) {
			return k;
		}
	}
	return U7_CONST_NONE;
}

static char *copy_text(const char *text) {
	size_t length = strlen(text) + 1U;
	char *copy = malloc(length);
	if (copy != NULL) {
		memcpy(copy, text, length);
	}
	return copy;
}

static int by_value(const void *a, const void *b) {
	unsigned x = ((const Constant *) a)->value;
	unsigned y = ((const Constant *) b)->value;
	return (x > y) - (x < y);
}

static const char *constant_name(const void *context, unsigned kind, unsigned value) {
	(void) context;
	Constant key = {value, NULL};
	const Constant *found = kind < U7_CONST_KINDS && g_constants[kind] != NULL
			? bsearch(&key, g_constants[kind], g_constant_counts[kind], sizeof key, by_value)
			: NULL;
	return found != NULL ? found->name : NULL;
}

/*
 * The rows that say what constants mean: a kind's named values, the kind each
 * intrinsic argument and result takes, and the script actions with their operands.
 */
static bool load_constant_row(char **field, size_t count) {
	unsigned number = (unsigned) strtoul(field[1], NULL, 10);
	if (strncmp(field[0], "const_", 6) == 0 && count >= 3) {
		unsigned kind = kind_named(field[0] + 6);
		if (kind == U7_CONST_NONE || number > 0xFFFFU) {
			return true;
		}
		Constant *grown = realloc(g_constants[kind], (g_constant_counts[kind] + 1U) * sizeof *grown);
		if (grown == NULL) {
			return false;
		}
		g_constants[kind] = grown;
		grown[g_constant_counts[kind]++] = (Constant) {number, copy_text(field[2])};
		return true;
	}
	if (strcmp(field[0], "argument_kind") == 0 && count >= 4) {
		unsigned position = (unsigned) strtoul(field[2], NULL, 10);
		if (number < 256U && position < 8U) {
			g_argument_kinds[number][position] = (uint8_t) kind_named(field[3]);
		}
		return true;
	}
	if (strcmp(field[0], "result_kind") == 0 && count >= 3) {
		if (number < 256U) {
			g_result_kinds[number] = (uint8_t) kind_named(field[2]);
		}
		return true;
	}
	if (strcmp(field[0], "script_action") == 0 && count >= 6 && number < 256U) {
		U7ScriptAction *action = &g_script_actions[number];
		action->name = copy_text(field[2]);
		action->operand_count = 0;
		for (char *kind = strtok(field[5], ","); kind != NULL && action->operand_count < 4U;
				kind = strtok(NULL, ",")) {
			action->operand_kinds[action->operand_count++] =
					(unsigned char) (strcmp(kind, "-") == 0 ? 0 : kind_named(kind));
		}
		if (strcmp(field[5], "-") == 0) {
			action->operand_count = 0;
		}
		return true;
	}
	return true;
}

/* Rows of "kind number identifier", as the name survey writes them. */
static bool load_symbols(const char *path) {
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		return false;
	}
	char line[512];
	while (fgets(line, sizeof line, file) != NULL) {
		char kind[32];
		unsigned number = 0;
		char name[64];
		if (line[0] == '#') {
			continue;
		}
		if (strncmp(line, "const_", 6) == 0 || strncmp(line, "argument_kind", 13) == 0 ||
				strncmp(line, "result_kind", 11) == 0 || strncmp(line, "script_action", 13) == 0) {
			char *field[8];
			size_t count = 0;
			line[strcspn(line, "\r\n")] = 0;
			for (char *cursor = line; count < 8U; ) {
				field[count++] = cursor;
				char *tab = strchr(cursor, '\t');
				if (tab == NULL) {
					break;
				}
				*tab = 0;
				cursor = tab + 1;
			}
			if (!load_constant_row(field, count)) {
				fclose(file);
				return false;
			}
			continue;
		}
		if (sscanf(line, "%31s %u %63s", kind, &number, name) != 3 || number > 0xFFFFU) {
			continue;
		}
		if (strcmp(kind, "intrinsic") == 0) {
			if (number < 256U) {
				snprintf(g_intrinsic_storage[number], sizeof g_intrinsic_storage[0], "%s",
						name);
				g_intrinsics[number] = g_intrinsic_storage[number];
			}
			continue;
		}
		if (strcmp(kind, "object_argument") == 0) {
			unsigned argument = (unsigned) strtoul(name, NULL, 10);
			if (number < 256U && argument < 8U) {
				g_object_arguments[number] |= (uint8_t) (1U << argument);
			}
			continue;
		}
		const char **table = strcmp(kind, "function") == 0 ? g_functions
				: strcmp(kind, "flag") == 0 ? g_flags
				: strcmp(kind, "object") == 0 ? g_objects : NULL;
		size_t length = strlen(name) + 1U;
		char *copy = table != NULL ? malloc(length) : NULL;
		if (copy != NULL) {
			memcpy(copy, name, length);
			table[number] = copy;
		}
	}
	fclose(file);
	for (unsigned k = 0; k < U7_CONST_KINDS; ++k) {
		if (g_constants[k] != NULL) {
			qsort(g_constants[k], g_constant_counts[k], sizeof *g_constants[k], by_value);
		}
	}
	return true;
}

static uint16_t g_object_parameters[65536];
static uint8_t g_parameter_kinds[65536][16];

enum { CONFLICTING_KINDS = 0xFF };

/* A parameter that reaches two kinds of argument is named as neither. */
static bool note_parameter_kind(uint16_t function, uint16_t parameter, uint8_t kind) {
	uint8_t *slot = &g_parameter_kinds[function][parameter];
	if (kind == U7_CONST_NONE || kind == CONFLICTING_KINDS || *slot == kind ||
			*slot == CONFLICTING_KINDS) {
		return false;
	}
	*slot = *slot == U7_CONST_NONE ? kind : CONFLICTING_KINDS;
	return true;
}

/* A routine handing its own parameter, unchanged, to another routine's. */
typedef struct Handoff {
	uint16_t function;
	uint16_t parameter;
	uint16_t callee;
	uint16_t callee_parameter;
} Handoff;

static Handoff *g_handoffs;
static size_t g_handoff_count;
static size_t g_handoff_capacity;

static void add_handoff(Handoff handoff) {
	if (g_handoff_count == g_handoff_capacity) {
		size_t capacity = g_handoff_capacity > 0 ? g_handoff_capacity * 2U : 256U;
		Handoff *grown = realloc(g_handoffs, capacity * sizeof *grown);
		if (grown == NULL) {
			return;
		}
		g_handoffs = grown;
		g_handoff_capacity = capacity;
	}
	g_handoffs[g_handoff_count++] = handoff;
}

/* Parameters the routine never assigns, so they still hold what was passed. */
static uint16_t unchanged_parameters(const U7Function *function, const U7Lifted *lifted) {
	unsigned count = function->argument_count < 16U ? function->argument_count : 16U;
	uint16_t unchanged = (uint16_t) ((1U << count) - 1U);
	for (size_t b = 0; b < lifted->block_count; ++b) {
		for (const U7Stmt *s = lifted->blocks[b].first; s != NULL; s = s->next) {
			if ((s->kind == U7_STMT_ASSIGN_LOCAL || s->kind == U7_STMT_ASSIGN_ELEMENT) &&
					s->slot >= 0 && s->slot < 16) {
				unchanged &= (uint16_t) ~(1U << s->slot);
			} else if (s->kind == U7_STMT_LOOP || s->kind == U7_STMT_LOOP_START) {
				/* A loop writes some of the slots it names; count them all. */
				for (size_t i = 0; i < U7_MAX_OPERANDS; ++i) {
					if (s->operands[i] >= 0 && s->operands[i] < 16) {
						unchanged &= (uint16_t) ~(1U << s->operands[i]);
					}
				}
			}
		}
	}
	return unchanged;
}

static void scan_handoffs(const U7Expr *expr, uint16_t function, uint16_t unchanged) {
	if (expr == NULL) {
		return;
	}
	for (size_t i = 0; i < expr->operand_count; ++i) {
		const U7Expr *operand = expr->operands[i];
		scan_handoffs(operand, function, unchanged);
		if (operand == NULL || operand->kind != U7_EXPR_LOCAL || operand->value < 0 ||
				operand->value >= 16 || (unchanged & (1U << operand->value)) == 0) {
			continue;
		}
		uint16_t parameter = (uint16_t) operand->value;
		size_t from_last = expr->operand_count - 1U - i;
		if (expr->kind == U7_EXPR_INTRINSIC && from_last < 8U && expr->value >= 0 &&
				expr->value < 256) {
			if ((g_object_arguments[expr->value] & (1U << from_last)) != 0) {
				g_object_parameters[function] |= (uint16_t) (1U << parameter);
			}
			note_parameter_kind(function, parameter, g_argument_kinds[expr->value][from_last]);
		} else if (expr->kind == U7_EXPR_CALL && i < 16U) {
			add_handoff((Handoff) {function, parameter, (uint16_t) expr->value,
					(uint16_t) i});
		}
	}
}

/*
 * A routine's parameter takes an object when the routine passes it on unchanged
 * to an argument that takes one, an intrinsic's or another routine's.
 */
static void find_object_parameters(U7Corpus *corpus) {
	for (size_t f = 0; f < corpus->count; ++f) {
		const U7Function *function = &corpus->functions[f];
		U7Cfg cfg;
		U7Lifted lifted;
		memset(&cfg, 0, sizeof cfg);
		memset(&lifted, 0, sizeof lifted);
		if (u7_cfg_build(function, &cfg) == U7_OK &&
				u7_lift_function(function, &cfg, u7_corpus_resolve, corpus,
						&lifted) == U7_OK) {
			uint16_t unchanged = unchanged_parameters(function, &lifted);
			for (size_t b = 0; b < lifted.block_count; ++b) {
				const U7LiftedBlock *block = &lifted.blocks[b];
				for (const U7Stmt *s = block->first; s != NULL; s = s->next) {
					scan_handoffs(s->value, function->function_id, unchanged);
					scan_handoffs(s->subscript, function->function_id, unchanged);
				}
				for (size_t i = 0; i < block->outgoing_count; ++i) {
					scan_handoffs(block->outgoing[i], function->function_id, unchanged);
				}
			}
		}
		u7_lifted_release(&lifted);
		u7_cfg_release(&cfg);
	}
	bool changed = true;
	while (changed) {
		changed = false;
		for (size_t i = 0; i < g_handoff_count; ++i) {
			const Handoff *h = &g_handoffs[i];
			uint16_t bit = (uint16_t) (1U << h->parameter);
			if ((g_object_parameters[h->callee] & (1U << h->callee_parameter)) != 0 &&
					(g_object_parameters[h->function] & bit) == 0) {
				g_object_parameters[h->function] |= bit;
				changed = true;
			}
			if (note_parameter_kind(h->function, h->parameter,
					g_parameter_kinds[h->callee][h->callee_parameter])) {
				changed = true;
			}
		}
	}
	free(g_handoffs);
}

/* Writes one function in a style; false when it cannot be recovered. */
static bool emit_one(FILE *out, U7Corpus *corpus, const U7Function *function,
		U7EmitStyle style, bool without_ids, const uint8_t *returns_value) {
	U7Cfg cfg;
	U7Lifted lifted;
	U7Structured structured;
	memset(&lifted, 0, sizeof lifted);
	memset(&structured, 0, sizeof structured);
	bool ok = u7_cfg_build(function, &cfg) == U7_OK &&
			u7_lift_function(function, &cfg, u7_corpus_resolve, corpus, &lifted) == U7_OK &&
			u7_structure_function(function, &cfg, &lifted, &structured) == U7_OK;
	if (ok) {
		U7DebugFunction debug;
		static U7DebugLine lines[4096];
		size_t line_count = 0;
		U7Names names;
		memset(&names, 0, sizeof names);
		names.function = function;
		names.cfg = &cfg;
		names.intrinsics = g_intrinsics;
		names.functions = g_functions;
		names.flags = g_flags;
		names.objects = g_objects;
		names.object_arguments = g_object_arguments;
		names.object_parameters = g_object_parameters;
		names.returns_value = returns_value;
		names.constant_name = constant_name;
		names.argument_kinds = g_argument_kinds;
		names.result_kinds = g_result_kinds;
		names.script_actions = g_script_actions;
		names.parameter_kinds = (const uint8_t (*)[16]) g_parameter_kinds;
		names.without_ids = without_ids;
		if (style == U7_EMIT_ANNOTATED) {
			if (u7_debug_read(function, &debug) == U7_OK) {
				names.debug = &debug;
			}
			if (u7_debug_lines(function, lines, 4096U, &line_count) == U7_OK) {
				names.lines = lines;
				names.line_count = line_count;
			}
		}
		u7_emit_function(out, function, &lifted, &structured, &names, style);
	}
	u7_structured_release(&structured);
	u7_lifted_release(&lifted);
	u7_cfg_release(&cfg);
	return ok;
}

/* Engine calls some code calls for a result, which Origin declared cfunction. */
static void find_result_calls(const U7Corpus *corpus, bool *returns) {
	for (size_t i = 0; i < corpus->count; ++i) {
		const U7Function *function = &corpus->functions[i];
		for (size_t at = 0; at < function->code_size;) {
			U7Instruction instruction;
			if (u7_decode_instruction(function->code, function->code_size, at,
					&instruction) != U7_OK) {
				break;
			}
			int32_t ordinal = 0;
			if (instruction.info->opcode == 0x38U &&
					u7_instruction_operand(&instruction, 0, &ordinal) == U7_OK &&
					ordinal >= 0 && ordinal < 256) {
				returns[ordinal] = true;
			}
			at += instruction.encoded_size;
		}
	}
}

/* The header, in the form of Origin's index. */
static void write_header(FILE *out, const bool *result_calls) {
	fputs("// Engine calls, numbered in the order they are declared.\n", out);
	int last = -1;
	for (int i = 0; i < 256; ++i) {
		if (g_intrinsics[i] != NULL) {
			last = i;
		}
	}
	for (int i = 0; i <= last; ++i) {
		const char *kind = result_calls[i] ? "cfunction" : "croutine";
		if (g_intrinsics[i] != NULL) {
			fprintf(out, "%s %s;\n", kind, g_intrinsics[i]);
		} else {
			fprintf(out, "%s intrinsic_%02X;\n", kind, (unsigned) i);
		}
	}
	fputs("\n// Usables, by number.\n", out);
	for (unsigned id = 0; id < 0x800U; ++id) {
		if (g_functions[id] != NULL) {
			fprintf(out, "usableindex #%s %u;\n", g_functions[id], id);
		}
	}
	fputs("\n// Flags.\n", out);
	for (unsigned flag = 0; flag < 65536U; ++flag) {
		if (g_flags[flag] != NULL) {
			fprintf(out, "global flag $%s = False; //flag number %u\n", g_flags[flag], flag);
		}
	}
	fputs("\n// Constants, and script action codes, which are pushed as bytes.\n", out);
	for (unsigned value = 0; value < 65536U; ++value) {
		if (g_objects[value] != NULL) {
			fprintf(out, "constant %s %u;\n", g_objects[value], value);
		}
	}
	for (unsigned kind = 1; kind < U7_CONST_KINDS; ++kind) {
		for (size_t i = 0; i < g_constant_counts[kind]; ++i) {
			const Constant *c = &g_constants[kind][i];
			if (c->name[0] == '_') {
				fprintf(out, "constant %s %u;\n", c->name, c->value);
			}
		}
	}
	for (unsigned code = 0; code < 256U; ++code) {
		if (g_script_actions[code].name != NULL) {
			fprintf(out, "action %s %u;\n", g_script_actions[code].name, code);
		}
	}
}

/* Compiles one function's text and compares it with the record it came from. */
static bool compiles_back(const char *text, size_t length, U7NameTable *names,
		const U7Function *function) {
	U7Parsed parsed;
	bool same = false;
	if (u7_compile_source(text, length, u7_names_lookup, names, &parsed) == U7_OK &&
			parsed.count == 1U) {
		static uint8_t record[0x10000];
		U7Unit unit;
		U7LowerReport report;
		u7_parsed_unit(&parsed.functions[0], &unit);
		same = parsed.functions[0].record == NULL &&
				u7_assemble_unit(&unit, record, sizeof record, &report) == U7_OK &&
				report.code_size == function->record_size &&
				memcmp(record, function->data - 6U, report.code_size) == 0;
	}
	u7_parsed_release(&parsed);
	return same;
}

/* Text a function writes in a style, or NULL. */
static char *emit_text(U7Corpus *corpus, const U7Function *function, U7EmitStyle style,
		const uint8_t *returns_value, size_t *out_length) {
	char *text = NULL;
	FILE *memory = open_memstream(&text, out_length);
	if (memory == NULL) {
		return NULL;
	}
	bool ok = emit_one(memory, corpus, function, style, true, returns_value);
	fclose(memory);
	if (!ok) {
		free(text);
		return NULL;
	}
	return text;
}

/* The source file a function belongs in, where U7_MODULES names one. */
static char *g_module_of[65536];

static void load_modules(const char *path) {
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		return;
	}
	char line[256];
	while (fgets(line, sizeof line, file) != NULL) {
		unsigned id = 0;
		char name[128];
		if (line[0] != '#' && sscanf(line, "%x %127s", &id, name) == 2 && id < 65536U) {
			free(g_module_of[id]);
			g_module_of[id] = copy_text(name);
		}
	}
	fclose(file);
}

/* One source file of a project and the functions it holds, in number order. */
typedef struct SourceFile {
	char name[128];
	size_t *functions;        /* Indices into the corpus. */
	size_t count;
	size_t capacity;
	unsigned first_routine;   /* The lowest routine number it holds, or 0x10000. */
} SourceFile;

/* Origin's order: files named for characters, then those starting &, each by name. */
static int origin_order(const char *a, const char *b) {
	bool amp_a = a[0] == '&';
	bool amp_b = b[0] == '&';
	if (amp_a != amp_b) {
		return amp_a ? 1 : -1;
	}
	return strcasecmp(a, b);
}

static int by_first_routine(const void *a, const void *b) {
	const SourceFile *x = *(const SourceFile *const *) a;
	const SourceFile *y = *(const SourceFile *const *) b;
	return (x->first_routine > y->first_routine) - (x->first_routine < y->first_routine);
}

static int by_origin_order(const void *a, const void *b) {
	return origin_order((*(const SourceFile *const *) a)->name,
			(*(const SourceFile *const *) b)->name);
}

/*
 * The link order. Routines are numbered in it, so the files that hold routines
 * keep the order their numbers show; the others fall where Origin's sorted
 * order puts them among those.
 */
static SourceFile **link_order(SourceFile *files, size_t count) {
	SourceFile **numbered = malloc((count + 1U) * sizeof *numbered);
	SourceFile **others = malloc((count + 1U) * sizeof *others);
	SourceFile **order = malloc((count + 1U) * sizeof *order);
	if (numbered == NULL || others == NULL || order == NULL) {
		free(numbered);
		free(others);
		free(order);
		return NULL;
	}
	size_t n = 0;
	size_t o = 0;
	for (size_t i = 0; i < count; ++i) {
		if (files[i].first_routine < 0x10000U) {
			numbered[n++] = &files[i];
		} else {
			others[o++] = &files[i];
		}
	}
	qsort(numbered, n, sizeof *numbered, by_first_routine);
	qsort(others, o, sizeof *others, by_origin_order);
	size_t at = 0;
	size_t next_other = 0;
	for (size_t i = 0; i < n; ++i) {
		while (next_other < o && origin_order(others[next_other]->name, numbered[i]->name) < 0) {
			order[at++] = others[next_other++];
		}
		order[at++] = numbered[i];
	}
	while (next_other < o) {
		order[at++] = others[next_other++];
	}
	free(numbered);
	free(others);
	return order;
}

/*
 * Which named routines need their number written: the fewest that leave the
 * rest rising in link order, which the build then numbers by itself.
 */
static void find_pins(SourceFile **order, size_t count, const U7Corpus *corpus, bool *pinned) {
	size_t total = 0;
	for (size_t f = 0; f < count; ++f) {
		total += order[f]->count;
	}
	unsigned *numbers = malloc((total + 1U) * sizeof *numbers);
	size_t *tails = malloc((total + 1U) * sizeof *tails);
	size_t *before = malloc((total + 1U) * sizeof *before);
	if (numbers == NULL || tails == NULL || before == NULL) {
		free(numbers);
		free(tails);
		free(before);
		return;
	}
	size_t n = 0;
	for (size_t f = 0; f < count; ++f) {
		for (size_t i = 0; i < order[f]->count; ++i) {
			unsigned id = corpus->functions[order[f]->functions[i]].function_id;
			if (id >= 0x800U && g_functions[id] != NULL) {
				numbers[n++] = id;
				pinned[id] = true;
			}
		}
	}
	/* The longest rising run, kept; everything outside it is pinned. */
	size_t length = 0;
	for (size_t i = 0; i < n; ++i) {
		size_t low = 0;
		size_t high = length;
		while (low < high) {
			size_t middle = (low + high) / 2U;
			if (numbers[tails[middle]] < numbers[i]) {
				low = middle + 1U;
			} else {
				high = middle;
			}
		}
		before[i] = low > 0 ? tails[low - 1U] : SIZE_MAX;
		tails[low] = i;
		length = low + 1U > length ? low + 1U : length;
	}
	for (size_t i = length > 0 ? tails[length - 1U] : SIZE_MAX; i != SIZE_MAX; i = before[i]) {
		pinned[numbers[i]] = false;
	}
	free(numbers);
	free(tails);
	free(before);
}

/*
 * A function the readable form cannot say exactly: the linear form, marked
 * #linear, or failing that its record's bytes, marked #bytes. It is declared
 * by its name where it has one, so other code can call it so.
 */
static bool write_fallback(FILE *source, U7Corpus *corpus, const U7Function *function,
		const uint8_t *returns_value, U7NameTable *names, bool pinned, bool *out_bytes) {
	size_t length = 0;
	char *text = emit_text(corpus, function, U7_EMIT_LINEAR, returns_value, &length);
	char *number = text != NULL ? strstr(text, " fn_") : NULL;
	if (number == NULL) {
		free(text);
		return false;
	}
	size_t name_start = (size_t) (number - text) + 1U;
	unsigned id = function->function_id;
	char declared[160];
	if (g_functions[id] != NULL) {
		snprintf(declared, sizeof declared, "%s%s", returns_value[id] ? "~" : "",
				g_functions[id]);
		if (pinned) {
			size_t used = strlen(declared);
			snprintf(declared + used, sizeof declared - used, " #id(0x%04X)", id);
		}
	} else {
		snprintf(declared, sizeof declared, "fn_%04X", id);
	}
	char *marked = NULL;
	size_t marked_length = 0;
	FILE *marking = open_memstream(&marked, &marked_length);
	if (marking == NULL) {
		free(text);
		return false;
	}
	fprintf(marking, "%.*s%s #linear%s", (int) name_start, text, declared,
			text + name_start + 7U);
	fclose(marking);
	*out_bytes = !compiles_back(marked, marked_length, names, function);
	if (!*out_bytes) {
		fputs(marked, source);
	} else {
		/* No form of the source says it: the record's own bytes. */
		fprintf(source, "%.*s%s #bytes", (int) name_start, text, declared);
		for (uint16_t a = 0; a < function->argument_count; ++a) {
			fprintf(source, "%s a%u", a == 0 ? " accepts" : ",", a);
		}
		fputs(" {", source);
		const uint8_t *bytes = function->data - 6U;
		for (size_t b = 0; b < function->record_size; ++b) {
			fprintf(source, "%s%02X", b % 32U == 0 ? "\n    " : "", bytes[b]);
		}
		fputs("\n}\n", source);
	}
	free(marked);
	free(text);
	return true;
}

/*
 * Writes a project that compiles back to the file it came from: one source
 * file per module, the header, the build order, the routine map and the
 * manifest, all in one directory. A function whose readable text does not
 * compile back to its record is written in the linear form. An existing
 * project is never written over, so edits made to one are kept.
 */
static int write_project(const char *directory, const char *usecode, U7Corpus *corpus,
		const uint8_t *returns_value) {
	char path[1024];
	snprintf(path, sizeof path, "%s/project.u7p", directory);
	FILE *existing = fopen(path, "r");
	if (existing != NULL) {
		fclose(existing);
		fprintf(stderr, "%s: a project is already there; it is left as it is\n", directory);
		return 1;
	}
	(void) mkdir(directory, 0755);

	static bool result_calls[256];
	find_result_calls(corpus, result_calls);
	char *header = NULL;
	size_t header_length = 0;
	FILE *memory = open_memstream(&header, &header_length);
	if (memory == NULL) {
		return 1;
	}
	write_header(memory, result_calls);
	fclose(memory);

	U7NameTable names = {0};
	size_t error_line = 0;
	if (u7_names_read_header(&names, header, header_length, &error_line) != U7_OK) {
		fprintf(stderr, "the header does not read back, line %zu\n", error_line);
		free(header);
		return 1;
	}
	for (unsigned id = 0x800U; id < 65536U; ++id) {
		if (g_functions[id] != NULL) {
			(void) u7_names_add(&names, U7_NAME_FUNCTION, g_functions[id], (int32_t) id);
		}
	}
	/* What the build checks calls against: each function's arguments and result. */
	for (size_t i = 0; i < corpus->count; ++i) {
		const U7Function *function = &corpus->functions[i];
		char numbered[8];
		snprintf(numbered, sizeof numbered, "fn_%04X", function->function_id);
		const char *called[2] = {g_functions[function->function_id], numbered};
		for (size_t n = 0; n < 2U; ++n) {
			if (called[n] != NULL) {
				(void) u7_names_add(&names, U7_NAME_ARGUMENTS, called[n],
						(int32_t) function->argument_count);
				(void) u7_names_add(&names, U7_NAME_RETURNS, called[n],
						returns_value[function->function_id] ? 1 : 0);
			}
		}
	}
	snprintf(path, sizeof path, "%s/usecode.uh", directory);
	FILE *file = fopen(path, "w");
	if (file == NULL) {
		fprintf(stderr, "%s: cannot write\n", path);
		free(header);
		u7_names_release(&names);
		return 1;
	}
	fwrite(header, 1U, header_length, file);
	fclose(file);
	free(header);

	/* Each function into its module's file, or a file of its own name. */
	SourceFile *files = calloc(corpus->count + 1U, sizeof *files);
	size_t file_count = 0;
	int status = files == NULL ? 1 : 0;
	for (size_t i = 0; i < corpus->count && status == 0; ++i) {
		unsigned id = corpus->functions[i].function_id;
		char name[128];
		snprintf(name, sizeof name, "%s", g_module_of[id] != NULL ? g_module_of[id]
				: g_functions[id] != NULL ? g_functions[id] : "");
		if (name[0] == 0) {
			snprintf(name, sizeof name, "fn_%04X", id);
		}
		SourceFile *into = NULL;
		for (size_t f = 0; f < file_count && into == NULL; ++f) {
			into = strcmp(files[f].name, name) == 0 ? &files[f] : NULL;
		}
		if (into == NULL) {
			into = &files[file_count++];
			snprintf(into->name, sizeof into->name, "%s", name);
			into->first_routine = 0x10000U;
		}
		if (into->count == into->capacity) {
			into->capacity = into->capacity > 0 ? into->capacity * 2U : 8U;
			size_t *grown = realloc(into->functions, into->capacity * sizeof *grown);
			if (grown == NULL) {
				status = 1;
				break;
			}
			into->functions = grown;
		}
		into->functions[into->count++] = i;
		if (id >= 0x800U && id < into->first_routine) {
			into->first_routine = id;
		}
	}
	SourceFile **order = status == 0 ? link_order(files, file_count) : NULL;
	static bool pinned[65536];
	if (order == NULL) {
		status = 1;
	} else {
		find_pins(order, file_count, corpus, pinned);
	}

	size_t readable = 0;
	size_t linear = 0;
	size_t carried = 0;
	size_t pins = 0;
	for (size_t f = 0; f < file_count && status == 0; ++f) {
		snprintf(path, sizeof path, "%s/%s.use", directory, order[f]->name);
		FILE *source = fopen(path, "w");
		if (source == NULL) {
			fprintf(stderr, "%s: cannot write\n", path);
			status = 1;
			break;
		}
		for (size_t i = 0; i < order[f]->count && status == 0; ++i) {
			const U7Function *function = &corpus->functions[order[f]->functions[i]];
			unsigned id = function->function_id;
			pins += pinned[id] ? 1U : 0U;
			size_t length = 0;
			char *text = NULL;
			FILE *writing = open_memstream(&text, &length);
			bool emitted = writing != NULL && emit_one(writing, corpus, function,
					U7_EMIT_ANNOTATED, !pinned[id], returns_value);
			if (writing != NULL) {
				fclose(writing);
			}
			if (emitted && compiles_back(text, length, &names, function)) {
				fputs(text, source);
				readable += 1U;
			} else {
				bool as_bytes = false;
				if (!write_fallback(source, corpus, function, returns_value, &names,
						pinned[id], &as_bytes)) {
					fprintf(stderr, "fn_%04X cannot be written\n", id);
					status = 1;
				}
				carried += as_bytes ? 1U : 0U;
				linear += as_bytes ? 0U : 1U;
			}
			free(text);
			if (i + 1U < order[f]->count) {
				fputc('\n', source);
			}
		}
		fclose(source);
	}

	/* The files in the order they are built, one to a line. */
	snprintf(path, sizeof path, "%s/usecode.lnk", directory);
	FILE *link = status == 0 ? fopen(path, "w") : NULL;
	if (status == 0 && link == NULL) {
		fprintf(stderr, "%s: cannot write\n", path);
		status = 1;
	}
	if (link != NULL) {
		for (size_t f = 0; f < file_count; ++f) {
			fprintf(link, "%s.use\n", order[f]->name);
		}
		fclose(link);
	}

	/* The routine map: each named routine's number, which the build keeps. */
	snprintf(path, sizeof path, "%s/usecode.map", directory);
	FILE *map = status == 0 ? fopen(path, "w") : NULL;
	if (status == 0 && map == NULL) {
		fprintf(stderr, "%s: cannot write\n", path);
		status = 1;
	}
	if (map != NULL) {
		for (size_t i = 0; i < corpus->count; ++i) {
			unsigned id = corpus->functions[i].function_id;
			if (id >= 0x800U && g_functions[id] != NULL) {
				fprintf(map, "%04X %s\n", id, g_functions[id]);
			}
		}
		fclose(map);
	}

	uint8_t *bytes = NULL;
	size_t size = 0;
	char digest[65] = {0};
	if (u7_read_file(usecode, &bytes, &size) == U7_OK) {
		u7_sha256_hex(bytes, size, digest);
	}
	free(bytes);
	snprintf(path, sizeof path, "%s/project.u7p", directory);
	file = status == 0 ? fopen(path, "w") : NULL;
	if (status == 0 && file == NULL) {
		fprintf(stderr, "%s: cannot write\n", path);
		status = 1;
	}
	snprintf(path, sizeof path, "%s/README.md", directory);
	FILE *readme = status == 0 ? fopen(path, "w") : NULL;
	if (readme != NULL) {
		fputs("# Usecode\n\n"
				"The `.use` files here compile in the order `usecode.lnk` gives, against the names in\n"
				"`usecode.uh`, as Origin's were. To build:\n\n"
				"    u7ucproj project.u7p <directory>\n\n"
				"That writes `USECODE`, `LINKDEP1` and `LINKDEP2`; copy all three into the game's `STATIC`\n"
				"folder. With `--check` the build also fails unless `USECODE` matches the shipped file.\n\n"
				"When changing it:\n\n"
				"- A usable's number comes from its `usableindex` line in `usecode.uh`. A routine keeps the\n"
				"  number `usecode.map` holds for its name; a new routine takes the next free number, and\n"
				"  the build adds it to the map. A renamed routine is a new one, unless `#id(0x...)` after\n"
				"  its name keeps the old number.\n"
				"- A new file is added to `usecode.lnk`.\n"
				"- The build puts any function a call names in the caller's link table, sizes the locals to\n"
				"  the slots used, and refuses a call with the wrong argument count or one whose result is\n"
				"  used when there is none, or left unused when there is one.\n"
				"- To run a function the engine loads it with everything it can call: 35 functions and\n"
				"  65,389 bytes at most. The build refuses anything past that, and writes nothing.\n"
				"- A function marked `#linear` or `#bytes` is written in a form the readable one cannot yet\n"
				"  say; it compiles as it stands.\n", readme);
		fclose(readme);
	}
	if (file != NULL) {
		const char *game = getenv("U7_GAME");
		fputs("u7project 1;\n", file);
		fprintf(file, "game \"%s\";\n", game != NULL ? game : "bg");
		fputs("header \"usecode.uh\";\n", file);
		fputs("source_root \".\";\n", file);
		fputs("link \"usecode.lnk\";\n", file);
		fputs("map \"usecode.map\";\n", file);
		fprintf(file, "expect_sha256 \"%s\";\n", digest);
		fclose(file);
		printf("%s -> %s\n", usecode, directory);
		printf("  %zu sources; %zu functions readable, %zu linear, %zu as bytes; %zu pinned\n",
				file_count, readable, linear, carried, pins);
	}

	for (size_t f = 0; f < file_count; ++f) {
		free(files[f].functions);
	}
	free(files);
	free(order);
	u7_names_release(&names);
	return status;
}

int main(int argc, char **argv) {
	bool annotated = false;
	bool linear = false;
	const char *project = NULL;
	int index = 1;
	while (index < argc && strncmp(argv[index], "--", 2) == 0) {
		if (strcmp(argv[index], "--annotated") == 0) {
			annotated = true;
		} else if (strcmp(argv[index], "--linear") == 0) {
			linear = true;
		} else if (strcmp(argv[index], "--project") == 0) {
			annotated = true;
			project = argv[index + 1 < argc ? index + 1 : index];
			index += 1;
		}
		index += 1;
	}
	if (argc - index != (project != NULL ? 1 : 2)) {
		fprintf(stderr, "usage: u7ucdec [--annotated|--linear] <USECODE> <output.uc>\n");
		fprintf(stderr, "       u7ucdec --project <directory> <USECODE>\n");
		fprintf(stderr, "  U7_MODULES names each function's source file for a project.\n");
		fprintf(stderr, "  U7_INTRINSIC_NAMES and U7_GAME add intrinsic names.\n");
		fprintf(stderr, "  U7_SYMBOLS adds names, and takes precedence over the intrinsic names.\n");
		return 2;
	}

	const char *names_path = getenv("U7_INTRINSIC_NAMES");
	if (names_path != NULL) {
		const char *game = getenv("U7_GAME");
		load_intrinsic_names(names_path, game != NULL ? game : "bg");
	}
	const char *symbols_path = getenv("U7_SYMBOLS");
	if (symbols_path != NULL && !load_symbols(symbols_path)) {
		fprintf(stderr, "%s: cannot read\n", symbols_path);
		return 1;
	}
	const char *modules_path = getenv("U7_MODULES");
	if (modules_path != NULL) {
		load_modules(modules_path);
	}

	U7Corpus corpus;
	U7Result result = u7_corpus_load(argv[index], &corpus);
	if (result != U7_OK) {
		fprintf(stderr, "%s: %s\n", argv[index], u7_result_name(result));
		return 1;
	}

	if (annotated && symbols_path != NULL) {
		find_object_parameters(&corpus);
	}
	static uint8_t returns_value[65536];
	for (size_t i = 0; i < corpus.count; ++i) {
		returns_value[corpus.functions[i].function_id] = corpus.returns_value[i] ? 1U : 0U;
	}

	if (project != NULL) {
		int status = write_project(project, argv[index], &corpus, returns_value);
		u7_corpus_release(&corpus);
		return status;
	}

	FILE *out = fopen(argv[index + 1], "w");
	if (out == NULL) {
		fprintf(stderr, "%s: cannot write\n", argv[index + 1]);
		u7_corpus_release(&corpus);
		return 1;
	}

	size_t emitted = 0;
	size_t failed = 0;
	for (size_t i = 0; i < corpus.count; ++i) {
		const U7Function *function = &corpus.functions[i];

		if (!emit_one(out, &corpus, function,
				linear ? U7_EMIT_LINEAR : annotated ? U7_EMIT_ANNOTATED : U7_EMIT_CANONICAL,
				false, returns_value)) {
			fprintf(out, "// fn_%04X could not be recovered\n\n", function->function_id);
			failed += 1U;
		} else {
			fputc('\n', out);
			emitted += 1U;
		}
	}

	fclose(out);
	printf("%s -> %s\n", argv[index], argv[index + 1]);
	printf("  %zu functions emitted, %zu could not be recovered\n", emitted, failed);
	u7_corpus_release(&corpus);
	return failed == 0 ? 0 : 1;
}
