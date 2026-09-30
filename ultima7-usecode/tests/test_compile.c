#include "u7/emit.h"
#include "u7/names.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

static void check(bool condition, const char *what) {
	if (!condition) {
		printf("FAIL %s\n", what);
		failures += 1;
	}
}

/* A record; every function a fixture calls is in its link table, as in the games. */
static size_t build_record(uint8_t *out, const char *data, size_t data_size, uint16_t locals,
		uint16_t link, const uint8_t *code, size_t code_size) {
	uint16_t links = link != 0 ? 1U : 0U;
	size_t payload = 2U + data_size + 6U + links * 2U + code_size;
	u7_write_u16(out, 0x00B2U);
	u7_write_u16(out + 2U, (uint16_t) payload);
	u7_write_u16(out + 4U, (uint16_t) data_size);
	memcpy(out + 6U, data, data_size);
	size_t at = 6U + data_size;
	u7_write_u16(out + at, 0U);
	u7_write_u16(out + at + 2U, locals);
	u7_write_u16(out + at + 4U, links);
	at += 6U;
	if (links > 0) {
		u7_write_u16(out + at, link);
		at += 2U;
	}
	memcpy(out + at, code, code_size);
	return 4U + payload;
}

/* Every call in these fixtures takes one argument and leaves nothing. */
static bool resolve_one_argument(void *context, uint16_t function_id, U7CallTarget *out) {
	(void) context;
	(void) function_id;
	out->argument_count = 1U;
	out->returns_value = false;
	return true;
}

/*
 * Writes a function as the readable listing, compiles that text back and
 * checks the record comes out byte for byte.
 */
static void compiles_back(const uint8_t *code, size_t code_size, const char *data,
		size_t data_size, uint16_t locals, uint16_t link, const U7Names *names,
		U7NameTable *table, const char *expected_text, const char *what) {
	static uint8_t record[512];
	static uint8_t rebuilt[512];
	size_t size = build_record(record, data, data_size, locals, link, code, code_size);

	U7Function function;
	U7Cfg cfg;
	U7Lifted lifted;
	U7Structured structured;
	check(u7_parse_function(record, size, 0, &function) == U7_OK, "record parses");
	check(u7_cfg_build(&function, &cfg) == U7_OK, "graph builds");
	check(u7_lift_function(&function, &cfg, resolve_one_argument, NULL, &lifted) == U7_OK,
			"lift runs");
	check(u7_structure_function(&function, &cfg, &lifted, &structured) == U7_OK,
			"structure runs");

	U7Names with_function = names != NULL ? *names : (U7Names) {0};
	with_function.function = &function;
	with_function.cfg = &cfg;
	char *text = NULL;
	size_t length = 0;
	FILE *stream = open_memstream(&text, &length);
	check(stream != NULL, "text stream");
	if (stream == NULL) {
		return;
	}
	u7_emit_function(stream, &function, &lifted, &structured, &with_function,
			names != NULL ? U7_EMIT_ANNOTATED : U7_EMIT_CANONICAL);
	fclose(stream);
	if (expected_text != NULL && strstr(text, expected_text) == NULL) {
		printf("  %s: the listing lacks \"%s\":\n%s", what, expected_text, text);
		check(false, what);
	}

	U7Parsed parsed;
	U7LowerReport report;
	memset(&report, 0, sizeof report);
	bool compiled = u7_compile_source(text, length, u7_names_lookup, table, &parsed) == U7_OK &&
			parsed.count == 1U;
	if (!compiled) {
		printf("  %s: line %zu: %s\n%s", what, parsed.error_line, parsed.error, text);
	}
	if (compiled) {
		U7Unit unit;
		u7_parsed_unit(&parsed.functions[0], &unit);
		compiled = u7_assemble_unit(&unit, rebuilt, sizeof rebuilt, &report) == U7_OK;
	}
	bool same = compiled && report.code_size == size && memcmp(rebuilt, record, size) == 0;
	if (compiled && !same) {
		printf("  %s: compiled to %zu bytes, original %zu\n%s", what, report.code_size, size,
				text);
	}
	check(same, what);

	u7_parsed_release(&parsed);
	free(text);
	u7_structured_release(&structured);
	u7_lifted_release(&lifted);
	u7_cfg_release(&cfg);
}

/* The compiler's jump past an else follows an arm that aborts, reached or not. */
static void test_else_after_abort(void) {
	static const uint8_t code[] = {
		0x48, 0x1F, 0x01, 0x00, 0x22,  /* event = 1           */
		0x05, 0x04, 0x00,              /* jump_if_false ->0C  */
		0x3F,                          /* terminate           */
		0x06, 0x06, 0x00,              /* jump ->12           */
		0x1F, 0x07, 0x00,              /* push_word 7         */
		0x12, 0x00, 0x00,              /* pop_local v0        */
		0x25                           /* the closing return  */
	};
	compiles_back(code, sizeof code, "", 0U, 1U, 0, NULL, NULL, "} else {",
			"if/else whose first arm terminates");
}

/* Two tests failing to one else: a short-circuit and. */
static void test_short_circuit_and(void) {
	static const uint8_t code[] = {
		0x48, 0x1F, 0x01, 0x00, 0x22,  /* event = 1           */
		0x05, 0x0F, 0x00,              /* jump_if_false ->17  */
		0x21, 0x00, 0x00,              /* push_local v0       */
		0x05, 0x09, 0x00,              /* jump_if_false ->17  */
		0x1F, 0x07, 0x00,              /* push_word 7         */
		0x12, 0x01, 0x00,              /* pop_local v1        */
		0x06, 0x06, 0x00,              /* jump ->1D           */
		0x1F, 0x08, 0x00,              /* push_word 8         */
		0x12, 0x01, 0x00,              /* pop_local v1        */
		0x25
	};
	compiles_back(code, sizeof code, "", 0U, 2U, 0, NULL, NULL, " && ", "a && b with an else");
}

/* A loop tested after work, and one tested at the bottom. */
static void test_loops(void) {
	static const uint8_t work_first[] = {
		0x44, 0x05,                    /* 00 push_byte 5          */
		0x12, 0x00, 0x00,              /* 02 pop_local v0         */
		0x21, 0x00, 0x00,              /* 05 push_local v0        */
		0x05, 0x08, 0x00,              /* 08 jump_if_false ->13   */
		0x44, 0x01,                    /* 0B push_byte 1          */
		0x12, 0x01, 0x00,              /* 0D pop_local v1         */
		0x06, 0xED, 0xFF,              /* 10 jump ->00            */
		0x25                           /* 13                      */
	};
	compiles_back(work_first, sizeof work_first, "", 0U, 2U, 0, NULL, NULL, "} while v0 {",
			"loop { work } while test { body }");

	static const uint8_t bottom[] = {
		0x44, 0x00, 0x12, 0x00, 0x00,  /* v0 = byte(0)           */
		0x21, 0x00, 0x00, 0x44, 0x01,  /* 05 v0 + byte(1)        */
		0x09, 0x12, 0x00, 0x00,        /* -> v0                  */
		0x21, 0x00, 0x00, 0x44, 0x05,  /* v0 < byte(5)           */
		0x17, 0x05, 0x03, 0x00,        /* jump_if_false ->1A     */
		0x06, 0xEB, 0xFF,              /* jump ->05              */
		0x25
	};
	compiles_back(bottom, sizeof bottom, "", 0U, 1U, 0, NULL, NULL, "} while ", "do-while");
}

/* Foreach takes the next two free locals for its counter and count. */
static void test_foreach(void) {
	static const uint8_t code[] = {
		0x1F, 0x01, 0x00, 0x1F, 0x02, 0x00,  /* 00 push 1, 2            */
		0x1E, 0x02, 0x00,                    /* 06 make_array 2         */
		0x12, 0x00, 0x00,                    /* 09 pop_local v0         */
		0x2E,                                /* 0C start_loop           */
		0x02, 0x01, 0x00, 0x02, 0x00,        /* 0D loop v1 v2 v3 v0 ->21 */
		0x03, 0x00, 0x00, 0x00, 0x09, 0x00,
		0x21, 0x03, 0x00,                    /* 18 push_local v3        */
		0x12, 0x03, 0x00,                    /* 1B pop_local v3         */
		0x06, 0xEC, 0xFF,                    /* 1E jump ->0D            */
		0x25                                 /* 21                      */
	};
	compiles_back(code, sizeof code, "", 0U, 4U, 0, NULL, NULL, "Foreach v3 in v0 {",
			"Foreach with its hidden slots");
}

/* A conversation: its key test, a print, and the jump back to the question. */
static void test_converse(void) {
	static const uint8_t code[] = {
		0x04, 0x0F, 0x00,              /* 00 ask ->12             */
		0x1D, 0x00, 0x00,              /* 03 push_data "name"     */
		0x07, 0x01, 0x00, 0x04, 0x00,  /* 06 compare 1 ->0F       */
		0x1C, 0x05, 0x00,              /* 0B append "I am X"      */
		0x33,                          /* 0E say                  */
		0x06, 0xEE, 0xFF,              /* 0F jump ->00            */
		0x25                           /* 12                      */
	};
	compiles_back(code, sizeof code, "name\0I am X", 12U, 0U, 0, NULL, NULL, "[I am X];",
			"converse with a key and a print");
}

/* Names resolve through a header in Origin's index format. */
static void test_names(void) {
	static const uint8_t code[] = {
		0x42, 0x0A, 0x00,              /* push_flag 10            */
		0x05, 0x0A, 0x00,              /* jump_if_false ->10      */
		0x1F, 0x07, 0x00,              /* push_word 7             */
		0x47, 0x01, 0x04,              /* call the usable 0x0401  */
		0x13, 0x43, 0x0A, 0x00,        /* $MetIolo = true         */
		0x25
	};
	static const char *functions[65536];
	static const char *flags[65536];
	functions[0x0401] = "Iolo";
	flags[10] = "MetIolo";
	U7Names names = {0};
	names.functions = functions;
	names.flags = flags;

	static const char header[] =
			"// A header in Origin's index format.\n"
			"croutine SetSpeaker;\n"
			"usableindex #Iolo 1025;\n"
			"global flag $MetIolo = False; //flag number 10\n"
			"constant _Avatar 65180;\n"
			"action _aWAIT_i 39;\n";
	U7NameTable table = {0};
	check(u7_names_read_header(&table, header, sizeof header - 1U, NULL) == U7_OK,
			"the header reads");
	int32_t value = 0;
	check(u7_names_lookup(&table, U7_NAME_INTRINSIC, "SetSpeaker", 10U, &value) && value == 0,
			"engine calls numbered in order");
	check(u7_names_lookup(&table, U7_NAME_ACTION, "_aWAIT_i", 8U, &value) && value == 39,
			"an action code");
	check(!u7_names_lookup(&table, U7_NAME_CONSTANT, "_aWAIT_i", 8U, &value),
			"an action is not a word constant");
	compiles_back(code, sizeof code, "", 0U, 0U, 0x0401U, &names, &table, "Iolo(7);",
			"names from the header");
	u7_names_release(&table);
}

int main(void) {
	test_else_after_abort();
	test_short_circuit_and();
	test_loops();
	test_foreach();
	test_converse();
	test_names();
	if (failures == 0) {
		printf("PASS test_compile\n");
	}
	return failures == 0 ? 0 : 1;
}
