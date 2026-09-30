#ifndef U7_OPCODE_H
#define U7_OPCODE_H

#include "u7/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Confidence describes how well the documented meaning is attested, not
 * runtime parity with the original interpreter. */
typedef enum U7Confidence {
	U7_CONFIDENCE_DOCUMENTED = 0,
	U7_CONFIDENCE_TENTATIVE
} U7Confidence;

typedef enum U7OperandKind {
	U7_OPERAND_U8 = 0,
	U7_OPERAND_U16,
	U7_OPERAND_LOCAL_INDEX,
	U7_OPERAND_RELATIVE_JUMP,
	U7_OPERAND_DATA_OFFSET,
	U7_OPERAND_LINK_INDEX,
	U7_OPERAND_FUNCTION_ID,
	U7_OPERAND_INTRINSIC_ID,
	U7_OPERAND_FLAG_INDEX,
	U7_OPERAND_ARGUMENT_COUNT,
	U7_OPERAND_SOURCE_LINE
} U7OperandKind;

typedef struct U7OperandSpec {
	U7OperandKind kind;
	uint8_t byte_offset;
	uint8_t byte_width;
} U7OperandSpec;

enum { U7_MAX_OPERANDS = 5 };

typedef struct U7OpcodeInfo {
	uint8_t opcode;
	const char *mnemonic;
	uint8_t encoded_size;
	uint8_t operand_count;
	U7Confidence confidence;
	U7OperandSpec operands[U7_MAX_OPERANDS];
} U7OpcodeInfo;

/* NULL for any opcode whose encoded length is not independently known. */
const U7OpcodeInfo *u7_opcode_info(uint8_t opcode);

/* True for the two debug records the Serpent Isle beta emits and retail does
 * not. They carry names and source lines, and execute as no-ops. */
bool u7_opcode_is_debug(uint8_t opcode);

typedef struct U7Instruction {
	size_t offset;
	const uint8_t *encoded;
	uint8_t encoded_size;
	const U7OpcodeInfo *info;
} U7Instruction;

U7Result u7_decode_instruction(const uint8_t *code, size_t code_size,
		size_t offset, U7Instruction *out_instruction);

/* Relative jumps sign-extend; every other operand is unsigned. */
U7Result u7_instruction_operand(const U7Instruction *instruction,
		size_t operand_index, int32_t *out_value);

U7Result u7_encode_instruction(uint8_t opcode, const int32_t *operands,
		size_t operand_count, uint8_t *output, size_t output_capacity,
		size_t *out_size);

#ifdef __cplusplus
}
#endif

#endif
