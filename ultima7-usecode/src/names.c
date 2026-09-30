#include "u7/names.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool u7_names_add(U7NameTable *table, U7NameKind kind, const char *text, int32_t value) {
	if (table->count == table->capacity) {
		size_t capacity = table->capacity > 0 ? table->capacity * 2U : 1024U;
		U7NameEntry *grown = realloc(table->entries, capacity * sizeof *grown);
		if (grown == NULL) {
			return false;
		}
		table->entries = grown;
		table->capacity = capacity;
	}
	size_t length = strlen(text);
	char *copy = malloc(length + 1U);
	if (copy == NULL) {
		return false;
	}
	memcpy(copy, text, length + 1U);
	table->entries[table->count] = (U7NameEntry) {kind, copy, value, table->count};
	table->count += 1U;
	table->sorted = false;
	return true;
}

void u7_names_release(U7NameTable *table) {
	for (size_t i = 0; i < table->count; ++i) {
		free(table->entries[i].text);
	}
	free(table->entries);
	memset(table, 0, sizeof *table);
}

static int compare_entries(const void *a, const void *b) {
	const U7NameEntry *x = a;
	const U7NameEntry *y = b;
	if (x->kind != y->kind) {
		return (int) x->kind - (int) y->kind;
	}
	int order = strcmp(x->text, y->text);
	if (order != 0) {
		return order;
	}
	return x->order < y->order ? -1 : x->order > y->order;
}

bool u7_names_lookup(const void *context, U7NameKind kind, const char *name, size_t length,
		int32_t *out_value) {
	U7NameTable *table = (U7NameTable *) context;
	if (table == NULL || table->count == 0) {
		return false;
	}
	if (!table->sorted) {
		qsort(table->entries, table->count, sizeof *table->entries, compare_entries);
		table->sorted = true;
	}
	/* The last entry of this kind and name, so later ones override earlier. */
	size_t low = 0;
	size_t high = table->count;
	while (low < high) {
		size_t middle = low + (high - low) / 2U;
		const U7NameEntry *entry = &table->entries[middle];
		int order = (int) entry->kind - (int) kind;
		if (order == 0) {
			order = strncmp(entry->text, name, length);
			if (order == 0 && entry->text[length] != 0) {
				order = 1;
			}
		}
		if (order <= 0) {
			low = middle + 1U;
		} else {
			high = middle;
		}
	}
	if (low == 0) {
		return false;
	}
	const U7NameEntry *found = &table->entries[low - 1U];
	if (found->kind != kind || strncmp(found->text, name, length) != 0 ||
			found->text[length] != 0) {
		return false;
	}
	*out_value = found->value;
	return true;
}

/* A word of the header: letters, digits, underscores and the sigils. */
static size_t word_at(const char *at, const char *end) {
	size_t length = 0;
	while (at + length < end && (at[length] == '_' || at[length] == '$' || at[length] == '#' ||
			at[length] == '~' || (at[length] >= '0' && at[length] <= '9') ||
			(at[length] >= 'a' && at[length] <= 'z') || (at[length] >= 'A' && at[length] <= 'Z'))) {
		length += 1U;
	}
	return length;
}

U7Result u7_names_read_header(U7NameTable *table, const char *text, size_t length,
		size_t *out_error_line) {
	const char *at = text;
	const char *end = text + length;
	size_t line = 1U;
	int32_t engine_calls = 0;
	int32_t next_flag = 0;
	while (at < end) {
		const char *stop = memchr(at, '\n', (size_t) (end - at));
		const char *line_end = stop != NULL ? stop : end;
		const char *p = at;
		while (p < line_end && (*p == ' ' || *p == '\t' || *p == '\r')) {
			p += 1;
		}
		char words[4][128];
		size_t count = 0;
		while (p < line_end && count < 4U && !(p[0] == '/' && p + 1 < line_end && p[1] == '/')) {
			size_t n = word_at(p, line_end);
			if (n == 0) {
				p += 1;
				continue;
			}
			snprintf(words[count++], sizeof words[0], "%.*s", (int) n, p);
			p += n;
		}
		const char *comment = NULL;
		for (const char *c = at; c + 1 < line_end; ++c) {
			if (c[0] == '/' && c[1] == '/') {
				comment = c;
				break;
			}
		}
		bool ok = true;
		if (count == 0) {
			/* A blank line or a comment. */
		} else if (strcmp(words[0], "croutine") == 0 || strcmp(words[0], "cfunction") == 0) {
			ok = count >= 2 && u7_names_add(table, U7_NAME_INTRINSIC, words[1], engine_calls++);
		} else if (strcmp(words[0], "usableindex") == 0) {
			ok = count == 3 && words[1][0] == '#' &&
					u7_names_add(table, U7_NAME_FUNCTION, words[1] + 1, (int32_t) atol(words[2]));
		} else if (strcmp(words[0], "global") == 0) {
			long number = next_flag;
			if (comment != NULL) {
				const char *digits = comment;
				while (digits < line_end && (*digits < '0' || *digits > '9')) {
					digits += 1;
				}
				if (digits < line_end) {
					number = atol(digits);
				}
			}
			ok = count >= 3 && strcmp(words[1], "flag") == 0 && words[2][0] == '$' &&
					u7_names_add(table, U7_NAME_FLAG, words[2] + 1, (int32_t) number);
			next_flag = (int32_t) number + 1;
		} else if (strcmp(words[0], "constant") == 0 || strcmp(words[0], "action") == 0) {
			ok = count == 3 && u7_names_add(table, words[0][0] == 'a' ? U7_NAME_ACTION
					: U7_NAME_CONSTANT, words[1], (int32_t) atol(words[2]));
		} else {
			ok = false;
		}
		if (!ok) {
			if (out_error_line != NULL) {
				*out_error_line = line;
			}
			return U7_ERROR_MISMATCH;
		}
		at = stop != NULL ? stop + 1 : end;
		line += 1U;
	}
	return U7_OK;
}
