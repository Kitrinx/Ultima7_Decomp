#include "u7/stack_flow.h"

#include <stdio.h>
#include <string.h>

static int failures;

static void check(bool condition, const char *what) {
	if (!condition) {
		printf("FAIL %s\n", what);
		failures += 1;
	}
}

static size_t build_record(uint8_t *out, uint16_t function_id, const char *data,
		size_t data_size, uint16_t args, uint16_t locals, const uint8_t *code,
		size_t code_size) {
	size_t payload = 2U + data_size + 6U + code_size;
	u7_write_u16(out, function_id);
	u7_write_u16(out + 2U, (uint16_t) payload);
	u7_write_u16(out + 4U, (uint16_t) data_size);
	memcpy(out + 6U, data, data_size);
	size_t at = 6U + data_size;
	u7_write_u16(out + at, args);
	u7_write_u16(out + at + 2U, locals);
	u7_write_u16(out + at + 4U, 0U);
	memcpy(out + at + 6U, code, code_size);
	return 4U + payload;
}

static U7Result analyze(const uint8_t *code, size_t code_size, uint16_t locals,
		U7StackReport *out_report, size_t *out_blocks) {
	static uint8_t record[512];
	size_t size = build_record(record, 1U, "x", 2U, 0U, locals, code, code_size);

	U7Function function;
	U7Result result = u7_parse_function(record, size, 0, &function);
	if (result != U7_OK) {
		return result;
	}

	U7Cfg cfg;
	result = u7_cfg_build(&function, &cfg);
	if (result != U7_OK) {
		return result;
	}
	if (out_blocks != NULL) {
		*out_blocks = cfg.block_count;
	}
	result = u7_stack_depths(&function, &cfg, NULL, NULL, out_report);
	u7_cfg_release(&cfg);
	return result;
}

/* A conditional splits into three blocks and both arms rejoin at one depth. */
static void test_conditional_balances(void) {
	static const uint8_t code[] = {
		0x44, 0x01,             /* push_byte 1        */
		0x05, 0x06, 0x00,       /* jump_if_false ->11 */
		0x1F, 0x07, 0x00,       /* push_word 7        */
		0x12, 0x00, 0x00,       /* pop_local L0       */
		0x25                    /* return             */
	};
	U7StackReport report;
	size_t blocks = 0;
	check(analyze(code, sizeof code, 1U, &report, &blocks) == U7_OK, "analysis runs");
	check(blocks == 3U, "three basic blocks");
	check(report.analyzed && report.balanced, "conditional balances");
	check(report.max_depth == 1U, "peak depth of one");
	check(report.exit_depth == 0U, "returns at depth zero");
}

/* compare_answers consumes as many answers as its count operand names. */
static void test_compare_answers_uses_its_count(void) {
	static const uint8_t code[] = {
		0x1D, 0x00, 0x00,       /* push_data           */
		0x1D, 0x00, 0x00,       /* push_data           */
		0x07, 0x02, 0x00,       /* compare_answers 2   */
		0x00, 0x00,             /*   falls through     */
		0x25                    /* return              */
	};
	U7StackReport report;
	check(analyze(code, sizeof code, 0U, &report, NULL) == U7_OK, "analysis runs");
	check(report.analyzed && report.balanced, "count of two balances");

	/* The same code claiming one answer leaves a value stranded. */
	uint8_t wrong[sizeof code];
	memcpy(wrong, code, sizeof code);
	wrong[7] = 0x01;
	check(analyze(wrong, sizeof wrong, 0U, &report, NULL) == U7_OK, "analysis runs");
	check(report.analyzed && report.exit_depth == 1U,
			"a count of one strands a value");
}

/* The conversation default branches; it is not a pair of local indices. */
static void test_conversation_default_branches(void) {
	static const uint8_t code[] = {
		0x31, 0x01, 0x00, 0x01, 0x00,  /* default 1 ->0x06 */
		0x25,                          /* return           */
		0x25                           /* return           */
	};
	U7StackReport report;
	size_t blocks = 0;
	check(analyze(code, sizeof code, 0U, &report, &blocks) == U7_OK, "analysis runs");
	check(blocks == 3U, "the default splits the block");
	check(report.analyzed && report.balanced, "default touches no stack value");

	const U7OpcodeInfo *info = u7_opcode_info(0x31U);
	check(info != NULL && info->operand_count == 2U &&
			info->operands[1].kind == U7_OPERAND_RELATIVE_JUMP,
			"second operand is a branch");
}

/* An unresolved call is reported, never assumed to be free. */
static void test_unresolved_call_is_reported(void) {
	static const uint8_t code[] = {
		0x47, 0x34, 0x12,       /* call_function 0x1234 */
		0x25
	};
	U7StackReport report;
	check(analyze(code, sizeof code, 0U, &report, NULL) == U7_OK, "analysis runs");
	check(!report.analyzed && report.blocking_opcode == 0x47U,
			"the call blocks analysis");
}

int main(void) {
	test_conditional_balances();
	test_compare_answers_uses_its_count();
	test_conversation_default_branches();
	test_unresolved_call_is_reported();
	if (failures == 0) {
		printf("test_stack_flow: all checks passed\n");
	}
	return failures == 0 ? 0 : 1;
}
