#include "u7/debug_records.h"
#include "u7/normalize.h"

#include <stdio.h>
#include <string.h>

static int failures;

static void check(bool condition, const char *what) {
	if (!condition) {
		printf("FAIL %s\n", what);
		failures += 1;
	}
}

/* Builds one classic record around a caller-supplied data segment and code. */
static size_t build_record(uint8_t *out, uint16_t function_id, const char *data,
		size_t data_size, uint16_t locals, const uint8_t *code, size_t code_size) {
	size_t payload = 2U + data_size + 6U + code_size;
	u7_write_u16(out, function_id);
	u7_write_u16(out + 2U, (uint16_t) payload);
	u7_write_u16(out + 4U, (uint16_t) data_size);
	memcpy(out + 6U, data, data_size);
	size_t at = 6U + data_size;
	u7_write_u16(out + at, 0U);
	u7_write_u16(out + at + 2U, locals);
	u7_write_u16(out + at + 4U, 0U);
	memcpy(out + at + 6U, code, code_size);
	return 4U + payload;
}

/*
 * The same routine with and without debug records must normalize identically:
 *
 *   push_byte 1 / jump_if_false -> return / push_data "hi" / return
 */
static void test_debug_records_do_not_change_the_body(void) {
	static const uint8_t plain_code[] = {
		0x44, 0x01,
		0x05, 0x03, 0x00,
		0x1D, 0x00, 0x00,
		0x25
	};
	static const uint8_t debug_code[] = {
		0x4C, 0x0A, 0x00,
		0x44, 0x01,
		0x05, 0x09, 0x00,
		0x4C, 0x0B, 0x00,
		0x1D, 0x00, 0x00,
		0x4C, 0x0C, 0x00,
		0x25
	};

	uint8_t plain[128];
	uint8_t marked[128];
	size_t plain_size = build_record(plain, 0x0096U, "hi", 3U, 1U, plain_code,
			sizeof plain_code);
	size_t marked_size = build_record(marked, 0x0096U, "hi", 3U, 1U, debug_code,
			sizeof debug_code);

	U7Function plain_function;
	U7Function marked_function;
	check(u7_parse_function(plain, plain_size, 0, &plain_function) == U7_OK,
			"plain record parses");
	check(u7_parse_function(marked, marked_size, 0, &marked_function) == U7_OK,
			"marked record parses");

	U7Canonical left = {0};
	U7Canonical right = {0};
	check(u7_canonical_body(&plain_function, &left) == U7_OK, "plain normalizes");
	check(u7_canonical_body(&marked_function, &right) == U7_OK, "marked normalizes");
	check(left.size == right.size && memcmp(left.bytes, right.bytes, left.size) == 0,
			"debug records do not change the canonical body");

	/* The jump must resolve to the return, the fourth surviving instruction. */
	check(left.size == 1U + 4U + 1U + 4U + 1U + 3U + 1U, "canonical body length");
	check(left.bytes[6] == 3U, "jump resolves to the surviving return");

	U7DebugLine lines[8];
	size_t line_count = 0;
	check(u7_debug_lines(&marked_function, lines, 8U, &line_count) == U7_OK &&
			line_count == 3U, "three source line markers");
	check(lines[0].line == 10U && lines[1].line == 11U && lines[2].line == 12U,
			"line numbers in order");

	u7_canonical_release(&left);
	u7_canonical_release(&right);
}

/* dbgfunc addresses one name per local slot, packed back to back. */
static void test_local_names(void) {
	static const uint8_t code[] = {
		0x4D, 0x00, 0x00, 0x05, 0x00,
		0x25
	};
	static const char data[] = "&MAP\0frame\0check";

	uint8_t record[128];
	size_t size = build_record(record, 0x00B2U, data, sizeof data, 2U, code,
			sizeof code);

	U7Function function;
	check(u7_parse_function(record, size, 0, &function) == U7_OK, "record parses");

	U7DebugFunction debug;
	check(u7_debug_read(&function, &debug) == U7_OK, "debug records read");
	check(debug.module_length == 3U && memcmp(debug.module, "MAP", 3U) == 0,
			"module name without its marker");
	check(debug.names_complete, "one name per local slot");

	const char *text = NULL;
	size_t length = 0;
	check(u7_debug_local_name(&function, &debug, 0, &text, &length) == U7_OK &&
			length == 5U && memcmp(text, "frame", 5U) == 0, "slot 0 name");
	check(u7_debug_local_name(&function, &debug, 1, &text, &length) == U7_OK &&
			length == 5U && memcmp(text, "check", 5U) == 0, "slot 1 name");
	check(u7_debug_local_name(&function, &debug, 2, &text, &length) == U7_ERROR_RANGE,
			"slot past the local count is refused");
}

/* A function without debug records reports none rather than inventing them. */
static void test_plain_function_has_no_debug_records(void) {
	static const uint8_t code[] = {0x25};
	uint8_t record[64];
	size_t size = build_record(record, 0x0001U, "", 1U, 0U, code, sizeof code);

	U7Function function;
	check(u7_parse_function(record, size, 0, &function) == U7_OK, "record parses");

	U7DebugFunction debug;
	check(u7_debug_read(&function, &debug) == U7_END, "no debug records");
}

int main(void) {
	test_debug_records_do_not_change_the_body();
	test_local_names();
	test_plain_function_has_no_debug_records();
	if (failures == 0) {
		printf("test_normalize: all checks passed\n");
	}
	return failures == 0 ? 0 : 1;
}
