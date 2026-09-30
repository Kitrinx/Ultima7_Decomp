#include "u7/archive.h"
#include "u7/linkdep.h"
#include "u7/names.h"
#include "u7/sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Builds a game's USECODE, LINKDEP1 and LINKDEP2 from a project, as Origin's
 * tools did from many source files:
 *
 *     u7project 1;
 *     game "bg";
 *     header "usecode.uh";          names, in Origin's index format
 *     source_root "src";
 *     link "usecode.lnk";           the source files in build order, one to a line
 *     map "usecode.map";            each routine's number, kept from build to build
 *     expect_sha256 "...";          the shipped USECODE's
 *
 * Usables take their numbers from the header's usableindex. A routine keeps the
 * number the map gives it, or one it claims with #id or as fn_XXXX; a new
 * routine takes the next number after all of those, in build order, as
 * Origin's late additions did. With no map, routines are numbered from 0x800 in
 * build order. The map is rewritten when routines come or go.
 */

enum { MAX_SOURCES = 4096 };

typedef struct Project {
	char game[16];
	char header[512];
	char source_root[512];
	char link[512];
	char map[512];
	char *sources[MAX_SOURCES];
	size_t source_count;
	char expect[65];
} Project;

typedef struct Record {
	uint16_t id;
	uint8_t *bytes;
	size_t size;
} Record;

/* A function a source declares, and what the build learns of it. */
typedef struct Declaration {
	char name[256];
	long number;              /* -1 until known. */
	bool claimed;             /* Written with #id or as fn_XXXX. */
	bool usable;              /* Numbered by the header. */
	int arguments;
	bool returns;
	size_t source;
	size_t line;
} Declaration;

/* The value of a manifest line: a quoted string or a bare word. */
static bool manifest_value(const char *line, const char *key, char *out, size_t capacity) {
	size_t length = strlen(key);
	if (strncmp(line, key, length) != 0 || (line[length] != ' ' && line[length] != '\t')) {
		return false;
	}
	const char *at = line + length;
	while (*at == ' ' || *at == '\t') {
		at += 1;
	}
	bool quoted = *at == '"';
	at += quoted ? 1 : 0;
	size_t n = 0;
	while (at[n] != 0 && at[n] != (quoted ? '"' : ';') && n + 1U < capacity) {
		n += 1U;
	}
	memcpy(out, at, n);
	out[n] = 0;
	return true;
}

static bool add_source(Project *project, const char *name) {
	if (project->source_count == MAX_SOURCES) {
		return false;
	}
	project->sources[project->source_count] = malloc(strlen(name) + 1U);
	if (project->sources[project->source_count] == NULL) {
		return false;
	}
	strcpy(project->sources[project->source_count++], name);
	return true;
}

static bool read_manifest(const char *path, Project *out) {
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		fprintf(stderr, "%s: cannot read\n", path);
		return false;
	}
	memset(out, 0, sizeof *out);
	char line[1024];
	char value[512];
	bool ok = true;
	while (ok && fgets(line, sizeof line, file) != NULL) {
		char *text = line;
		while (*text == ' ' || *text == '\t') {
			text += 1;
		}
		if (manifest_value(text, "game", value, sizeof value)) {
			snprintf(out->game, sizeof out->game, "%s", value);
		} else if (manifest_value(text, "header", value, sizeof value)) {
			snprintf(out->header, sizeof out->header, "%s", value);
		} else if (manifest_value(text, "source_root", value, sizeof value)) {
			snprintf(out->source_root, sizeof out->source_root, "%s", value);
		} else if (manifest_value(text, "link", value, sizeof value)) {
			snprintf(out->link, sizeof out->link, "%s", value);
		} else if (manifest_value(text, "map", value, sizeof value)) {
			snprintf(out->map, sizeof out->map, "%s", value);
		} else if (manifest_value(text, "source", value, sizeof value)) {
			ok = add_source(out, value);
		} else if (manifest_value(text, "expect_sha256", value, sizeof value)) {
			snprintf(out->expect, sizeof out->expect, "%s", value);
		}
	}
	fclose(file);
	if (!ok) {
		fprintf(stderr, "%s: too many sources\n", path);
	}
	return ok;
}

/* The build order: one source file to a line. */
static bool read_link_order(const char *path, Project *project) {
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		fprintf(stderr, "%s: cannot read\n", path);
		return false;
	}
	char line[1024];
	bool ok = true;
	while (ok && fgets(line, sizeof line, file) != NULL) {
		char *text = line;
		while (*text == ' ' || *text == '\t') {
			text += 1;
		}
		size_t length = strcspn(text, "\r\n");
		while (length > 0 && (text[length - 1U] == ' ' || text[length - 1U] == '\t')) {
			length -= 1U;
		}
		text[length] = 0;
		if (length > 0) {
			ok = add_source(project, text);
		}
	}
	fclose(file);
	if (!ok) {
		fprintf(stderr, "%s: too many sources\n", path);
	}
	return ok;
}

/* A path beside the manifest: its directory, then the parts given. */
static void beside(char *out, size_t capacity, const char *manifest, const char *a,
		const char *b) {
	const char *slash = strrchr(manifest, '/');
	int directory = slash != NULL ? (int) (slash - manifest + 1) : 0;
	if (b != NULL && b[0] != 0) {
		snprintf(out, capacity, "%.*s%s/%s", directory, manifest, a, b);
	} else {
		snprintf(out, capacity, "%.*s%s", directory, manifest, a);
	}
}

static bool is_name_char(char c) {
	return c == '_' || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z');
}

/*
 * Reads a declaration from a line starting Usable, Routine or Function: its
 * name, a number it claims, how many arguments it accepts and whether it
 * returns a value, which a Function or a ~ before the name says.
 */
static bool declaration_at(const U7NameTable *names, const char *at, const char *line_end,
		Declaration *out) {
	const char *keywords[] = {"Usable ", "Routine ", "Function "};
	for (size_t k = 0; k < 3U; ++k) {
		size_t n = strlen(keywords[k]);
		if ((size_t) (line_end - at) <= n || strncmp(at, keywords[k], n) != 0) {
			continue;
		}
		memset(out, 0, sizeof *out);
		const char *name = at + n;
		out->returns = k == 2U || *name == '~';
		name += *name == '~' ? 1 : 0;
		size_t length = 0;
		while (name + length < line_end && is_name_char(name[length])) {
			length += 1U;
		}
		snprintf(out->name, sizeof out->name, "%.*s", (int) length, name);
		out->number = -1;
		int32_t value = 0;
		out->usable = u7_names_lookup(names, U7_NAME_FUNCTION, name, length, &value);
		const char *after = name + length;
		if (length == 7U && strncmp(name, "fn_", 3U) == 0) {
			out->number = strtol(name + 3, NULL, 16);
			out->claimed = true;
		} else if ((size_t) (line_end - after) > 6U && strncmp(after, " #id(0x", 7U) == 0) {
			out->number = strtol(after + 7, NULL, 16);
			out->claimed = true;
		} else if (out->usable) {
			out->number = value;
		}
		/* accepts a0, a1, ... counts the arguments. */
		for (const char *p = after; p + 8 < line_end; ++p) {
			if (strncmp(p, " accepts ", 9U) != 0) {
				continue;
			}
			for (p += 9; p < line_end && *p != '#' && *p != '{'; ++p) {
				out->arguments += *p == 'a' && (p[-1] == ' ' || p[-1] == ',') ? 1 : 0;
			}
			break;
		}
		return true;
	}
	return false;
}

static bool read_declarations(const U7NameTable *names, uint8_t **texts, const size_t *lengths,
		size_t count, Declaration **out, size_t *out_count) {
	size_t capacity = 1024U;
	Declaration *list = malloc(capacity * sizeof *list);
	size_t n = 0;
	if (list == NULL) {
		return false;
	}
	for (size_t s = 0; s < count; ++s) {
		const char *at = (const char *) texts[s];
		const char *end = at + lengths[s];
		size_t line = 1U;
		while (at < end) {
			const char *stop = memchr(at, '\n', (size_t) (end - at));
			const char *line_end = stop != NULL ? stop : end;
			Declaration declaration;
			if (declaration_at(names, at, line_end, &declaration)) {
				if (n == capacity) {
					capacity *= 2U;
					Declaration *grown = realloc(list, capacity * sizeof *grown);
					if (grown == NULL) {
						free(list);
						return false;
					}
					list = grown;
				}
				declaration.source = s;
				declaration.line = line;
				list[n++] = declaration;
			}
			at = stop != NULL ? stop + 1 : end;
			line += 1U;
		}
	}
	*out = list;
	*out_count = n;
	return true;
}

/* The routine numbers a previous build kept: lines of a hex number and a name. */
static bool read_map(const char *path, U7NameTable *out) {
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		return false;
	}
	char line[512];
	while (fgets(line, sizeof line, file) != NULL) {
		unsigned number = 0;
		char name[256];
		if (sscanf(line, "%x %255s", &number, name) == 2) {
			(void) u7_names_add(out, U7_NAME_FUNCTION, name, (int32_t) number);
		}
	}
	fclose(file);
	return true;
}

static int by_number(const void *a, const void *b) {
	long x = (*(const Declaration *const *) a)->number;
	long y = (*(const Declaration *const *) b)->number;
	return (x > y) - (x < y);
}

/*
 * Gives every routine its number and says so in the names table. A routine
 * keeps a number it claims, else the one the map holds for its name; a new
 * one follows everything already numbered, in build order, or with no map
 * takes the numbers left from 0x800 up.
 */
static bool number_routines(Declaration *list, size_t count, const U7NameTable *map,
		bool have_map, U7NameTable *names, char **sources, size_t *out_added) {
	static long owner[65536];
	for (size_t i = 0; i < 65536U; ++i) {
		owner[i] = -1;
	}
	uint32_t highest = 0x7FFU;
	for (int pass = 0; pass < 2; ++pass) {
		for (size_t d = 0; d < count; ++d) {
			Declaration *declaration = &list[d];
			if (declaration->usable || (pass == 0) != declaration->claimed) {
				continue;
			}
			int32_t kept = 0;
			if (pass == 1 && u7_names_lookup(map, U7_NAME_FUNCTION, declaration->name,
					strlen(declaration->name), &kept)) {
				declaration->number = kept;
			}
			if (declaration->number < 0) {
				continue;
			}
			if (declaration->number >= U7_LINKDEP_FUNCTIONS) {
				fprintf(stderr, "%s:%zu: %s is numbered 0x%04lX, past the 0x%X the engine indexes\n",
						sources[declaration->source], declaration->line, declaration->name,
						declaration->number, U7_LINKDEP_FUNCTIONS);
				return false;
			}
			long other = owner[declaration->number];
			if (other >= 0) {
				fprintf(stderr, "%s:%zu: %s and %s (%s:%zu) both take 0x%04lX\n",
						sources[declaration->source], declaration->line, declaration->name,
						list[other].name, sources[list[other].source], list[other].line,
						declaration->number);
				return false;
			}
			owner[declaration->number] = (long) d;
			if (declaration->number >= 0x800 && (uint32_t) declaration->number > highest) {
				highest = (uint32_t) declaration->number;
			}
		}
	}
	uint32_t next = have_map ? highest + 1U : 0x800U;
	size_t added = 0;
	for (size_t d = 0; d < count; ++d) {
		Declaration *declaration = &list[d];
		if (declaration->usable || declaration->number >= 0) {
			continue;
		}
		while (next < U7_LINKDEP_FUNCTIONS && owner[next] >= 0) {
			next += 1U;
		}
		if (next >= U7_LINKDEP_FUNCTIONS) {
			fprintf(stderr, "%s:%zu: no number is left for %s\n", sources[declaration->source],
					declaration->line, declaration->name);
			return false;
		}
		declaration->number = next;
		owner[next] = (long) d;
		added += 1U;
	}
	for (size_t d = 0; d < count; ++d) {
		const Declaration *declaration = &list[d];
		bool ok = (declaration->usable || strncmp(declaration->name, "fn_", 3U) == 0 ||
				u7_names_add(names, U7_NAME_FUNCTION, declaration->name,
						(int32_t) declaration->number)) &&
				u7_names_add(names, U7_NAME_ARGUMENTS, declaration->name, declaration->arguments) &&
				u7_names_add(names, U7_NAME_RETURNS, declaration->name, declaration->returns);
		if (!ok) {
			return false;
		}
	}
	*out_added = have_map ? added : 0;
	return true;
}

/* Rewrites the map: every named routine, by number. */
static bool write_map(const char *path, Declaration *list, size_t count) {
	Declaration **sorted = malloc((count + 1U) * sizeof *sorted);
	if (sorted == NULL) {
		return false;
	}
	size_t n = 0;
	for (size_t d = 0; d < count; ++d) {
		if (!list[d].usable && list[d].number >= 0x800 && strncmp(list[d].name, "fn_", 3U) != 0) {
			sorted[n++] = &list[d];
		}
	}
	qsort(sorted, n, sizeof *sorted, by_number);
	FILE *file = fopen(path, "w");
	if (file == NULL) {
		free(sorted);
		return false;
	}
	for (size_t i = 0; i < n; ++i) {
		fprintf(file, "%04lX %s\n", sorted[i]->number, sorted[i]->name);
	}
	fclose(file);
	free(sorted);
	return true;
}

static int compare_records(const void *a, const void *b) {
	const Record *x = a;
	const Record *y = b;
	return (int) x->id - (int) y->id;
}

/*
 * Whether the engine can load every function, with all it can call, and read
 * the dependency file that lists them.
 */
static bool loadable(const U7Corpus *corpus, const Declaration *list, size_t count,
		char **sources) {
	static U7Load loads[U7_LINKDEP_FUNCTIONS];
	if (u7_linkdep_loads(corpus, loads) != U7_OK) {
		fprintf(stderr, "cannot follow the link tables\n");
		return false;
	}
	bool ok = true;
	size_t entries = 0;
	for (size_t f = 0; f < corpus->count; ++f) {
		uint16_t id = corpus->functions[f].function_id;
		const U7Load *load = &loads[id];
		entries += load->functions;
		if (load->functions <= U7_LOAD_FUNCTIONS && load->bytes <= U7_LOAD_BYTES) {
			continue;
		}
		size_t d = 0;
		while (d < count && list[d].number != id) {
			d += 1U;
		}
		if (d < count) {
			fprintf(stderr, "%s:%zu: ", sources[list[d].source], list[d].line);
		}
		fprintf(stderr, "to run %s the engine loads %zu functions, %zu bytes; it holds at most "
				"%d functions and %d bytes\n", d < count ? list[d].name : "a function",
				load->functions, load->bytes, U7_LOAD_FUNCTIONS, U7_LOAD_BYTES);
		ok = false;
	}
	if (entries > U7_LINKDEP_ENTRIES) {
		fprintf(stderr, "LINKDEP2 would hold %zu entries; the engine reads at most %d\n", entries,
				U7_LINKDEP_ENTRIES);
		ok = false;
	}
	return ok;
}

static bool write_file(const char *path, const uint8_t *bytes, size_t size) {
	FILE *file = fopen(path, "wb");
	bool ok = file != NULL && fwrite(bytes, 1U, size, file) == size;
	if (file != NULL) {
		ok = fclose(file) == 0 && ok;
	}
	if (!ok) {
		fprintf(stderr, "%s: cannot write\n", path);
	}
	return ok;
}

int main(int argc, char **argv) {
	bool check = argc > 1 && strcmp(argv[1], "--check") == 0;
	int first = check ? 2 : 1;
	if (argc != first + 2) {
		fprintf(stderr, "usage: u7ucproj [--check] <project.u7p> <output directory>\n"
				"  writes USECODE, LINKDEP1 and LINKDEP2; --check fails unless USECODE\n"
				"  matches the project's expected sha256.\n");
		return 2;
	}
	const char *manifest = argv[first];
	const char *output_directory = argv[first + 1];
	Project project;
	if (!read_manifest(manifest, &project)) {
		return 1;
	}

	int status = 1;
	U7NameTable names = {0};
	U7NameTable map = {0};
	uint8_t *texts[MAX_SOURCES] = {0};
	size_t lengths[MAX_SOURCES] = {0};
	Declaration *declarations = NULL;
	size_t declaration_count = 0;
	Record *records = NULL;
	size_t record_count = 0;
	uint8_t *output = NULL;
	uint8_t *record = NULL;
	U7Corpus corpus = {0};
	char path[1024];

	uint8_t *header = NULL;
	size_t header_length = 0;
	size_t error_line = 0;
	beside(path, sizeof path, manifest, project.header, NULL);
	if (u7_read_file(path, &header, &header_length) != U7_OK ||
			u7_names_read_header(&names, (const char *) header, header_length,
					&error_line) != U7_OK) {
		fprintf(stderr, "%s:%zu: cannot read the header\n", path, error_line);
		free(header);
		goto done;
	}
	free(header);

	if (project.link[0] != 0) {
		beside(path, sizeof path, manifest, project.link, NULL);
		if (!read_link_order(path, &project)) {
			goto done;
		}
	}
	/* Every source is read first, so a routine may be named before it is defined. */
	for (size_t s = 0; s < project.source_count; ++s) {
		beside(path, sizeof path, manifest, project.source_root, project.sources[s]);
		if (u7_read_file(path, &texts[s], &lengths[s]) != U7_OK) {
			fprintf(stderr, "%s: cannot read\n", path);
			goto done;
		}
	}
	if (!read_declarations(&names, texts, lengths, project.source_count, &declarations,
			&declaration_count)) {
		goto done;
	}
	for (size_t a = 0; a < declaration_count; ++a) {
		for (size_t b = a + 1U; b < declaration_count; ++b) {
			if (strcmp(declarations[a].name, declarations[b].name) == 0) {
				fprintf(stderr, "%s:%zu: %s is also declared at %s:%zu\n",
						project.sources[declarations[b].source], declarations[b].line,
						declarations[b].name, project.sources[declarations[a].source],
						declarations[a].line);
				goto done;
			}
		}
	}
	bool have_map = false;
	if (project.map[0] != 0) {
		beside(path, sizeof path, manifest, project.map, NULL);
		have_map = read_map(path, &map);
	}
	size_t added = 0;
	if (!number_routines(declarations, declaration_count, &map, have_map, &names,
			project.sources, &added)) {
		goto done;
	}

	record = malloc(0x10000U);
	if (record == NULL) {
		goto done;
	}
	for (size_t s = 0; s < project.source_count; ++s) {
		U7Parsed parsed;
		if (u7_compile_source((const char *) texts[s], lengths[s], u7_names_lookup, &names,
				&parsed) != U7_OK) {
			fprintf(stderr, "%s:%zu: %s\n", project.sources[s], parsed.error_line, parsed.error);
			u7_parsed_release(&parsed);
			goto done;
		}
		Record *grown = realloc(records, (record_count + parsed.count + 1U) * sizeof *grown);
		if (grown == NULL) {
			u7_parsed_release(&parsed);
			goto done;
		}
		records = grown;
		for (size_t f = 0; f < parsed.count; ++f) {
			U7Unit unit;
			U7LowerReport report;
			u7_parsed_unit(&parsed.functions[f], &unit);
			if (parsed.functions[f].record != NULL) {
				/* Carried as its bytes. */
				memcpy(record, parsed.functions[f].record, parsed.functions[f].record_size);
				report.code_size = parsed.functions[f].record_size;
			} else if (u7_assemble_unit(&unit, record, 0x10000U, &report) != U7_OK) {
				fprintf(stderr, "%s: fn_%04X cannot be assembled (opcode %02X)\n",
						project.sources[s], unit.function_id, report.failing_opcode);
				u7_parsed_release(&parsed);
				goto done;
			}
			Record *out = &records[record_count++];
			out->id = unit.function_id;
			out->size = report.code_size;
			out->bytes = malloc(report.code_size);
			if (out->bytes == NULL) {
				u7_parsed_release(&parsed);
				goto done;
			}
			memcpy(out->bytes, record, report.code_size);
		}
		u7_parsed_release(&parsed);
	}

	qsort(records, record_count, sizeof *records, compare_records);
	size_t total = 0;
	for (size_t r = 0; r < record_count; ++r) {
		if (r > 0 && records[r].id == records[r - 1U].id) {
			fprintf(stderr, "fn_%04X is defined twice\n", records[r].id);
			goto done;
		}
		total += records[r].size;
	}
	output = malloc(total + 1U);
	if (output == NULL) {
		goto done;
	}
	size_t at = 0;
	for (size_t r = 0; r < record_count; ++r) {
		memcpy(output + at, records[r].bytes, records[r].size);
		at += records[r].size;
	}

	/* Nothing is written that the engine could not read. */
	U7ArchiveStats stats;
	U7ArchiveError problem;
	if (u7_archive_verify(output, total, &stats, &problem) != U7_OK) {
		fprintf(stderr, "the built USECODE does not verify: %s in fn_%04X at code +0x%zX\n",
				u7_result_name(problem.result), problem.function_id, problem.code_offset);
		goto done;
	}
	uint8_t *copy = malloc(total + 1U);
	if (copy == NULL) {
		goto done;
	}
	memcpy(copy, output, total);
	if (u7_corpus_parse(copy, total, &corpus) != U7_OK ||
			!loadable(&corpus, declarations, declaration_count, project.sources)) {
		goto done;
	}
	snprintf(path, sizeof path, "%s/USECODE", output_directory);
	if (!write_file(path, output, total)) {
		goto done;
	}
	snprintf(path, sizeof path, "%s/LINKDEP1", output_directory);
	FILE *one = fopen(path, "wb");
	snprintf(path, sizeof path, "%s/LINKDEP2", output_directory);
	FILE *two = fopen(path, "wb");
	U7Result linked = one != NULL && two != NULL
			? u7_write_linkdep(&corpus, one, two, NULL) : U7_ERROR_IO;
	if (one != NULL) {
		fclose(one);
	}
	if (two != NULL) {
		fclose(two);
	}
	if (linked != U7_OK) {
		fprintf(stderr, "cannot write the dependency files: %s\n", u7_result_name(linked));
		goto done;
	}

	if (project.map[0] != 0) {
		beside(path, sizeof path, manifest, project.map, NULL);
		if (!write_map(path, declarations, declaration_count)) {
			fprintf(stderr, "%s: cannot write\n", path);
			goto done;
		}
	}

	char digest[65];
	u7_sha256_hex(output, total, digest);
	printf("%s -> %s: USECODE, LINKDEP1, LINKDEP2\n", manifest, output_directory);
	printf("  %zu sources, %zu functions, %zu bytes, sha256 %s\n", project.source_count,
			record_count, total, digest);
	if (added > 0) {
		printf("  %zu new routines numbered after the existing ones\n", added);
	}
	status = 0;
	if (project.expect[0] != 0) {
		bool same = strcmp(digest, project.expect) == 0;
		printf("  %s\n", same ? "identical to the shipped USECODE"
				: "differs from the shipped USECODE");
		status = check && !same ? 1 : 0;
	}

done:
	for (size_t s = 0; s < project.source_count; ++s) {
		free(project.sources[s]);
		free(texts[s]);
	}
	for (size_t r = 0; r < record_count; ++r) {
		free(records[r].bytes);
	}
	free(records);
	free(record);
	free(output);
	u7_corpus_release(&corpus);
	free(declarations);
	u7_names_release(&map);
	u7_names_release(&names);
	return status;
}
