#include "u7/corpus.h"
#include "u7/structure.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Structures every function of a real corpus and checks the result against
 * the graph it came from: every reachable block placed, none copied except a
 * lone return, and every block's successors exactly what the structure says.
 * A tidy-looking listing that routes control somewhere else fails here.
 */
int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "usage: test_corpus_structure <USECODE>\n");
		return 2;
	}
	U7Corpus corpus;
	/* A corpus that cannot be read is a failure, not a pass: a check that
	 * quietly measured nothing looks exactly like one that found nothing. */
	if (u7_corpus_load(argv[1], &corpus) != U7_OK) {
		printf("test_corpus_structure: FAILED: cannot read %s\n", argv[1]);
		return 1;
	}

	size_t missing = 0;
	size_t repeated = 0;
	size_t misrouted = 0;
	size_t failed = 0;
	for (size_t i = 0; i < corpus.count; ++i) {
		const U7Function *function = &corpus.functions[i];
		U7Cfg cfg;
		U7Lifted lifted;
		U7Structured structured;
		memset(&lifted, 0, sizeof lifted);
		memset(&structured, 0, sizeof structured);
		if (u7_cfg_build(function, &cfg) != U7_OK ||
				u7_lift_function(function, &cfg, u7_corpus_resolve, &corpus,
						&lifted) != U7_OK ||
				u7_structure_function(function, &cfg, &lifted, &structured) != U7_OK) {
			failed += 1U;
		} else {
			missing += structured.blocks_missing;
			repeated += structured.blocks_repeated;
			if (structured.edges_misrendered > 0) {
				misrouted += 1U;
				printf("  fn_%04X routes %zu block(s) wrongly\n", function->function_id,
						structured.edges_misrendered);
			}
		}
		u7_structured_release(&structured);
		u7_lifted_release(&lifted);
		u7_cfg_release(&cfg);
	}
	u7_corpus_release(&corpus);

	bool ok = missing == 0 && repeated == 0 && misrouted == 0 && failed == 0;
	printf("test_corpus_structure: %s: %zu missing, %zu copied, %zu functions misrouted,"
			" %zu failed\n", ok ? "passed" : "FAILED", missing, repeated, misrouted, failed);
	return ok ? 0 : 1;
}
