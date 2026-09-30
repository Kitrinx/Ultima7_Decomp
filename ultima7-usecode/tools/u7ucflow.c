#include "u7/debug_records.h"
#include "u7/structure.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Entry {
	U7Function function;
	bool returns_value;
	bool scanned;
} Entry;

typedef struct Corpus {
	uint8_t *bytes;
	size_t size;
	Entry *entries;
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
		/* Only these two leave a value behind for the caller. */
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
		if (corpus->entries[i].function.function_id != function_id) {
			continue;
		}
		out_target->argument_count = corpus->entries[i].function.argument_count;
		out_target->returns_value = corpus->entries[i].returns_value;
		return true;
	}
	return false;
}

static void corpus_release(Corpus *corpus) {
	free(corpus->bytes);
	free(corpus->entries);
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
	out_corpus->entries = calloc(capacity, sizeof *out_corpus->entries);
	if (out_corpus->entries == NULL) {
		corpus_release(out_corpus);
		return 1;
	}

	size_t offset = 0;
	while (offset < out_corpus->size) {
		if (out_corpus->count == capacity) {
			capacity *= 2U;
			Entry *grown = realloc(out_corpus->entries, capacity * sizeof *grown);
			if (grown == NULL) {
				corpus_release(out_corpus);
				return 1;
			}
			out_corpus->entries = grown;
		}
		Entry *entry = &out_corpus->entries[out_corpus->count];
		memset(entry, 0, sizeof *entry);
		result = u7_parse_function(out_corpus->bytes, out_corpus->size, offset,
				&entry->function);
		if (result != U7_OK) {
			fprintf(stderr, "%s: %s at 0x%zX\n", path, u7_result_name(result), offset);
			corpus_release(out_corpus);
			return 1;
		}
		entry->returns_value = scan_returns_value(&entry->function);
		offset = entry->function.next_offset;
		out_corpus->count += 1U;
	}
	return 0;
}

int main(int argc, char **argv) {
	bool detail = argc > 1 && strcmp(argv[1], "--detail") == 0;
	int first = detail ? 2 : 1;
	if (argc <= first) {
		fprintf(stderr, "usage: u7ucflow [--detail] <USECODE> ...\n");
		return 2;
	}

	for (int a = first; a < argc; ++a) {
		Corpus corpus;
		if (corpus_load(argv[a], &corpus) != 0) {
			return 1;
		}

		size_t analyzed = 0;
		size_t balanced = 0;
		size_t cfg_failed = 0;
		size_t clean_exit = 0;
		size_t blocked_by[256];
		size_t exit_depths[8];
		memset(blocked_by, 0, sizeof blocked_by);
		memset(exit_depths, 0, sizeof exit_depths);
		size_t returns_value = 0;
		size_t blocks = 0;
		size_t lifted_complete = 0;
		size_t lifted_self_contained = 0;
		size_t statements = 0;
		size_t lift_blocked[256];
		memset(lift_blocked, 0, sizeof lift_blocked);
		size_t structured_fully = 0;
		size_t gotos = 0;
		size_t loops = 0;
		size_t conditionals = 0;
		size_t converses = 0;
		size_t foreaches = 0;
		size_t revisit = 0;
		size_t escape = 0;
		size_t unstructured = 0;
		size_t missing_blocks = 0;
		size_t repeated_blocks = 0;
		size_t edges_checked = 0;
		size_t misrendered = 0;
		size_t functions_misrendered = 0;
		size_t same_order = 0;
		size_t ordered_checked = 0;
		size_t line_checked = 0;
		size_t line_agreeing = 0;
		size_t line_straddled = 0;

		for (size_t i = 0; i < corpus.count; ++i) {
			const U7Function *function = &corpus.entries[i].function;
			returns_value += corpus.entries[i].returns_value ? 1U : 0U;

			U7Cfg cfg;
			if (u7_cfg_build(function, &cfg) != U7_OK) {
				cfg_failed += 1U;
				u7_cfg_release(&cfg);
				continue;
			}
			blocks += cfg.block_count;

			U7StackReport report;
			if (u7_stack_depths(function, &cfg, resolve, &corpus, &report) != U7_OK) {
				cfg_failed += 1U;
				u7_cfg_release(&cfg);
				continue;
			}
			if (!report.analyzed) {
				blocked_by[report.blocking_opcode] += 1U;
			} else {
				analyzed += 1U;
				if (!report.balanced && detail) {
					printf("    unbalanced %04X at code +0x%zX\n",
							function->function_id, report.conflict_offset);
				}
				if (report.balanced) {
					balanced += 1U;
					exit_depths[report.exit_depth < 8U ? report.exit_depth : 7U] += 1U;
					if (report.exit_depth == 0 && report.exit_depths_agree) {
						clean_exit += 1U;
					}
				}
			}
			U7Lifted lifted;
			if (u7_lift_function(function, &cfg, resolve, &corpus, &lifted) == U7_OK) {
				if (lifted.complete) {
					lifted_complete += 1U;
					if (lifted.self_contained) {
						lifted_self_contained += 1U;
					}
				} else {
					lift_blocked[lifted.failure_opcode] += 1U;
				}
				for (size_t b = 0; b < lifted.block_count; ++b) {
					statements += lifted.blocks[b].statement_count;
				}
				/* A recovered statement must not straddle a source line
				 * marker: the original compiler put one at each statement. */
				U7DebugLine lines[4096];
				size_t line_count = 0;
				if (u7_debug_lines(function, lines, 4096U, &line_count) == U7_OK &&
						line_count > 0) {
					line_checked += 1U;
					size_t straddles = 0;
					for (size_t b = 0; b < lifted.block_count; ++b) {
						for (const U7Stmt *st = lifted.blocks[b].first; st != NULL;
								st = st->next) {
							for (size_t l = 0; l < line_count; ++l) {
								if (lines[l].code_offset > st->first_offset &&
										lines[l].code_offset <= st->offset) {
									straddles += 1U;
									break;
								}
							}
						}
					}
					line_straddled += straddles;
					if (straddles == 0) {
						line_agreeing += 1U;
					}
				}

				U7Structured structured;
				if (u7_structure_function(function, &cfg, &lifted, &structured) == U7_OK) {
					structured_fully += structured.complete ? 1U : 0U;
					gotos += structured.goto_count;
					if (detail && structured.goto_count > 3U) {
						printf("    %04X needs %zu gotos\n", function->function_id,
								structured.goto_count);
					}
					loops += structured.loop_count;
					conditionals += structured.conditional_count;
					converses += structured.converse_count;
					foreaches += structured.foreach_count;
					revisit += structured.goto_revisit;
					escape += structured.goto_loop_escape;
					unstructured += structured.goto_unstructured;
					missing_blocks += structured.blocks_missing;
					repeated_blocks += structured.blocks_repeated;
					edges_checked += structured.edges_checked;
					misrendered += structured.edges_misrendered;
					if (structured.edges_misrendered > 0) {
						functions_misrendered += 1U;
						if (detail) {
							printf("    %04X routes %zu block(s) wrongly, first block %d\n",
									function->function_id, structured.edges_misrendered,
									structured.first_misrendered);
						}
					}
					/* Does reading the structure back out visit the blocks in
					 * the order the original file laid them down? */
					if (structured.complete) {
						size_t seen = 0;
						bool ordered = true;
						size_t stack_size = 0;
						const U7Region *stack[256];
						stack[stack_size++] = structured.root;
						/* Depth-first, children left to right. */
						size_t order[1024];
						size_t order_count = 0;
						while (stack_size > 0 && ordered) {
							const U7Region *region = stack[--stack_size];
							if (region == NULL) {
								continue;
							}
							if (region->kind == U7_REGION_BLOCK) {
								if (order_count < 1024U) {
									order[order_count++] = region->block;
								}
								continue;
							}
							if (region->kind != U7_REGION_SEQUENCE &&
									region->kind != U7_REGION_GOTO &&
									region->kind != U7_REGION_BREAK &&
									region->kind != U7_REGION_CONTINUE &&
									region->kind != U7_REGION_LABEL) {
								if (order_count < 1024U) {
									order[order_count++] = region->block;
								}
							}
							for (size_t c = region->child_count; c-- > 0;) {
								if (stack_size < 256U) {
									stack[stack_size++] = region->children[c];
								}
							}
						}
						for (size_t k = 1; k < order_count; ++k) {
							if (order[k] < order[k - 1U]) {
								ordered = false;
							}
						}
						seen = order_count;
						(void) seen;
						ordered_checked += 1U;
						same_order += ordered ? 1U : 0U;
					}
				}
				u7_structured_release(&structured);
			}
			u7_lifted_release(&lifted);

			u7_cfg_release(&cfg);
		}

		printf("%s\n", argv[a]);
		printf("  %zu functions, %zu basic blocks, %zu return a value\n",
				corpus.count, blocks, returns_value);
		printf("  control-flow graph failed: %zu\n", cfg_failed);
		printf("  every effect known and every call resolved: %zu\n", analyzed);
		printf("  depth consistent at every join: %zu\n", balanced);
		printf("  and reaching every return at depth zero: %zu\n", clean_exit);
		printf("  exit depth 0/1/2/3+: %zu/%zu/%zu/%zu\n", exit_depths[0],
				exit_depths[1], exit_depths[2],
				exit_depths[3] + exit_depths[4] + exit_depths[5] +
				exit_depths[6] + exit_depths[7]);

		printf("  lifted to expressions: %zu, needing no value from a predecessor: %zu\n",
				lifted_complete, lifted_self_contained);
		printf("  statements recovered: %zu\n", statements);
		printf("  structured with no goto: %zu, residual gotos: %zu\n",
				structured_fully, gotos);
		if (line_checked > 0) {
			printf("  statement boundaries agreeing with original line markers: %zu of %zu"
					" (%zu straddled)\n", line_agreeing, line_checked, line_straddled);
		}
		printf("  of those, structure read back in original block order: %zu of %zu\n",
				same_order, ordered_checked);
		printf("  blocks never placed: %zu, placed twice without being a lone return: %zu\n",
				missing_blocks, repeated_blocks);
		printf("  blocks whose structure routes control wrongly: %zu of %zu, in %zu functions\n",
				misrendered, edges_checked, functions_misrendered);
		printf("  gotos by cause: %zu already placed, %zu leaving a loop, %zu not rejoining\n",
				revisit, escape, unstructured);
		printf("  loops: %zu (%zu conversations, %zu array loops), conditionals: %zu\n",
				loops, converses, foreaches, conditionals);
		printf("  lift blocked by opcode:");
		for (size_t opcode = 0; opcode < 256U; ++opcode) {
			if (lift_blocked[opcode] != 0) {
				const U7OpcodeInfo *info = u7_opcode_info((uint8_t) opcode);
				printf(" %02zX %s=%zu", opcode, info != NULL ? info->mnemonic : "?",
						lift_blocked[opcode]);
			}
		}
		printf("\n");
		printf("  depth blocked by opcode:");
		for (size_t opcode = 0; opcode < 256U; ++opcode) {
			if (blocked_by[opcode] != 0) {
				const U7OpcodeInfo *info = u7_opcode_info((uint8_t) opcode);
				printf(" %02zX %s=%zu", opcode, info != NULL ? info->mnemonic : "?",
						blocked_by[opcode]);
			}
		}
		printf("\n");
		corpus_release(&corpus);
	}
	return 0;
}
