#ifndef U7_COMMON_H
#define U7_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum U7Result {
	U7_OK = 0,
	U7_END,
	U7_ERROR_ARGUMENT,
	U7_ERROR_TRUNCATED,
	U7_ERROR_UNKNOWN_OPCODE,
	U7_ERROR_RANGE,
	U7_ERROR_CAPACITY,
	U7_ERROR_MISMATCH,
	U7_ERROR_IO
} U7Result;

const char *u7_result_name(U7Result result);

/* Little-endian reads that refuse to run past the end of the buffer. */
bool u7_read_u8(const uint8_t *bytes, size_t size, size_t offset, uint8_t *out);
bool u7_read_u16(const uint8_t *bytes, size_t size, size_t offset, uint16_t *out);

void u7_write_u16(uint8_t *bytes, uint16_t value);

/* Reads a whole file into a fresh buffer the caller frees. */
U7Result u7_read_file(const char *path, uint8_t **out_bytes, size_t *out_size);

#ifdef __cplusplus
}
#endif

#endif
