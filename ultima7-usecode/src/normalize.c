#include "u7/normalize.h"

#include <stdlib.h>
#include <string.h>

void u7_canonical_release(U7Canonical *canonical) {
	if (canonical == NULL) {
		return;
	}
	free(canonical->bytes);
	canonical->bytes = NULL;
	canonical->size = 0;
	canonical->capacity = 0;
}

static bool u7_canonical_reserve(U7Canonical *canonical, size_t extra) {
	if (canonical->size + extra <= canonical->capacity) {
		return true;
	}
	size_t capacity = canonical->capacity == 0 ? 256U : canonical->capacity;
	while (capacity < canonical->size + extra) {
		capacity *= 2U;
	}
	uint8_t *bytes = realloc(canonical->bytes, capacity);
	if (bytes == NULL) {
		return false;
	}
	canonical->bytes = bytes;
	canonical->capacity = capacity;
	return true;
}

static bool u7_canonical_put(U7Canonical *canonical, const void *bytes, size_t size) {
	if (!u7_canonical_reserve(canonical, size)) {
		return false;
	}
	memcpy(canonical->bytes + canonical->size, bytes, size);
	canonical->size += size;
	return true;
}

static bool u7_canonical_put_u32(U7Canonical *canonical, uint32_t value) {
	uint8_t encoded[4] = {
		(uint8_t) (value & 0xFFU), (uint8_t) ((value >> 8) & 0xFFU),
		(uint8_t) ((value >> 16) & 0xFFU), (uint8_t) ((value >> 24) & 0xFFU)
	};
	return u7_canonical_put(canonical, encoded, sizeof encoded);
}

U7Result u7_canonical_body(const U7Function *function, U7Canonical *out_canonical) {
	if (function == NULL || out_canonical == NULL) {
		return U7_ERROR_ARGUMENT;
	}
	out_canonical->size = 0;

	/* Map each code byte offset to its position among surviving instructions,
	 * so a jump into a dropped debug record still lands somewhere stable. */
	uint32_t *index_of = calloc(function->code_size + 1U, sizeof *index_of);
	if (index_of == NULL) {
		return U7_ERROR_CAPACITY;
	}

	uint32_t surviving = 0;
	size_t code_at = 0;
	while (code_at < function->code_size) {
		U7Instruction instruction;
		U7Result result = u7_decode_instruction(function->code,
				function->code_size, code_at, &instruction);
		if (result != U7_OK) {
			free(index_of);
			return result;
		}
		bool debug = u7_opcode_is_debug(instruction.info->opcode);
		for (size_t i = 0; i < instruction.encoded_size; ++i) {
			index_of[code_at + i] = surviving;
		}
		if (!debug) {
			surviving += 1U;
		}
		code_at += instruction.encoded_size;
	}
	index_of[function->code_size] = surviving;

	code_at = 0;
	while (code_at < function->code_size) {
		U7Instruction instruction;
		U7Result result = u7_decode_instruction(function->code,
				function->code_size, code_at, &instruction);
		if (result != U7_OK) {
			free(index_of);
			return result;
		}
		if (u7_opcode_is_debug(instruction.info->opcode)) {
			code_at += instruction.encoded_size;
			continue;
		}

		if (!u7_canonical_put(out_canonical, &instruction.info->opcode, 1U)) {
			free(index_of);
			return U7_ERROR_CAPACITY;
		}

		for (size_t i = 0; i < instruction.info->operand_count; ++i) {
			int32_t value = 0;
			result = u7_instruction_operand(&instruction, i, &value);
			if (result != U7_OK) {
				free(index_of);
				return result;
			}

			bool ok = true;
			switch (instruction.info->operands[i].kind) {
				case U7_OPERAND_RELATIVE_JUMP: {
					size_t next = code_at + instruction.encoded_size;
					int64_t target = (int64_t) next + value;
					if (target < 0 || (size_t) target > function->code_size) {
						free(index_of);
						return U7_ERROR_RANGE;
					}
					ok = u7_canonical_put_u32(out_canonical,
							index_of[(size_t) target]);
					break;
				}
				case U7_OPERAND_DATA_OFFSET: {
					const char *text = NULL;
					size_t length = 0;
					if (u7_function_string(function, (size_t) value, &text,
							&length) != U7_OK) {
						free(index_of);
						return U7_ERROR_RANGE;
					}
					ok = u7_canonical_put(out_canonical, text, length + 1U);
					break;
				}
				case U7_OPERAND_LINK_INDEX: {
					uint16_t target_id = 0;
					if (u7_function_link(function, (size_t) value, &target_id) != U7_OK) {
						free(index_of);
						return U7_ERROR_RANGE;
					}
					ok = u7_canonical_put_u32(out_canonical, target_id);
					break;
				}
				default:
					ok = u7_canonical_put_u32(out_canonical, (uint32_t) value);
					break;
			}
			if (!ok) {
				free(index_of);
				return U7_ERROR_CAPACITY;
			}
		}
		code_at += instruction.encoded_size;
	}

	free(index_of);
	return U7_OK;
}
