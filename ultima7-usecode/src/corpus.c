#include "u7/corpus.h"

#include <stdlib.h>
#include <string.h>

/* A function leaves a value behind only through these two exits. */
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

void u7_corpus_release(U7Corpus *corpus) {
	if (corpus == NULL) {
		return;
	}
	free(corpus->bytes);
	free(corpus->functions);
	free(corpus->returns_value);
	memset(corpus, 0, sizeof *corpus);
}

U7Result u7_corpus_load(const char *path, U7Corpus *out_corpus) {
	if (path == NULL || out_corpus == NULL) {
		return U7_ERROR_ARGUMENT;
	}
	uint8_t *bytes = NULL;
	size_t size = 0;
	U7Result result = u7_read_file(path, &bytes, &size);
	if (result != U7_OK) {
		memset(out_corpus, 0, sizeof *out_corpus);
		return result;
	}
	return u7_corpus_parse(bytes, size, out_corpus);
}

U7Result u7_corpus_parse(uint8_t *bytes, size_t size, U7Corpus *out_corpus) {
	if (bytes == NULL || out_corpus == NULL) {
		free(bytes);
		return U7_ERROR_ARGUMENT;
	}
	memset(out_corpus, 0, sizeof *out_corpus);
	out_corpus->bytes = bytes;
	out_corpus->size = size;
	U7Result result = U7_OK;

	size_t capacity = 2048;
	out_corpus->functions = malloc(capacity * sizeof *out_corpus->functions);
	out_corpus->returns_value = malloc(capacity * sizeof *out_corpus->returns_value);
	if (out_corpus->functions == NULL || out_corpus->returns_value == NULL) {
		u7_corpus_release(out_corpus);
		return U7_ERROR_CAPACITY;
	}

	size_t offset = 0;
	while (offset < out_corpus->size) {
		if (out_corpus->count == capacity) {
			capacity *= 2U;
			U7Function *grown = realloc(out_corpus->functions, capacity * sizeof *grown);
			if (grown == NULL) {
				u7_corpus_release(out_corpus);
				return U7_ERROR_CAPACITY;
			}
			out_corpus->functions = grown;
			bool *flags = realloc(out_corpus->returns_value, capacity * sizeof *flags);
			if (flags == NULL) {
				u7_corpus_release(out_corpus);
				return U7_ERROR_CAPACITY;
			}
			out_corpus->returns_value = flags;
		}

		U7Function *function = &out_corpus->functions[out_corpus->count];
		result = u7_parse_function(out_corpus->bytes, out_corpus->size, offset, function);
		if (result != U7_OK) {
			u7_corpus_release(out_corpus);
			return result;
		}
		out_corpus->returns_value[out_corpus->count] = scan_returns_value(function);
		offset = function->next_offset;
		out_corpus->count += 1U;
	}
	return U7_OK;
}

const U7Function *u7_corpus_find(const U7Corpus *corpus, uint16_t function_id) {
	if (corpus == NULL) {
		return NULL;
	}
	for (size_t i = 0; i < corpus->count; ++i) {
		if (corpus->functions[i].function_id == function_id) {
			return &corpus->functions[i];
		}
	}
	return NULL;
}

bool u7_corpus_resolve(void *context, uint16_t function_id, U7CallTarget *out_target) {
	const U7Corpus *corpus = context;
	if (corpus == NULL || out_target == NULL) {
		return false;
	}
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
