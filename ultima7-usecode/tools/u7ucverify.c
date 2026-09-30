#include "u7/archive.h"

#include <stdio.h>
#include <stdlib.h>

static int verify_one(const char *path);

static int verify_one(const char *path) {
	uint8_t *bytes = NULL;
	size_t size = 0;
	U7Result result = u7_read_file(path, &bytes, &size);
	if (result != U7_OK) {
		fprintf(stderr, "%s: %s\n", path, u7_result_name(result));
		return 1;
	}

	U7ArchiveStats stats;
	U7ArchiveError error;
	result = u7_archive_verify(bytes, size, &stats, &error);
	free(bytes);

	if (result != U7_OK) {
		fprintf(stderr,
				"%s: %s at record 0x%zX (function 0x%04X), code +0x%zX, opcode 0x%02X\n",
				path, u7_result_name(error.result), error.record_offset,
				error.function_id, error.code_offset, error.opcode);
		return 1;
	}

	printf("%s\n", path);
	printf("  %zu bytes, %zu functions, ids strictly increasing: %s\n", size,
			stats.function_count, stats.ids_strictly_increasing ? "yes" : "no");
	printf("  %zu data bytes, %zu code bytes, %zu links\n", stats.data_bytes,
			stats.code_bytes, stats.link_count);
	printf("  %zu instructions re-encoded byte-exactly\n", stats.instruction_count);
	printf("  debug records: %zu dbgline, %zu dbgfunc\n", stats.debug_line_count,
			stats.debug_func_count);
	return 0;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "usage: u7ucverify <USECODE> ...\n");
		return 2;
	}
	int status = 0;
	for (int i = 1; i < argc; ++i) {
		status |= verify_one(argv[i]);
	}
	return status;
}
