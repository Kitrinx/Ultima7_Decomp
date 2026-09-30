#include "u7/emit.h"
#include "u7/parse.h"

#include <stdlib.h>
#include <string.h>

static int failures;

static void check(bool condition, const char *what) {
	if (!condition) {
		printf("FAIL %s\n", what);
		failures += 1;
	}
}

static bool resolve_none(void *context, uint16_t function_id, U7CallTarget *out) {
	(void) context;
	(void) function_id;
	out->argument_count = 0;
	out->returns_value = false;
	return true;
}

static size_t build_record(uint8_t *out, const char *data, size_t data_size,
		uint16_t locals, const uint8_t *code, size_t code_size) {
	size_t payload = 2U + data_size + 6U + code_size;
	u7_write_u16(out, 0x0200U);
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

/* Bytes to source and back must land on the same bytes. */
static void round_trip(const uint8_t *code, size_t code_size, const char *data,
		size_t data_size, uint16_t locals, const char *what) {
	static uint8_t record[1024];
	static char text[8192];
	static uint8_t rebuilt[1024];

	size_t size = build_record(record, data, data_size, locals, code, code_size);

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

	U7Names names;
	memset(&names, 0, sizeof names);
	names.function = &function;
	names.cfg = &cfg;

	FILE *stream = tmpfile();
	check(stream != NULL, "temporary stream");
	size_t written = 0;
	if (stream != NULL) {
		u7_emit_function(stream, &function, &lifted, &structured, &names,
				U7_EMIT_LINEAR);
		rewind(stream);
		written = fread(text, 1U, sizeof text - 1U, stream);
		text[written] = 0;
		fclose(stream);
	}

	U7Parsed parsed;
	U7Result result = u7_parse_source(text, written, &parsed);
	if (result != U7_OK) {
		printf("  %s: line %zu: %s\n", what, parsed.error_line, parsed.error);
	}
	check(result == U7_OK, "source parses");
	check(parsed.count == 1U, "one function parsed");

	if (result == U7_OK && parsed.count == 1U) {
		U7Unit unit;
		U7LowerReport report;
		u7_parsed_unit(&parsed.functions[0], &unit);
		check(u7_assemble_unit(&unit, rebuilt, sizeof rebuilt, &report) == U7_OK,
				"source assembles");
		bool same = report.code_size == size &&
				memcmp(rebuilt, record, size) == 0;
		if (!same) {
			printf("  %s: produced %zu bytes, original %zu\n", what,
					report.code_size, size);
		}
		check(same, what);
	}

	u7_parsed_release(&parsed);
	u7_structured_release(&structured);
	u7_lifted_release(&lifted);
	u7_cfg_release(&cfg);
}

static void test_conditional(void) {
	static const uint8_t code[] = {
		0x48, 0x44, 0x01, 0x22, 0x05, 0x06, 0x00,
		0x1F, 0x07, 0x00, 0x12, 0x00, 0x00, 0x25
	};
	round_trip(code, sizeof code, "", 0U, 1U, "conditional round trips through source");
}

static void test_strings_and_lists(void) {
	static const uint8_t code[] = {
		0x1D, 0x00, 0x00,       /* push_data "one"   */
		0x13,                   /* push_true         */
		0x1E, 0x02, 0x00,       /* make_array 2      */
		0x12, 0x00, 0x00,       /* pop_local v0      */
		0x1C, 0x04, 0x00,       /* append_data "two" */
		0x33,                   /* say               */
		0x25
	};
	round_trip(code, sizeof code, "one\0two", 8U, 1U,
			"strings and lists round trip through source");
}

static void test_conversation(void) {
	static const uint8_t code[] = {
		0x04, 0x0C, 0x00,       /* ask past the arm     */
		0x1D, 0x00, 0x00,       /* push_data "hi"       */
		0x07, 0x01, 0x00, 0x01, 0x00,  /* answer -> the jump */
		0x33,                   /* say                  */
		0x06, 0xF1, 0xFF,       /* jump back to the ask */
		0x25
	};
	round_trip(code, sizeof code, "hi", 3U, 0U,
			"conversation round trips through source");
}

int main(void) {
	test_conditional();
	test_strings_and_lists();
	test_conversation();
	if (failures == 0) {
		printf("test_parse: all checks passed\n");
	}
	return failures == 0 ? 0 : 1;
}
