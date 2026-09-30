#include "u7/debug_records.h"

#include <string.h>

enum { U7_OPCODE_DBGLINE = 0x4CU, U7_OPCODE_DBGFUNC = 0x4DU };

static U7Result u7_debug_leading(const U7Function *function,
		U7Instruction *out_instruction) {
	U7Result result = u7_decode_instruction(function->code, function->code_size,
			0, out_instruction);
	if (result != U7_OK) {
		return result;
	}
	if (out_instruction->info->opcode != U7_OPCODE_DBGFUNC) {
		return U7_END;
	}
	return U7_OK;
}

U7Result u7_debug_read(const U7Function *function, U7DebugFunction *out_debug) {
	if (out_debug != NULL) {
		memset(out_debug, 0, sizeof *out_debug);
	}
	if (function == NULL || out_debug == NULL) {
		return U7_ERROR_ARGUMENT;
	}

	U7Instruction dbgfunc;
	U7Result result = u7_debug_leading(function, &dbgfunc);
	if (result != U7_OK) {
		return result;
	}

	int32_t word = 0;
	int32_t name_offset = 0;
	if (u7_instruction_operand(&dbgfunc, 0, &word) != U7_OK ||
			u7_instruction_operand(&dbgfunc, 1, &name_offset) != U7_OK) {
		return U7_ERROR_TRUNCATED;
	}

	out_debug->function_id = function->function_id;
	out_debug->local_count = function->local_count;
	out_debug->name_offset = (size_t) name_offset;
	out_debug->dbgfunc_word = (uint16_t) word;

	const char *module = NULL;
	size_t module_length = 0;
	if (u7_function_string(function, 0, &module, &module_length) == U7_OK &&
			module_length > 1U && module[0] == '&') {
		out_debug->module = module + 1;
		out_debug->module_length = module_length - 1U;
	}

	/* Names run back to back from the dbgfunc offset, one per local slot. */
	out_debug->names_complete = true;
	size_t at = out_debug->name_offset;
	for (uint16_t slot = 0; slot < function->local_count; ++slot) {
		const char *text = NULL;
		size_t length = 0;
		if (u7_function_string(function, at, &text, &length) != U7_OK) {
			out_debug->names_complete = false;
			break;
		}
		at += length + 1U;
	}

	size_t code_at = 0;
	bool have_line = false;
	while (code_at < function->code_size) {
		U7Instruction instruction;
		if (u7_decode_instruction(function->code, function->code_size, code_at,
				&instruction) != U7_OK) {
			break;
		}
		if (instruction.info->opcode == U7_OPCODE_DBGLINE) {
			int32_t line = 0;
			if (u7_instruction_operand(&instruction, 0, &line) == U7_OK) {
				if (!have_line) {
					out_debug->first_line = (uint16_t) line;
					have_line = true;
				}
				out_debug->last_line = (uint16_t) line;
				out_debug->line_count += 1U;
			}
		}
		code_at += instruction.encoded_size;
	}
	return U7_OK;
}

U7Result u7_debug_local_name(const U7Function *function,
		const U7DebugFunction *debug, uint16_t slot, const char **out_text,
		size_t *out_length) {
	if (out_text != NULL) {
		*out_text = NULL;
	}
	if (out_length != NULL) {
		*out_length = 0;
	}
	if (function == NULL || debug == NULL || out_text == NULL || out_length == NULL) {
		return U7_ERROR_ARGUMENT;
	}
	if (slot >= debug->local_count) {
		return U7_ERROR_RANGE;
	}

	size_t at = debug->name_offset;
	for (uint16_t i = 0; i < slot; ++i) {
		const char *text = NULL;
		size_t length = 0;
		U7Result result = u7_function_string(function, at, &text, &length);
		if (result != U7_OK) {
			return result;
		}
		at += length + 1U;
	}
	return u7_function_string(function, at, out_text, out_length);
}

U7Result u7_debug_lines(const U7Function *function, U7DebugLine *out_lines,
		size_t capacity, size_t *out_count) {
	if (out_count != NULL) {
		*out_count = 0;
	}
	if (function == NULL || out_count == NULL || (out_lines == NULL && capacity > 0)) {
		return U7_ERROR_ARGUMENT;
	}

	size_t count = 0;
	size_t code_at = 0;
	while (code_at < function->code_size) {
		U7Instruction instruction;
		U7Result result = u7_decode_instruction(function->code,
				function->code_size, code_at, &instruction);
		if (result != U7_OK) {
			return result;
		}
		if (instruction.info->opcode == U7_OPCODE_DBGLINE) {
			int32_t line = 0;
			if (u7_instruction_operand(&instruction, 0, &line) != U7_OK) {
				return U7_ERROR_TRUNCATED;
			}
			if (count < capacity) {
				out_lines[count].code_offset = code_at + instruction.encoded_size;
				out_lines[count].line = (uint16_t) line;
			}
			count += 1U;
		}
		code_at += instruction.encoded_size;
	}

	*out_count = count;
	return count > capacity ? U7_ERROR_CAPACITY : U7_OK;
}
