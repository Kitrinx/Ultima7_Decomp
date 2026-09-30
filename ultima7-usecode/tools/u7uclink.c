#include "u7/linkdep.h"

#include <stdio.h>
#include <stdlib.h>

/* Rebuilds the two dependency files the engine reads beside USECODE. */
int main(int argc, char **argv) {
	if (argc != 4) {
		fprintf(stderr, "usage: u7uclink <USECODE> <LINKDEP1> <LINKDEP2>\n");
		return 2;
	}

	U7Corpus corpus;
	U7Result result = u7_corpus_load(argv[1], &corpus);
	if (result != U7_OK) {
		fprintf(stderr, "%s: %s\n", argv[1], u7_result_name(result));
		return 1;
	}

	/* Nothing is written for a USECODE the engine could not load. */
	static U7Load loads[U7_LINKDEP_FUNCTIONS];
	result = u7_linkdep_loads(&corpus, loads);
	size_t entries = 0;
	for (size_t f = 0; result == U7_OK && f < corpus.count; ++f) {
		uint16_t id = corpus.functions[f].function_id;
		entries += loads[id].functions;
		if (loads[id].functions > U7_LOAD_FUNCTIONS || loads[id].bytes > U7_LOAD_BYTES) {
			fprintf(stderr, "to run fn_%04X the engine loads %zu functions, %zu bytes; it holds at "
					"most %d functions and %d bytes\n", id, loads[id].functions, loads[id].bytes,
					U7_LOAD_FUNCTIONS, U7_LOAD_BYTES);
			result = U7_ERROR_RANGE;
		}
	}
	if (result == U7_OK && entries > U7_LINKDEP_ENTRIES) {
		fprintf(stderr, "LINKDEP2 would hold %zu entries; the engine reads at most %d\n", entries,
				U7_LINKDEP_ENTRIES);
		result = U7_ERROR_RANGE;
	}
	if (result != U7_OK) {
		u7_corpus_release(&corpus);
		return 1;
	}

	FILE *one = fopen(argv[2], "wb");
	FILE *two = fopen(argv[3], "wb");
	result = one != NULL && two != NULL
			? u7_write_linkdep(&corpus, one, two, &entries) : U7_ERROR_IO;
	if (one != NULL) {
		fclose(one);
	}
	if (two != NULL) {
		fclose(two);
	}
	if (result != U7_OK) {
		fprintf(stderr, "cannot write the dependency files: %s\n", u7_result_name(result));
		u7_corpus_release(&corpus);
		return 1;
	}
	printf("%s -> %s, %s\n", argv[1], argv[2], argv[3]);
	printf("  %zu functions, %zu dependency entries\n", corpus.count, entries);
	u7_corpus_release(&corpus);
	return 0;
}
