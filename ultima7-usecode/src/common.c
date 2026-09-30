#include "u7/common.h"

#include <stdio.h>
#include <stdlib.h>

const char *u7_result_name(U7Result result) {
	switch (result) {
		case U7_OK: return "ok";
		case U7_END: return "end";
		case U7_ERROR_ARGUMENT: return "invalid argument";
		case U7_ERROR_TRUNCATED: return "truncated";
		case U7_ERROR_UNKNOWN_OPCODE: return "unknown opcode";
		case U7_ERROR_RANGE: return "out of range";
		case U7_ERROR_CAPACITY: return "insufficient capacity";
		case U7_ERROR_MISMATCH: return "mismatch";
		case U7_ERROR_IO: return "io error";
	}
	return "unrecognized result";
}

bool u7_read_u8(const uint8_t *bytes, size_t size, size_t offset, uint8_t *out) {
	if (bytes == NULL || out == NULL || offset >= size) {
		return false;
	}
	*out = bytes[offset];
	return true;
}

bool u7_read_u16(const uint8_t *bytes, size_t size, size_t offset, uint16_t *out) {
	if (bytes == NULL || out == NULL || offset + 2U > size || offset + 2U < offset) {
		return false;
	}
	*out = (uint16_t) ((uint16_t) bytes[offset] | ((uint16_t) bytes[offset + 1U] << 8));
	return true;
}

void u7_write_u16(uint8_t *bytes, uint16_t value) {
	bytes[0] = (uint8_t) (value & 0xFFU);
	bytes[1] = (uint8_t) (value >> 8);
}

U7Result u7_read_file(const char *path, uint8_t **out_bytes, size_t *out_size) {
	if (path == NULL || out_bytes == NULL || out_size == NULL) {
		return U7_ERROR_ARGUMENT;
	}
	*out_bytes = NULL;
	*out_size = 0;

	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		return U7_ERROR_IO;
	}
	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return U7_ERROR_IO;
	}
	long end = ftell(file);
	if (end < 0 || fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		return U7_ERROR_IO;
	}

	size_t size = (size_t) end;
	uint8_t *bytes = malloc(size > 0 ? size : 1U);
	if (bytes == NULL) {
		fclose(file);
		return U7_ERROR_CAPACITY;
	}
	if (size > 0 && fread(bytes, 1U, size, file) != size) {
		free(bytes);
		fclose(file);
		return U7_ERROR_IO;
	}
	fclose(file);

	*out_bytes = bytes;
	*out_size = size;
	return U7_OK;
}
