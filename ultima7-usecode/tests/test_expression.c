#include "u7/expression.h"

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
		uint16_t locals, const uint8_t *code, size_t code_size) {
	size_t payload = 2U + data_size + 6U + code_size;
	u7_write_u16(out, 1U);
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

/* L1 = 7 + L0, then return. */
static void test_assignment_tree(void) {
	static const uint8_t code[] = {
		0x1F, 0x07, 0x00,       /* push_word 7   */
		0x21, 0x00, 0x00,       /* push_local L0 */
		0x09,                   /* add           */
		0x12, 0x01, 0x00,       /* pop_local L1  */
		0x25                    /* return        */
	};
	uint8_t record[128];
	size_t size = build_record(record, "x", 2U, 2U, code, sizeof code);

	U7Function function;
	check(u7_parse_function(record, size, 0, &function) == U7_OK, "record parses");

	U7Cfg cfg;
	check(u7_cfg_build(&function, &cfg) == U7_OK, "graph builds");

	U7Lifted lifted;
	check(u7_lift_function(&function, &cfg, NULL, NULL, &lifted) == U7_OK, "lift runs");
	check(lifted.complete && lifted.self_contained, "lifted without placeholders");
	check(lifted.block_count == 1U, "one block");
	check(lifted.blocks[0].statement_count == 2U, "two statements");

	const U7Stmt *assign = lifted.blocks[0].first;
	check(assign != NULL && assign->kind == U7_STMT_ASSIGN_LOCAL && assign->slot == 1,
			"assigns local 1");

	const U7Expr *sum = assign != NULL ? assign->value : NULL;
	check(sum != NULL && sum->kind == U7_EXPR_BINARY && sum->opcode == 0x09U &&
			sum->operand_count == 2U, "value is a sum");
	/* Source order must survive: the constant was pushed first. */
	check(sum != NULL && sum->operands[0]->kind == U7_EXPR_CONSTANT &&
			sum->operands[0]->value == 7, "left operand is the constant");
	check(sum != NULL && sum->operands[1]->kind == U7_EXPR_LOCAL &&
			sum->operands[1]->value == 0, "right operand is the local");

	const U7Stmt *ret = assign != NULL ? assign->next : NULL;
	check(ret != NULL && ret->kind == U7_STMT_RETURN, "then returns");

	u7_lifted_release(&lifted);
	u7_cfg_release(&cfg);
}

/* A block entered carrying a value reports it rather than inventing one. */
static void test_incoming_value_is_reported(void) {
	static const uint8_t code[] = {
		0x12, 0x00, 0x00,       /* pop_local L0 with nothing pushed */
		0x25
	};
	uint8_t record[128];
	size_t size = build_record(record, "x", 2U, 1U, code, sizeof code);

	U7Function function;
	U7Cfg cfg;
	U7Lifted lifted;
	check(u7_parse_function(record, size, 0, &function) == U7_OK, "record parses");
	check(u7_cfg_build(&function, &cfg) == U7_OK, "graph builds");
	check(u7_lift_function(&function, &cfg, NULL, NULL, &lifted) == U7_OK, "lift runs");
	check(lifted.complete, "the block still lifts");
	check(!lifted.self_contained && lifted.consumed_incoming == 1U,
			"one value came from a predecessor");

	u7_lifted_release(&lifted);
	u7_cfg_release(&cfg);
}

int main(void) {
	test_assignment_tree();
	test_incoming_value_is_reported();
	if (failures == 0) {
		printf("test_expression: all checks passed\n");
	}
	return failures == 0 ? 0 : 1;
}
