#include "u7/generate.h"

#include <stdio.h>
#include <string.h>

static int failures;

static void check(bool condition, const char *what) {
	if (!condition) {
		printf("FAIL %s\n", what);
		failures += 1;
	}
}

static size_t build_record(uint8_t *out, uint16_t locals, const uint8_t *code,
		size_t code_size) {
	size_t payload = 2U + 6U + code_size;
	u7_write_u16(out, 0x0100U);
	u7_write_u16(out + 2U, (uint16_t) payload);
	u7_write_u16(out + 4U, 0U);
	u7_write_u16(out + 6U, 0U);
	u7_write_u16(out + 8U, locals);
	u7_write_u16(out + 10U, 0U);
	memcpy(out + 12U, code, code_size);
	return 4U + payload;
}

static bool resolve_none(void *context, uint16_t function_id, U7CallTarget *out) {
	(void) context;
	(void) function_id;
	out->argument_count = 0;
	out->returns_value = false;
	return true;
}

/* Compiles the function back from its structure alone and compares. */
static void compiles_back(const uint8_t *code, size_t code_size, uint16_t locals,
		U7RegionKind expected, const char *what) {
	static uint8_t record[512];
	static uint8_t rebuilt[512];
	size_t size = build_record(record, locals, code, code_size);

	U7Function function;
	U7Cfg cfg;
	U7Lifted lifted;
	U7Structured structured;
	check(u7_parse_function(record, size, 0, &function) == U7_OK, "record parses");
	check(u7_cfg_build(&function, &cfg) == U7_OK, "graph builds");
	check(u7_lift_function(&function, &cfg, resolve_none, NULL, &lifted) == U7_OK,
			"lift runs");
	check(u7_structure_function(&function, &cfg, &lifted, &structured) == U7_OK,
			"structure runs");

	bool found = false;
	for (size_t i = 0; i < structured.root->child_count; ++i) {
		found = found || structured.root->children[i]->kind == expected;
	}
	check(found && structured.goto_count == 0, what);

	U7LowerReport report;
	check(u7_generate_function(&function, &cfg, &lifted, &structured, rebuilt,
			sizeof rebuilt, &report) == U7_OK, "generation runs");
	if (!report.identical) {
		printf("  %s: produced %zu bytes, original %zu, first difference +0x%zX\n",
				what, report.code_size, function.record_size, report.first_difference);
	}
	check(report.identical, what);

	u7_structured_release(&structured);
	u7_lifted_release(&lifted);
	u7_cfg_release(&cfg);
}

/* The jump past an else follows a then-arm that aborts, though nothing reaches it. */
static void test_else_after_abort(void) {
	static const uint8_t code[] = {
		0x48,                   /* push_event          */
		0x44, 0x01,             /* push_byte 1         */
		0x22,                   /* equal               */
		0x05, 0x04, 0x00,       /* jump_if_false ->0B  */
		0x3F,                   /* abort               */
		0x06, 0x06, 0x00,       /* jump ->11           */
		0x1F, 0x07, 0x00,       /* push_word 7         */
		0x12, 0x00, 0x00,       /* pop_local v0        */
		0x25                    /* return              */
	};
	compiles_back(code, sizeof code, 1U, U7_REGION_IF_ELSE,
			"if/else whose first arm aborts");
}

/* Two tests failing to one else: a short-circuit and. */
static void test_short_circuit_and(void) {
	static const uint8_t code[] = {
		0x48,                   /* push_event          */
		0x44, 0x01,             /* push_byte 1         */
		0x22,                   /* equal               */
		0x05, 0x0F, 0x00,       /* jump_if_false ->16  */
		0x21, 0x00, 0x00,       /* push_local v0       */
		0x05, 0x09, 0x00,       /* jump_if_false ->16  */
		0x1F, 0x07, 0x00,       /* push_word 7         */
		0x12, 0x01, 0x00,       /* pop_local v1        */
		0x06, 0x06, 0x00,       /* jump ->1C           */
		0x1F, 0x08, 0x00,       /* push_word 8         */
		0x12, 0x01, 0x00,       /* pop_local v1        */
		0x25                    /* return              */
	};
	compiles_back(code, sizeof code, 2U, U7_REGION_IF_ELSE, "a && b with an else");
}

/* A loop tested at the bottom. */
static void test_do_while(void) {
	static const uint8_t code[] = {
		0x44, 0x00,             /* push_byte 0         */
		0x12, 0x00, 0x00,       /* pop_local v0        */
		0x21, 0x00, 0x00,       /* push_local v0       */
		0x44, 0x01,             /* push_byte 1         */
		0x09,                   /* add                 */
		0x12, 0x00, 0x00,       /* pop_local v0        */
		0x21, 0x00, 0x00,       /* push_local v0       */
		0x44, 0x05,             /* push_byte 5         */
		0x17,                   /* less_than           */
		0x05, 0x03, 0x00,       /* jump_if_false ->1A  */
		0x06, 0xEB, 0xFF,       /* jump ->05           */
		0x25                    /* return              */
	};
	compiles_back(code, sizeof code, 1U, U7_REGION_DO_WHILE, "do-while");
}

int main(void) {
	test_else_after_abort();
	test_short_circuit_and();
	test_do_while();
	if (failures == 0) {
		printf("PASS test_generate\n");
	}
	return failures == 0 ? 0 : 1;
}
