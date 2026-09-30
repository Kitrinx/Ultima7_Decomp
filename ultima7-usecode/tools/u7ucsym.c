#include "u7/debug_records.h"
#include "u7/normalize.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Corpus {
	uint8_t *bytes;
	size_t size;
	U7Function *functions;
	size_t count;
} Corpus;

static void corpus_release(Corpus *corpus);
static int corpus_load(const char *path, Corpus *out_corpus);
static int command_extract(const char *beta_path, const char *out_path);
static int command_transfer(const char *beta_path, const char *retail_path);
static int command_diff(const char *beta_path, const char *retail_path, const char *id_text);

static void corpus_release(Corpus *corpus) {
	free(corpus->bytes);
	free(corpus->functions);
	memset(corpus, 0, sizeof *corpus);
}

static int corpus_load(const char *path, Corpus *out_corpus) {
	memset(out_corpus, 0, sizeof *out_corpus);

	U7Result result = u7_read_file(path, &out_corpus->bytes, &out_corpus->size);
	if (result != U7_OK) {
		fprintf(stderr, "%s: %s\n", path, u7_result_name(result));
		return 1;
	}

	size_t capacity = 2048;
	out_corpus->functions = malloc(capacity * sizeof *out_corpus->functions);
	if (out_corpus->functions == NULL) {
		corpus_release(out_corpus);
		return 1;
	}

	size_t offset = 0;
	while (offset < out_corpus->size) {
		if (out_corpus->count == capacity) {
			capacity *= 2U;
			U7Function *grown = realloc(out_corpus->functions,
					capacity * sizeof *grown);
			if (grown == NULL) {
				corpus_release(out_corpus);
				return 1;
			}
			out_corpus->functions = grown;
		}
		result = u7_parse_function(out_corpus->bytes, out_corpus->size, offset,
				&out_corpus->functions[out_corpus->count]);
		if (result != U7_OK) {
			fprintf(stderr, "%s: %s at 0x%zX\n", path, u7_result_name(result), offset);
			corpus_release(out_corpus);
			return 1;
		}
		offset = out_corpus->functions[out_corpus->count].next_offset;
		out_corpus->count += 1U;
	}
	return 0;
}

static const U7Function *corpus_find(const Corpus *corpus, uint16_t function_id) {
	for (size_t i = 0; i < corpus->count; ++i) {
		if (corpus->functions[i].function_id == function_id) {
			return &corpus->functions[i];
		}
	}
	return NULL;
}

/* Counts instructions that are not debug records. */
static size_t u7_instruction_count(const U7Function *function) {
	size_t count = 0;
	size_t at = 0;
	while (at < function->code_size) {
		U7Instruction instruction;
		if (u7_decode_instruction(function->code, function->code_size, at,
				&instruction) != U7_OK) {
			break;
		}
		if (!u7_opcode_is_debug(instruction.info->opcode)) {
			count += 1U;
		}
		at += instruction.encoded_size;
	}
	return count;
}

static int command_extract(const char *beta_path, const char *out_path) {
	Corpus beta;
	if (corpus_load(beta_path, &beta) != 0) {
		return 1;
	}

	FILE *out = fopen(out_path, "w");
	if (out == NULL) {
		fprintf(stderr, "%s: cannot write\n", out_path);
		corpus_release(&beta);
		return 1;
	}

	fprintf(out, "# Serpent Isle beta debug records\n");
	fprintf(out, "# function <id> <module> args=<n> locals=<n> lines=<n> "
			"first=<line> last=<line>\n");
	fprintf(out, "# local <slot> <name>\n");

	size_t with_records = 0;
	size_t named_slots = 0;
	size_t total_lines = 0;
	size_t incomplete = 0;

	for (size_t i = 0; i < beta.count; ++i) {
		const U7Function *function = &beta.functions[i];
		U7DebugFunction debug;
		U7Result result = u7_debug_read(function, &debug);
		if (result != U7_OK) {
			continue;
		}
		with_records += 1U;
		total_lines += debug.line_count;
		if (!debug.names_complete) {
			incomplete += 1U;
		}

		fprintf(out, "function %04X %.*s args=%u locals=%u lines=%zu first=%u last=%u\n",
				function->function_id,
				debug.module_length > 0 ? (int) debug.module_length : 1,
				debug.module != NULL ? debug.module : "-",
				function->argument_count, function->local_count,
				debug.line_count, debug.first_line, debug.last_line);

		for (uint16_t slot = 0; slot < debug.local_count; ++slot) {
			const char *text = NULL;
			size_t length = 0;
			if (u7_debug_local_name(function, &debug, slot, &text, &length) != U7_OK) {
				break;
			}
			fprintf(out, "local %u %.*s\n", slot, (int) length, text);
			named_slots += 1U;
		}
	}

	fclose(out);
	printf("%s -> %s\n", beta_path, out_path);
	printf("  %zu functions, %zu with debug records, %zu incomplete name runs\n",
			beta.count, with_records, incomplete);
	printf("  %zu named local slots, %zu source line markers\n", named_slots, total_lines);
	corpus_release(&beta);
	return 0;
}

static int command_transfer(const char *beta_path, const char *retail_path) {
	Corpus beta;
	Corpus retail;
	if (corpus_load(beta_path, &beta) != 0) {
		return 1;
	}
	if (corpus_load(retail_path, &retail) != 0) {
		corpus_release(&beta);
		return 1;
	}

	size_t shared = 0;
	size_t identical = 0;
	size_t transferred_slots = 0;
	size_t same_opcode_count = 0;
	size_t same_shape = 0;
	U7Canonical left = {0};
	U7Canonical right = {0};

	for (size_t i = 0; i < beta.count; ++i) {
		const U7Function *beta_function = &beta.functions[i];
		const U7Function *retail_function =
				corpus_find(&retail, beta_function->function_id);
		if (retail_function == NULL) {
			continue;
		}
		shared += 1U;

		if (u7_canonical_body(beta_function, &left) != U7_OK ||
				u7_canonical_body(retail_function, &right) != U7_OK) {
			continue;
		}
		if (left.size == right.size && memcmp(left.bytes, right.bytes, left.size) == 0) {
			identical += 1U;
			transferred_slots += retail_function->local_count;
		}
		if (beta_function->argument_count == retail_function->argument_count &&
				beta_function->local_count == retail_function->local_count) {
			same_shape += 1U;
		}
		if (u7_instruction_count(beta_function) == u7_instruction_count(retail_function)) {
			same_opcode_count += 1U;
		}
	}

	u7_canonical_release(&left);
	u7_canonical_release(&right);

	printf("beta %zu functions, retail %zu functions\n", beta.count, retail.count);
	printf("  shared ids: %zu\n", shared);
	printf("  bodies identical once debug records are removed: %zu\n", identical);
	printf("  retail local slots inheriting an original name: %zu\n", transferred_slots);
	printf("  same argument and local counts: %zu\n", same_shape);
	printf("  same surviving instruction count: %zu\n", same_opcode_count);

	corpus_release(&beta);
	corpus_release(&retail);
	return 0;
}

static int command_diff(const char *beta_path, const char *retail_path,
		const char *id_text) {
	Corpus beta;
	Corpus retail;
	if (corpus_load(beta_path, &beta) != 0) {
		return 1;
	}
	if (corpus_load(retail_path, &retail) != 0) {
		corpus_release(&beta);
		return 1;
	}

	uint16_t wanted = (uint16_t) strtoul(id_text, NULL, 16);
	const U7Function *left_function = corpus_find(&beta, wanted);
	const U7Function *right_function = corpus_find(&retail, wanted);
	if (left_function == NULL || right_function == NULL) {
		fprintf(stderr, "function %04X missing from one corpus\n", wanted);
		corpus_release(&beta);
		corpus_release(&retail);
		return 1;
	}

	printf("function %04X\n", wanted);
	printf("  beta   args=%u locals=%u instructions=%zu\n",
			left_function->argument_count, left_function->local_count,
			u7_instruction_count(left_function));
	printf("  retail args=%u locals=%u instructions=%zu\n",
			right_function->argument_count, right_function->local_count,
			u7_instruction_count(right_function));

	U7Canonical left = {0};
	U7Canonical right = {0};
	if (u7_canonical_body(left_function, &left) == U7_OK &&
			u7_canonical_body(right_function, &right) == U7_OK) {
		size_t limit = left.size < right.size ? left.size : right.size;
		size_t at = 0;
		while (at < limit && left.bytes[at] == right.bytes[at]) {
			at += 1U;
		}
		printf("  canonical sizes %zu and %zu, first difference at byte %zu\n",
				left.size, right.size, at);
	}
	u7_canonical_release(&left);
	u7_canonical_release(&right);

	corpus_release(&beta);
	corpus_release(&retail);
	return 0;
}

int main(int argc, char **argv) {
	if (argc == 4 && strcmp(argv[1], "extract") == 0) {
		return command_extract(argv[2], argv[3]);
	}
	if (argc == 4 && strcmp(argv[1], "transfer") == 0) {
		return command_transfer(argv[2], argv[3]);
	}
	if (argc == 5 && strcmp(argv[1], "diff") == 0) {
		return command_diff(argv[2], argv[3], argv[4]);
	}
	fprintf(stderr, "usage: u7ucsym extract <beta USECODE> <sidecar>\n");
	fprintf(stderr, "       u7ucsym transfer <beta USECODE> <retail USECODE>\n");
	fprintf(stderr, "       u7ucsym diff <beta USECODE> <retail USECODE> <hex id>\n");
	return 2;
}
