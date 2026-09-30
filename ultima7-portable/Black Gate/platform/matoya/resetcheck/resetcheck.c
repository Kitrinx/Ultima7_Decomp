/* Test builds only (U7_RESET_CHECK): checks that every program starts from the same globals.
 *
 * The game's writable globals are gathered in two sections (sections.h). They are copied at
 * start-up and again after the first reset, before any program runs. The first reset is compared
 * with start-up, which shows any reset writing a value its declaration doesn't give; tables that
 * get their values only from a reset show up there as expected. Each later reset is compared with
 * the first. Each differing byte range is printed as its offset from the image base, which nm on
 * the binary turns into a symbol, and the nearest symbol dladdr knows.
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../backend.h"

#ifdef __APPLE__
#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>
#else
extern uint8_t __start_u7bss[] __attribute__((weak));
extern uint8_t __stop_u7bss[] __attribute__((weak));
extern uint8_t __start_u7data[] __attribute__((weak));
extern uint8_t __stop_u7data[] __attribute__((weak));
#endif

typedef struct {
	const char *name;
	uint8_t *start;
	size_t size;
	uint8_t *copy;
	uint8_t *startup;
} section;

/* Globals allowed to differ after a reset, each with why. */
static const struct {
	const char *symbol;
	size_t size;
} allowed[] = {
	/* New heap blocks each time their owners are constructed again. */
	{"WorkString", sizeof (char *)},
	{"PaperdollPositions", 8 * sizeof (void *)},
	/* The DOS memory block, allocated by the first program and kept for the rest. */
	{"LinearBase", sizeof (void *)},
	{"LinearSize", sizeof (uint32_t)},
	{NULL, 0},
};

static section sections[2];

static void find_sections(void)
{
#ifdef __APPLE__
	unsigned long size;

	sections[0].name = "__u7bss";
	sections[0].start = getsectiondata(&_mh_execute_header, "__DATA", "__u7bss", &size);
	sections[0].size = sections[0].start != NULL ? size : 0;
	sections[1].name = "__u7data";
	sections[1].start = getsectiondata(&_mh_execute_header, "__DATA", "__u7data", &size);
	sections[1].size = sections[1].start != NULL ? size : 0;
#else
	sections[0].name = "u7bss";
	sections[0].start = __start_u7bss;
	sections[0].size = __start_u7bss != NULL ? (size_t) (__stop_u7bss - __start_u7bss) : 0;
	sections[1].name = "u7data";
	sections[1].start = __start_u7data;
	sections[1].size = __start_u7data != NULL ? (size_t) (__stop_u7data - __start_u7data) : 0;
#endif
}

static uint8_t *copy_of(const section *s)
{
	uint8_t *copy = malloc(s->size ? s->size : 1);

	memcpy(copy, s->start, s->size);
	return copy;
}

void reset_check_startup(void)
{
	find_sections();
	for (int i = 0; i < 2; i++) {
		sections[i].startup = copy_of(&sections[i]);
		fprintf(stderr, "u7: reset check: %s holds %zu bytes\n", sections[i].name, sections[i].size);
	}
}

static bool is_allowed(const uint8_t *address)
{
	for (size_t i = 0; allowed[i].symbol != NULL; i++) {
		const uint8_t *start = dlsym(RTLD_DEFAULT, allowed[i].symbol);

		if (start != NULL && address >= start && address < start + allowed[i].size)
			return true;
	}
	return false;
}

static void report(const uint8_t *start, size_t length)
{
	Dl_info info = {0};

	dladdr(start, &info);
	fprintf(stderr, "u7: reset check: image+%#lx %zu bytes", (unsigned long) (start
		- (const uint8_t *) info.dli_fbase), length);
	if (info.dli_sname != NULL)
		fprintf(stderr, ", near %s+%lu", info.dli_sname,
			(unsigned long) (start - (const uint8_t *) info.dli_saddr));
	fputc('\n', stderr);
}

/* Reports each range where the globals differ from a copy; returns the bytes that differ. */
static size_t compare(bool is_copy, const char *against)
{
	size_t ranges = 0, bytes = 0;

	for (int i = 0; i < 2; i++) {
		section *s = &sections[i];
		const uint8_t *copy = is_copy ? s->copy : s->startup;

		for (size_t at = 0; at < s->size;) {
			size_t end = at;

			while (end < s->size && s->start[end] != copy[end] && !is_allowed(s->start + end))
				end++;
			if (end == at) {
				at++;
				continue;
			}
			report(s->start + at, end - at);
			ranges++;
			bytes += end - at;
			at = end;
		}
	}
	fprintf(stderr, "u7: reset check: %zu ranges, %zu bytes differ from %s\n", ranges, bytes, against);
	return bytes;
}

void reset_check(void)
{
	if (sections[0].copy == NULL) {
		compare(false, "start-up");
		for (int i = 0; i < 2; i++)
			sections[i].copy = copy_of(&sections[i]);
		return;
	}
	compare(true, "the first program's start");
}
