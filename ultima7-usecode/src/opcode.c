#include "u7/opcode.h"

#include <string.h>

#define U7_OPERAND(operand_kind, offset, width) \
	{.kind = (operand_kind), .byte_offset = (offset), .byte_width = (width)}

static const U7OpcodeInfo U7_OPCODES[] = {
	{0x02, "loop", 11, 5, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_LOCAL_INDEX, 1, 2),
		 U7_OPERAND(U7_OPERAND_LOCAL_INDEX, 3, 2),
		 U7_OPERAND(U7_OPERAND_LOCAL_INDEX, 5, 2),
		 U7_OPERAND(U7_OPERAND_LOCAL_INDEX, 7, 2),
		 U7_OPERAND(U7_OPERAND_RELATIVE_JUMP, 9, 2)}},
	{0x04, "ask", 3, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_RELATIVE_JUMP, 1, 2)}},
	{0x05, "jump_if_false", 3, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_RELATIVE_JUMP, 1, 2)}},
	{0x06, "jump", 3, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_RELATIVE_JUMP, 1, 2)}},
	{0x07, "compare_answers", 5, 2, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_U16, 1, 2),
		 U7_OPERAND(U7_OPERAND_RELATIVE_JUMP, 3, 2)}},
	{0x09, "add", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x0A, "subtract", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x0B, "divide", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x0C, "multiply", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x0D, "modulo", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x0E, "and", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x0F, "or", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x10, "not", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x12, "pop_local", 3, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_LOCAL_INDEX, 1, 2)}},
	{0x13, "push_true", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x14, "push_false", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x16, "greater_than", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x17, "less_than", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x18, "greater_equal", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x19, "less_equal", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x1A, "not_equal", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x1C, "append_data", 3, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_DATA_OFFSET, 1, 2)}},
	{0x1D, "push_data", 3, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_DATA_OFFSET, 1, 2)}},
	{0x1E, "make_array", 3, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_U16, 1, 2)}},
	{0x1F, "push_word", 3, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_U16, 1, 2)}},
	{0x21, "push_local", 3, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_LOCAL_INDEX, 1, 2)}},
	{0x22, "equal", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x24, "call_link", 3, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_LINK_INDEX, 1, 2)}},
	{0x25, "return", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x26, "array_get", 3, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_LOCAL_INDEX, 1, 2)}},
	/* The reference explicitly marks EXIT2 semantics as needing verification. */
	{0x2C, "exit2", 1, 0, U7_CONFIDENCE_TENTATIVE, {{0}}},
	{0x2D, "set_return", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x2E, "start_loop", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x2F, "append_local", 3, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_LOCAL_INDEX, 1, 2)}},
	{0x30, "in", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	/* Its only two uses pass 4 in the second operand inside a function with
	 * two locals, so the local-index reading cannot be right. Read as a
	 * branch it skips a default reply once some answer has been handled. */
	{0x31, "conversation_default", 5, 2, U7_CONFIDENCE_TENTATIVE,
		{U7_OPERAND(U7_OPERAND_U16, 1, 2),
		 U7_OPERAND(U7_OPERAND_RELATIVE_JUMP, 3, 2)}},
	{0x32, "return_result", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x33, "say", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x38, "call_intrinsic_result", 4, 2, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_INTRINSIC_ID, 1, 2),
		 U7_OPERAND(U7_OPERAND_ARGUMENT_COUNT, 3, 1)}},
	{0x39, "call_intrinsic", 4, 2, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_INTRINSIC_ID, 1, 2),
		 U7_OPERAND(U7_OPERAND_ARGUMENT_COUNT, 3, 1)}},
	{0x3E, "push_item_ref", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	/* The reference explicitly marks EXIT semantics as needing verification. */
	{0x3F, "exit", 1, 0, U7_CONFIDENCE_TENTATIVE, {{0}}},
	/* The reference explicitly marks answer clearing as needing verification. */
	{0x40, "clear_answers", 1, 0, U7_CONFIDENCE_TENTATIVE, {{0}}},
	{0x42, "push_flag", 3, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_FLAG_INDEX, 1, 2)}},
	{0x43, "pop_flag", 3, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_FLAG_INDEX, 1, 2)}},
	{0x44, "push_byte", 2, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_U8, 1, 1)}},
	{0x46, "array_put", 3, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_LOCAL_INDEX, 1, 2)}},
	{0x47, "call_function", 3, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_FUNCTION_ID, 1, 2)}},
	{0x48, "push_event", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x4A, "array_append", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	{0x4B, "pop_event", 1, 0, U7_CONFIDENCE_DOCUMENTED, {{0}}},
	/* Emitted by the Serpent Isle beta only. The line number is the original
	 * source line; the data offset addresses one name per local slot. */
	{0x4C, "dbgline", 3, 1, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_SOURCE_LINE, 1, 2)}},
	{0x4D, "dbgfunc", 5, 2, U7_CONFIDENCE_DOCUMENTED,
		{U7_OPERAND(U7_OPERAND_U16, 1, 2),
		 U7_OPERAND(U7_OPERAND_DATA_OFFSET, 3, 2)}},
};

const U7OpcodeInfo *u7_opcode_info(uint8_t opcode) {
	for (size_t i = 0; i < sizeof U7_OPCODES / sizeof U7_OPCODES[0]; ++i) {
		if (U7_OPCODES[i].opcode == opcode) {
			return &U7_OPCODES[i];
		}
	}
	return NULL;
}

bool u7_opcode_is_debug(uint8_t opcode) {
	return opcode == 0x4CU || opcode == 0x4DU;
}

U7Result u7_decode_instruction(const uint8_t *code, size_t code_size,
		size_t offset, U7Instruction *out_instruction) {
	if (out_instruction != NULL) {
		memset(out_instruction, 0, sizeof *out_instruction);
	}
	if (code == NULL || out_instruction == NULL) {
		return U7_ERROR_ARGUMENT;
	}
	if (offset == code_size) {
		return U7_END;
	}
	if (offset > code_size) {
		return U7_ERROR_ARGUMENT;
	}

	const U7OpcodeInfo *info = u7_opcode_info(code[offset]);
	if (info == NULL) {
		return U7_ERROR_UNKNOWN_OPCODE;
	}
	if (code_size - offset < info->encoded_size) {
		return U7_ERROR_TRUNCATED;
	}

	out_instruction->offset = offset;
	out_instruction->encoded = code + offset;
	out_instruction->encoded_size = info->encoded_size;
	out_instruction->info = info;
	return U7_OK;
}

U7Result u7_instruction_operand(const U7Instruction *instruction,
		size_t operand_index, int32_t *out_value) {
	if (out_value != NULL) {
		*out_value = 0;
	}
	if (instruction == NULL || out_value == NULL || instruction->info == NULL) {
		return U7_ERROR_ARGUMENT;
	}
	if (operand_index >= instruction->info->operand_count) {
		return U7_ERROR_RANGE;
	}

	const U7OperandSpec *spec = &instruction->info->operands[operand_index];
	if (spec->byte_width == 1U) {
		*out_value = (int32_t) instruction->encoded[spec->byte_offset];
		return U7_OK;
	}

	uint16_t raw = (uint16_t) ((uint16_t) instruction->encoded[spec->byte_offset] |
			((uint16_t) instruction->encoded[spec->byte_offset + 1U] << 8));
	*out_value = spec->kind == U7_OPERAND_RELATIVE_JUMP
			? (int32_t) (int16_t) raw
			: (int32_t) raw;
	return U7_OK;
}

U7Result u7_encode_instruction(uint8_t opcode, const int32_t *operands,
		size_t operand_count, uint8_t *output, size_t output_capacity,
		size_t *out_size) {
	if (out_size != NULL) {
		*out_size = 0;
	}
	if (output == NULL || out_size == NULL || (operands == NULL && operand_count > 0)) {
		return U7_ERROR_ARGUMENT;
	}

	const U7OpcodeInfo *info = u7_opcode_info(opcode);
	if (info == NULL) {
		return U7_ERROR_UNKNOWN_OPCODE;
	}
	if (operand_count != info->operand_count) {
		return U7_ERROR_ARGUMENT;
	}
	if (output_capacity < info->encoded_size) {
		return U7_ERROR_CAPACITY;
	}

	/* Validate every field before touching the caller's buffer. */
	for (size_t i = 0; i < operand_count; ++i) {
		const U7OperandSpec *spec = &info->operands[i];
		int32_t value = operands[i];
		if (spec->kind == U7_OPERAND_RELATIVE_JUMP) {
			if (value < INT16_MIN || value > INT16_MAX) {
				return U7_ERROR_RANGE;
			}
		} else {
			int32_t limit = spec->byte_width == 1U ? 0xFF : 0xFFFF;
			if (value < 0 || value > limit) {
				return U7_ERROR_RANGE;
			}
		}
	}

	uint8_t encoded[16];
	memset(encoded, 0, sizeof encoded);
	encoded[0] = opcode;
	for (size_t i = 0; i < operand_count; ++i) {
		const U7OperandSpec *spec = &info->operands[i];
		if (spec->byte_width == 1U) {
			encoded[spec->byte_offset] = (uint8_t) operands[i];
		} else {
			u7_write_u16(&encoded[spec->byte_offset], (uint16_t) operands[i]);
		}
	}

	memcpy(output, encoded, info->encoded_size);
	*out_size = info->encoded_size;
	return U7_OK;
}
