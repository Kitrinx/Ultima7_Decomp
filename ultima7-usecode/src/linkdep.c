#include "u7/linkdep.h"

#include <stdlib.h>
#include <string.h>

typedef struct Table {
	uint32_t where[U7_LINKDEP_FUNCTIONS];
	uint16_t size[U7_LINKDEP_FUNCTIONS];
	const U7Function *function[U7_LINKDEP_FUNCTIONS];
	bool present[U7_LINKDEP_FUNCTIONS];
} Table;

/* Collects the call closure of one function, each member once. */
static size_t closure(const Table *table, uint16_t root, uint16_t *out,
		bool *seen) {
	memset(seen, 0, U7_LINKDEP_FUNCTIONS * sizeof *seen);

	uint16_t *stack = malloc(U7_LINKDEP_FUNCTIONS * sizeof *stack);
	if (stack == NULL) {
		return 0;
	}
	size_t top = 0;
	size_t count = 0;
	stack[top++] = root;
	seen[root] = true;

	while (top > 0) {
		uint16_t current = stack[--top];
		out[count++] = current;

		const U7Function *function = table->function[current];
		if (function == NULL) {
			continue;
		}
		for (uint16_t i = 0; i < function->link_count; ++i) {
			uint16_t target = 0;
			if (u7_function_link(function, i, &target) != U7_OK ||
					target >= U7_LINKDEP_FUNCTIONS || seen[target]) {
				continue;
			}
			seen[target] = true;
			stack[top++] = target;
		}
	}

	free(stack);
	return count;
}

/* Every function's place, payload size and record, by number. */
static U7Result fill_table(const U7Corpus *corpus, Table *table, uint16_t *out_highest) {
	uint16_t highest = 0;
	for (size_t i = 0; i < corpus->count; ++i) {
		const U7Function *function = &corpus->functions[i];
		if (function->function_id >= U7_LINKDEP_FUNCTIONS) {
			return U7_ERROR_RANGE;
		}
		table->where[function->function_id] = (uint32_t) function->record_offset;
		table->size[function->function_id] = function->payload_size;
		table->function[function->function_id] = function;
		table->present[function->function_id] = true;
		highest = function->function_id;
	}
	*out_highest = highest;
	return U7_OK;
}

U7Result u7_linkdep_loads(const U7Corpus *corpus, U7Load *out) {
	Table *table = calloc(1, sizeof *table);
	uint16_t *tree = malloc(U7_LINKDEP_FUNCTIONS * sizeof *tree);
	bool *seen = malloc(U7_LINKDEP_FUNCTIONS * sizeof *seen);
	uint16_t highest = 0;
	U7Result result = table != NULL && tree != NULL && seen != NULL
			? fill_table(corpus, table, &highest) : U7_ERROR_CAPACITY;
	for (uint32_t id = 0; result == U7_OK && id <= highest; ++id) {
		if (!table->present[id]) {
			continue;
		}
		size_t count = closure(table, (uint16_t) id, tree, seen);
		out[id].functions = count;
		out[id].bytes = 0;
		for (size_t i = 0; i < count; ++i) {
			out[id].bytes += table->size[tree[i]];
		}
	}
	free(table);
	free(tree);
	free(seen);
	return result;
}

static int compare_ids(const void *left, const void *right) {
	uint16_t a = *(const uint16_t *) left;
	uint16_t b = *(const uint16_t *) right;
	return a < b ? -1 : a > b ? 1 : 0;
}

static void write_u16(FILE *out, uint16_t value) {
	fputc(value & 0xFF, out);
	fputc((value >> 8) & 0xFF, out);
}

static void write_u32(FILE *out, uint32_t value) {
	for (int i = 0; i < 4; ++i) {
		fputc((int) ((value >> (i * 8)) & 0xFFU), out);
	}
}

U7Result u7_write_linkdep(const U7Corpus *corpus, FILE *one, FILE *two, size_t *out_entries) {
	Table *table = calloc(1, sizeof *table);
	uint16_t *tree = malloc(U7_LINKDEP_FUNCTIONS * sizeof *tree);
	bool *seen = malloc(U7_LINKDEP_FUNCTIONS * sizeof *seen);
	U7Result result = U7_OK;
	if (table == NULL || tree == NULL || seen == NULL) {
		result = U7_ERROR_CAPACITY;
		goto done;
	}

	uint16_t highest = 0;
	result = fill_table(corpus, table, &highest);
	if (result != U7_OK) {
		goto done;
	}

	uint16_t written = 0;
	size_t closures = 0;
	for (uint32_t id = 0; id <= highest; ++id) {
		if (!table->present[id]) {
			write_u16(one, written);
			write_u16(one, 0xFFFFU);
			continue;
		}

		size_t count = closure(table, (uint16_t) id, tree, seen);
		qsort(tree, count, sizeof *tree, compare_ids);

		uint16_t total = 0;
		for (size_t i = 0; i < count; ++i) {
			total = (uint16_t) (total + table->size[tree[i]]);
		}

		write_u16(one, written);
		write_u16(one, total);
		for (size_t i = 0; i < count; ++i) {
			write_u32(two, table->where[tree[i]]);
			written += 1U;
		}
		closures += count;
	}
	write_u16(one, written);
	write_u16(one, 0);
	if (out_entries != NULL) {
		*out_entries = closures;
	}

done:
	free(table);
	free(tree);
	free(seen);
	return result;
}
