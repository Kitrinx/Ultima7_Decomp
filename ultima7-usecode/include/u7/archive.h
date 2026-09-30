#ifndef U7_ARCHIVE_H
#define U7_ARCHIVE_H

#include "u7/opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A borrowed view of one classic USECODE function record. Pointers address
 * the caller's bytes, which must outlive the view unchanged.
 *
 *   id | size | data_size | data ... | args | locals | links | link ids | code
 */
typedef struct U7Function {
	uint16_t function_id;
	uint16_t payload_size;
	size_t record_offset;
	size_t record_size;
	size_t next_offset;

	const uint8_t *data;
	uint16_t data_size;
	size_t data_offset;

	uint16_t argument_count;
	uint16_t local_count;

	const uint8_t *links;
	uint16_t link_count;

	const uint8_t *code;
	size_t code_size;
	size_t code_offset;
} U7Function;

/* U7_END means offset is exactly end of input. */
U7Result u7_parse_function(const uint8_t *bytes, size_t size, size_t offset,
		U7Function *out_function);

U7Result u7_function_link(const U7Function *function, size_t link_index,
		uint16_t *out_function_id);

/* Reads the NUL-terminated string starting at a data-segment offset. */
U7Result u7_function_string(const U7Function *function, size_t data_offset,
		const char **out_text, size_t *out_length);

typedef struct U7ArchiveStats {
	size_t function_count;
	size_t data_bytes;
	size_t code_bytes;
	size_t link_count;
	size_t instruction_count;
	size_t debug_line_count;
	size_t debug_func_count;
	bool ids_strictly_increasing;
} U7ArchiveStats;

typedef struct U7ArchiveError {
	U7Result result;
	size_t record_offset;
	uint16_t function_id;
	size_t code_offset;
	uint8_t opcode;
} U7ArchiveError;

/*
 * Walks every record to exact end of file, decodes and re-encodes every
 * instruction, and rebuilds each complete record, requiring byte identity
 * throughout. Trailing bytes are a failure, not a truncated success.
 */
U7Result u7_archive_verify(const uint8_t *bytes, size_t size,
		U7ArchiveStats *out_stats, U7ArchiveError *out_error);

#ifdef __cplusplus
}
#endif

#endif
