#include "u7/names.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The engine calls a game's name table gives, which the symbol table may override. */
static void load_intrinsic_names(U7NameTable *names, const char *path, const char *game) {
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		return;
	}
	char line[160];
	while (fgets(line, sizeof line, file) != NULL) {
		char which[8];
		unsigned ordinal = 0;
		char arity[16];
		char result[16];
		char name[48];
		if (sscanf(line, "%7s %x %15s %15s %47s", which, &ordinal, arity, result, name) == 5 &&
				name[0] != '-' && strcmp(name, "UNKNOWN") != 0 && strcmp(which, game) == 0 &&
				ordinal < 256U) {
			(void) u7_names_add(names, U7_NAME_INTRINSIC, name, (int32_t) ordinal);
		}
	}
	fclose(file);
}

/* Rows of the symbol table the listing was written with: kind, number, name. */
static void load_symbols(U7NameTable *names, const char *path) {
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		return;
	}
	char line[512];
	while (fgets(line, sizeof line, file) != NULL) {
		char kind[32];
		long number = 0;
		char name[256];
		if (line[0] == '#' || sscanf(line, "%31s %ld %255s", kind, &number, name) != 3) {
			continue;
		}
		U7NameKind which;
		if (strcmp(kind, "function") == 0) {
			which = U7_NAME_FUNCTION;
		} else if (strcmp(kind, "intrinsic") == 0) {
			which = U7_NAME_INTRINSIC;
		} else if (strcmp(kind, "flag") == 0) {
			which = U7_NAME_FLAG;
		} else if (strcmp(kind, "script_action") == 0) {
			which = U7_NAME_ACTION;
		} else if (strcmp(kind, "object") == 0 ||
				(strncmp(kind, "const_", 6U) == 0 && strcmp(kind, "const_usable") != 0)) {
			which = U7_NAME_CONSTANT;
		} else {
			continue;
		}
		(void) u7_names_add(names, which, name, (int32_t) number);
	}
	fclose(file);
}

int main(int argc, char **argv) {
	bool readable = argc > 1 && strcmp(argv[1], "--readable") == 0;
	int first = readable ? 2 : 1;
	if (argc != first + 2) {
		fprintf(stderr, "usage: u7uccomp [--readable] <source.uc> <USECODE>\n"
				"  --readable takes names from U7_SYMBOLS, U7_INTRINSIC_NAMES and U7_GAME.\n");
		return 2;
	}
	const char *source = argv[first];
	const char *target = argv[first + 1];

	uint8_t *text = NULL;
	size_t length = 0;
	U7Result result = u7_read_file(source, &text, &length);
	if (result != U7_OK) {
		fprintf(stderr, "%s: %s\n", source, u7_result_name(result));
		return 1;
	}

	U7NameTable names = {0};
	if (readable) {
		const char *game = getenv("U7_GAME");
		const char *intrinsics = getenv("U7_INTRINSIC_NAMES");
		const char *symbols = getenv("U7_SYMBOLS");
		if (intrinsics != NULL) {
			load_intrinsic_names(&names, intrinsics, game != NULL ? game : "bg");
		}
		if (symbols != NULL) {
			load_symbols(&names, symbols);
		}
	}

	U7Parsed parsed;
	result = readable
			? u7_compile_source((const char *) text, length, u7_names_lookup, &names, &parsed)
			: u7_parse_source((const char *) text, length, &parsed);
	if (result != U7_OK) {
		fprintf(stderr, "%s:%zu: %s\n", source, parsed.error_line, parsed.error);
		u7_parsed_release(&parsed);
		free(text);
		return 1;
	}

	FILE *out = fopen(target, "wb");
	uint8_t *record = malloc(0x10000U);
	if (out == NULL || record == NULL) {
		fprintf(stderr, "%s: cannot write\n", target);
		free(record);
		u7_parsed_release(&parsed);
		free(text);
		return 1;
	}

	size_t written = 0;
	size_t bytes = 0;
	size_t refused = 0;
	for (size_t i = 0; i < parsed.count; ++i) {
		U7Unit unit;
		u7_parsed_unit(&parsed.functions[i], &unit);

		U7LowerReport report;
		if (u7_assemble_unit(&unit, record, 0x10000U, &report) != U7_OK) {
			fprintf(stderr, "fn_%04X: cannot be assembled (opcode %02X)\n",
					unit.function_id, report.failing_opcode);
			refused += 1U;
			continue;
		}
		fwrite(record, 1U, report.code_size, out);
		bytes += report.code_size;
		written += 1U;
	}

	fclose(out);
	free(record);
	printf("%s -> %s\n", source, target);
	printf("  %zu functions assembled, %zu bytes, %zu refused\n", written, bytes,
			refused);

	u7_names_release(&names);
	u7_parsed_release(&parsed);
	free(text);
	return refused == 0 ? 0 : 1;
}
