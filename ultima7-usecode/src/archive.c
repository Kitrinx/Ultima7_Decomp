#include "u7/archive.h"

#include <stdlib.h>
#include <string.h>

enum { U7_RECORD_HEADER_SIZE = 4U, U7_MAX_RECORD_SIZE = 4U + 0xFFFFU };

U7Result u7_parse_function(const uint8_t *bytes, size_t size, size_t offset,
		U7Function *out_function) {
	if (out_function != NULL) {
		memset(out_function, 0, sizeof *out_function);
	}
	if (bytes == NULL || out_function == NULL || offset > size) {
		return U7_ERROR_ARGUMENT;
	}
	if (offset == size) {
		return U7_END;
	}

	uint16_t function_id = 0;
	uint16_t payload_size = 0;
	if (!u7_read_u16(bytes, size, offset, &function_id) ||
			!u7_read_u16(bytes, size, offset + 2U, &payload_size)) {
		return U7_ERROR_TRUNCATED;
	}

	size_t body = offset + U7_RECORD_HEADER_SIZE;
	if (payload_size > size - body) {
		return U7_ERROR_TRUNCATED;
	}
	size_t end = body + payload_size;

	uint16_t data_size = 0;
	if (!u7_read_u16(bytes, end, body, &data_size)) {
		return U7_ERROR_TRUNCATED;
	}
	size_t counts = body + 2U + data_size;
	if (counts < body || counts + 6U > end) {
		return U7_ERROR_TRUNCATED;
	}

	uint16_t argument_count = 0;
	uint16_t local_count = 0;
	uint16_t link_count = 0;
	if (!u7_read_u16(bytes, end, counts, &argument_count) ||
			!u7_read_u16(bytes, end, counts + 2U, &local_count) ||
			!u7_read_u16(bytes, end, counts + 4U, &link_count)) {
		return U7_ERROR_TRUNCATED;
	}

	size_t links_offset = counts + 6U;
	size_t link_bytes = (size_t) link_count * 2U;
	if (link_bytes > end - links_offset) {
		return U7_ERROR_TRUNCATED;
	}
	size_t code_offset = links_offset + link_bytes;

	out_function->function_id = function_id;
	out_function->payload_size = payload_size;
	out_function->record_offset = offset;
	out_function->record_size = U7_RECORD_HEADER_SIZE + payload_size;
	out_function->next_offset = end;
	out_function->data = bytes + body + 2U;
	out_function->data_size = data_size;
	out_function->data_offset = body + 2U;
	out_function->argument_count = argument_count;
	out_function->local_count = local_count;
	out_function->links = bytes + links_offset;
	out_function->link_count = link_count;
	out_function->code = bytes + code_offset;
	out_function->code_size = end - code_offset;
	out_function->code_offset = code_offset;
	return U7_OK;
}

U7Result u7_function_link(const U7Function *function, size_t link_index,
		uint16_t *out_function_id) {
	if (out_function_id != NULL) {
		*out_function_id = 0;
	}
	if (function == NULL || out_function_id == NULL) {
		return U7_ERROR_ARGUMENT;
	}
	if (link_index >= function->link_count) {
		return U7_ERROR_RANGE;
	}
	*out_function_id = (uint16_t) ((uint16_t) function->links[link_index * 2U] |
			((uint16_t) function->links[link_index * 2U + 1U] << 8));
	return U7_OK;
}

U7Result u7_function_string(const U7Function *function, size_t data_offset,
		const char **out_text, size_t *out_length) {
	if (out_text != NULL) {
		*out_text = NULL;
	}
	if (out_length != NULL) {
		*out_length = 0;
	}
	if (function == NULL || out_text == NULL || out_length == NULL) {
		return U7_ERROR_ARGUMENT;
	}
	if (data_offset >= function->data_size) {
		return U7_ERROR_RANGE;
	}

	const uint8_t *start = function->data + data_offset;
	const uint8_t *terminator = memchr(start, 0, function->data_size - data_offset);
	if (terminator == NULL) {
		return U7_ERROR_TRUNCATED;
	}

	*out_text = (const char *) start;
	*out_length = (size_t) (terminator - start);
	return U7_OK;
}

/* Rebuilds one record from its parsed parts, re-encoding every instruction
 * rather than copying the code bytes through. */
static U7Result u7_rebuild_record(const U7Function *function, uint8_t *scratch,
		size_t scratch_size, size_t *out_size, U7ArchiveError *error,
		U7ArchiveStats *stats) {
	size_t need = function->record_size;
	if (need > scratch_size) {
		return U7_ERROR_CAPACITY;
	}

	u7_write_u16(scratch, function->function_id);
	u7_write_u16(scratch + 2U, function->payload_size);
	size_t at = U7_RECORD_HEADER_SIZE;
	u7_write_u16(scratch + at, function->data_size);
	at += 2U;
	memcpy(scratch + at, function->data, function->data_size);
	at += function->data_size;
	u7_write_u16(scratch + at, function->argument_count);
	u7_write_u16(scratch + at + 2U, function->local_count);
	u7_write_u16(scratch + at + 4U, function->link_count);
	at += 6U;
	memcpy(scratch + at, function->links, (size_t) function->link_count * 2U);
	at += (size_t) function->link_count * 2U;

	size_t code_at = 0;
	while (code_at < function->code_size) {
		U7Instruction instruction;
		U7Result result = u7_decode_instruction(function->code,
				function->code_size, code_at, &instruction);
		if (result != U7_OK) {
			error->result = result;
			error->code_offset = code_at;
			error->opcode = function->code[code_at];
			return result;
		}

		int32_t operands[U7_MAX_OPERANDS];
		for (size_t i = 0; i < instruction.info->operand_count; ++i) {
			result = u7_instruction_operand(&instruction, i, &operands[i]);
			if (result != U7_OK) {
				error->result = result;
				error->code_offset = code_at;
				error->opcode = instruction.info->opcode;
				return result;
			}
		}

		size_t written = 0;
		result = u7_encode_instruction(instruction.info->opcode, operands,
				instruction.info->operand_count, scratch + at + code_at,
				need - at - code_at, &written);
		if (result != U7_OK || written != instruction.encoded_size ||
				memcmp(scratch + at + code_at, instruction.encoded, written) != 0) {
			error->result = result == U7_OK ? U7_ERROR_MISMATCH : result;
			error->code_offset = code_at;
			error->opcode = instruction.info->opcode;
			return error->result;
		}

		if (instruction.info->opcode == 0x4CU) {
			stats->debug_line_count += 1U;
		} else if (instruction.info->opcode == 0x4DU) {
			stats->debug_func_count += 1U;
		}
		stats->instruction_count += 1U;
		code_at += instruction.encoded_size;
	}

	*out_size = at + function->code_size;
	return U7_OK;
}

U7Result u7_archive_verify(const uint8_t *bytes, size_t size,
		U7ArchiveStats *out_stats, U7ArchiveError *out_error) {
	U7ArchiveStats stats;
	U7ArchiveError error;
	memset(&stats, 0, sizeof stats);
	memset(&error, 0, sizeof error);
	stats.ids_strictly_increasing = true;

	if (out_stats != NULL) {
		*out_stats = stats;
	}
	if (out_error != NULL) {
		*out_error = error;
	}
	if (bytes == NULL) {
		return U7_ERROR_ARGUMENT;
	}

	uint8_t *scratch = malloc(U7_MAX_RECORD_SIZE);
	if (scratch == NULL) {
		return U7_ERROR_CAPACITY;
	}

	U7Result result = U7_OK;
	size_t offset = 0;
	bool have_previous = false;
	uint16_t previous_id = 0;

	while (offset < size) {
		U7Function function;
		result = u7_parse_function(bytes, size, offset, &function);
		if (result != U7_OK) {
			error.result = result;
			error.record_offset = offset;
			break;
		}

		error.record_offset = offset;
		error.function_id = function.function_id;

		size_t rebuilt = 0;
		result = u7_rebuild_record(&function, scratch, U7_MAX_RECORD_SIZE,
				&rebuilt, &error, &stats);
		if (result != U7_OK) {
			break;
		}
		if (rebuilt != function.record_size ||
				memcmp(scratch, bytes + offset, rebuilt) != 0) {
			result = U7_ERROR_MISMATCH;
			error.result = result;
			break;
		}

		if (have_previous && function.function_id <= previous_id) {
			stats.ids_strictly_increasing = false;
		}
		previous_id = function.function_id;
		have_previous = true;

		stats.function_count += 1U;
		stats.data_bytes += function.data_size;
		stats.code_bytes += function.code_size;
		stats.link_count += function.link_count;
		offset = function.next_offset;
	}

	free(scratch);

	if (result == U7_OK && offset != size) {
		result = U7_ERROR_TRUNCATED;
		error.result = result;
		error.record_offset = offset;
	}

	if (out_stats != NULL) {
		*out_stats = stats;
	}
	if (out_error != NULL) {
		*out_error = error;
	}
	return result;
}
