#include "u7/lower.h"

#include <stdio.h>
#include <string.h>

static int failures;

static void check(bool condition, const char *what) {
	if (!condition) {
		printf("FAIL %s\n", what);
		failures += 1;
	}
}

static size_t build_record(uint8_t *out, const char *data, size_t data_size,
		uint16_t locals, const uint16_t *links, uint16_t link_count,
		const uint8_t *code, size_t code_size) {
	size_t payload = 2U + data_size + 6U + (size_t) link_count * 2U + code_size;
	u7_write_u16(out, 0x0100U);
	u7_write_u16(out + 2U, (uint16_t) payload);
	u7_write_u16(out + 4U, (uint16_t) data_size);
	memcpy(out + 6U, data, data_size);
	size_t at = 6U + data_size;
	u7_write_u16(out + at, 0U);
	u7_write_u16(out + at + 2U, locals);
	u7_write_u16(out + at + 4U, link_count);
	at += 6U;
	for (uint16_t i = 0; i < link_count; ++i) {
		u7_write_u16(out + at + i * 2U, links[i]);
	}
	memcpy(out + at + (size_t) link_count * 2U, code, code_size);
	return 4U + payload;
}

/* Every callee here takes nothing and returns nothing. */
static bool resolve_none(void *context, uint16_t function_id, U7CallTarget *out) {
	(void) context;
	(void) function_id;
	out->argument_count = 0;
	out->returns_value = false;
	return true;
}

static void round_trip(const uint8_t *code, size_t code_size, const char *data,
		size_t data_size, uint16_t locals, const uint16_t *links,
		uint16_t link_count, const char *what) {
	static uint8_t record[1024];
	static uint8_t rebuilt[1024];
	size_t size = build_record(record, data, data_size, locals, links, link_count,
			code, code_size);

	U7Function function;
	U7Cfg cfg;
	U7Lifted lifted;
	check(u7_parse_function(record, size, 0, &function) == U7_OK, "record parses");
	check(u7_cfg_build(&function, &cfg) == U7_OK, "graph builds");
	check(u7_lift_function(&function, &cfg, resolve_none, NULL, &lifted) == U7_OK,
			"lift runs");

	U7LowerReport report;
	check(u7_lower_record(&function, &cfg, &lifted, rebuilt, sizeof rebuilt,
			&report) == U7_OK, "lowering runs");
	if (!report.identical) {
		printf("  %s: produced %zu bytes, original %zu, first difference +0x%zX\n",
				what, report.code_size, function.record_size, report.first_difference);
	}
	check(report.identical, what);

	u7_lifted_release(&lifted);
	u7_cfg_release(&cfg);
}

/* Branch displacements are recomputed, so a match means they were recovered. */
static void test_branches_round_trip(void) {
	static const uint8_t code[] = {
		0x48,                   /* push_event        */
		0x44, 0x01,             /* push_byte 1       */
		0x22,                   /* equal             */
		0x05, 0x06, 0x00,       /* jump_if_false ->D */
		0x1F, 0x07, 0x00,       /* push_word 7       */
		0x12, 0x00, 0x00,       /* pop_local v0      */
		0x25                    /* return            */
	};
	round_trip(code, sizeof code, "", 0U, 1U, NULL, 0U, "conditional round trips");
}

/*
 * The data segment is laid out afresh from the strings the code names, in the
 * order it names them, which is the layout every shipped function has.
 */
static void test_data_segment_rebuilds(void) {
	static const uint8_t code[] = {
		0x1D, 0x00, 0x00,       /* push_data "one"  */
		0x12, 0x00, 0x00,       /* pop_local v0     */
		0x1D, 0x04, 0x00,       /* push_data "two"  */
		0x12, 0x00, 0x00,       /* pop_local v0     */
		0x1D, 0x00, 0x00,       /* push_data "one"  */
		0x12, 0x00, 0x00,       /* pop_local v0     */
		0x25
	};
	round_trip(code, sizeof code, "one\0two", 8U, 1U, NULL, 0U,
			"data segment rebuilds in reference order");
}

/* A table may name one target twice; the call keeps its own slot. */
static void test_duplicate_link_slots(void) {
	static const uint16_t links[] = {0x0802U, 0x0802U};
	static const uint8_t code[] = {
		0x24, 0x01, 0x00,       /* call_link slot 1 */
		0x24, 0x00, 0x00,       /* call_link slot 0 */
		0x25
	};
	round_trip(code, sizeof code, "", 0U, 0U, links, 2U,
			"duplicate link slots survive");
}

int main(void) {
	test_branches_round_trip();
	test_data_segment_rebuilds();
	test_duplicate_link_slots();
	if (failures == 0) {
		printf("test_lower: all checks passed\n");
	}
	return failures == 0 ? 0 : 1;
}
