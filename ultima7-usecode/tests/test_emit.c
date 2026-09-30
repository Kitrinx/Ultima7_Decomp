#include "u7/emit.h"

#include <stdlib.h>
#include <string.h>

static int failures;

static void check(bool condition, const char *what) {
	if (!condition) {
		printf("FAIL %s\n", what);
		failures += 1;
	}
}

static size_t build_record(uint8_t *out, const char *data, size_t data_size,
		uint16_t args, uint16_t locals, const uint8_t *code, size_t code_size) {
	size_t payload = 2U + data_size + 6U + code_size;
	u7_write_u16(out, 0x00B2U);
	u7_write_u16(out + 2U, (uint16_t) payload);
	u7_write_u16(out + 4U, (uint16_t) data_size);
	memcpy(out + 6U, data, data_size);
	size_t at = 6U + data_size;
	u7_write_u16(out + at, args);
	u7_write_u16(out + at + 2U, locals);
	u7_write_u16(out + at + 4U, 0U);
	memcpy(out + at + 6U, code, code_size);
	return 4U + payload;
}

/* Every call in these fixtures takes one argument and leaves nothing. */
static bool resolve_one_argument(void *context, uint16_t function_id, U7CallTarget *out) {
	(void) context;
	(void) function_id;
	out->argument_count = 1U;
	out->returns_value = false;
	return true;
}

static void emit_styled(const uint8_t *code, size_t code_size, const char *data,
		size_t data_size, uint16_t args, uint16_t locals, const U7Names *names,
		U7EmitStyle style, char *out, size_t capacity) {
	static uint8_t record[512];
	size_t size = build_record(record, data, data_size, args, locals, code, code_size);

	U7Function function;
	U7Cfg cfg;
	U7Lifted lifted;
	U7Structured structured;
	check(u7_parse_function(record, size, 0, &function) == U7_OK, "record parses");
	check(u7_cfg_build(&function, &cfg) == U7_OK, "graph builds");
	check(u7_lift_function(&function, &cfg, resolve_one_argument, NULL, &lifted) == U7_OK,
			"lift runs");
	check(u7_structure_function(&function, &cfg, &lifted, &structured) == U7_OK,
			"structure runs");

	FILE *stream = tmpfile();
	check(stream != NULL, "temporary stream");
	if (stream != NULL) {
		U7Names with_function = names != NULL ? *names : (U7Names) {0};
		with_function.function = &function;
		with_function.cfg = &cfg;
		u7_emit_function(stream, &function, &lifted, &structured, &with_function, style);
		rewind(stream);
		size_t read = fread(out, 1U, capacity - 1U, stream);
		out[read] = 0;
		fclose(stream);
	}

	u7_structured_release(&structured);
	u7_lifted_release(&lifted);
	u7_cfg_release(&cfg);
}

static void emit_to_buffer(const uint8_t *code, size_t code_size, const char *data,
		size_t data_size, uint16_t args, uint16_t locals, char *out, size_t capacity) {
	emit_styled(code, code_size, data, data_size, args, locals, NULL, U7_EMIT_CANONICAL,
			out, capacity);
}

/* A conditional assignment emits the language's own spelling. */
static void test_conditional_emits_agil(void) {
	static const uint8_t code[] = {
		0x48,                   /* push_event        */
		0x1F, 0x01, 0x00,       /* push_word 1       */
		0x22,                   /* equal             */
		0x05, 0x06, 0x00,       /* jump_if_false ->E */
		0x1F, 0x07, 0x00,       /* push_word 7       */
		0x12, 0x00, 0x00,       /* pop_local v0      */
		0x25                    /* return            */
	};
	char text[2048];
	emit_to_buffer(code, sizeof code, "x", 2U, 0U, 1U, text, sizeof text);

	check(strstr(text, "Usable fn_00B2") != NULL, "declared as a usable");
	check(strstr(text, "#locals(1)") != NULL, "local count pinned");
	check(strstr(text, "if (event = 1)") != NULL, "equality spelled with one sign");
	check(strstr(text, "v0 = 7;") != NULL, "assignment emitted");
	check(strstr(text, "return") == NULL, "the closing return left to the compiler");
}

/* Appends closed by say become the bracketed print construct. */
static void test_print_construct(void) {
	static const uint8_t code[] = {
		0x1C, 0x00, 0x00,       /* append_data "Hi "  */
		0x2F, 0x00, 0x00,       /* append_local v0    */
		0x33,                   /* say                */
		0x25
	};
	char text[2048];
	emit_to_buffer(code, sizeof code, "Hi ", 4U, 0U, 1U, text, sizeof text);
	check(strstr(text, "[Hi <v0>];") != NULL, "print construct emitted");
	check(strstr(text, "append") == NULL, "no append survives the fold");
}

/* A list literal keeps the language's own brackets and one-based subscript. */
static void test_list_forms(void) {
	static const uint8_t code[] = {
		0x1F, 0x01, 0x00,       /* push_word 1    */
		0x1F, 0x02, 0x00,       /* push_word 2    */
		0x1E, 0x02, 0x00,       /* make_array 2   */
		0x12, 0x00, 0x00,       /* pop_local v0   */
		0x1F, 0x01, 0x00,       /* push_word 1    */
		0x26, 0x00, 0x00,       /* array_get v0   */
		0x12, 0x01, 0x00,       /* pop_local v1   */
		0x25
	};
	char text[2048];
	emit_to_buffer(code, sizeof code, "x", 2U, 0U, 2U, text, sizeof text);
	check(strstr(text, "v0 = << 2, 1 >>;") != NULL, "list literal, last push first");
	check(strstr(text, "v1 = v0::1;") != NULL, "one-based subscript");
}

/* A print that ends its block must appear once, not once folded and once raw. */
static void test_print_ending_a_block(void) {
	static const uint8_t code[] = {
		0x44, 0x01,             /* push_byte 1          */
		0x05, 0x04, 0x00,       /* jump_if_false ->0x09 */
		0x1C, 0x00, 0x00,       /* append_data "Hi"     */
		0x33,                   /* say, last in block   */
		0x25                    /* return               */
	};
	char text[2048];
	emit_to_buffer(code, sizeof code, "Hi", 3U, 0U, 0U, text, sizeof text);
	const char *first = strstr(text, "Hi");
	check(first != NULL && strstr(first + 2, "Hi") == NULL, "the print appears once");
	check(strstr(text, "append") == NULL, "no raw append follows the print");
}

/* A loop that works before its test keeps that work ahead of the test. */
static void test_loop_working_before_its_test(void) {
	static const uint8_t code[] = {
		0x44, 0x05,             /* 00 push_byte 5          */
		0x12, 0x00, 0x00,       /* 02 pop_local v0         */
		0x21, 0x00, 0x00,       /* 05 push_local v0        */
		0x05, 0x08, 0x00,       /* 08 jump_if_false ->19   */
		0x44, 0x01,             /* 11 push_byte 1          */
		0x12, 0x01, 0x00,       /* 13 pop_local v1         */
		0x06, 0xED, 0xFF,       /* 16 jump ->00            */
		0x25                    /* 19 return               */
	};
	char text[2048];
	emit_to_buffer(code, sizeof code, "", 0U, 0U, 2U, text, sizeof text);
	check(strstr(text, "loop {") != NULL, "written as loop");
	check(strstr(text, "v0 = byte(5);") != NULL, "the work before the test survives");
	check(strstr(text, "} while v0 {") != NULL, "the test between work and body");
	check(strstr(text, "v1 = byte(1);") != NULL, "the body survives");
}

/* Names reach only the listing people read; the compiler's form keeps numbers. */
static void test_names_in_annotated_listing(void) {
	static const uint8_t code[] = {
		0x42, 0x0A, 0x00,       /* push_flag 10          */
		0x05, 0x2B, 0x00,       /* jump_if_false ->R     */
		0x1F, 0x07, 0x00,       /* push_word 7           */
		0x47, 0x01, 0x04,       /* call fn_0401 directly */
		0x1F, 0xFF, 0xFF,       /* push_word 65535       */
		0x47, 0x01, 0x04,       /* call fn_0401 directly */
		0x13,                   /* push_true             */
		0x43, 0x0A, 0x00,       /* pop_flag 10           */
		0x1F, 0x00, 0x00,       /* push_word 0           */
		0x1F, 0xFF, 0xFF,       /* push_word 65535       */
		0x39, 0x03, 0x00, 0x02, /* show_npc_face         */
		0x1F, 0xFF, 0xFF,       /* push_word 65535       */
		0x1F, 0x02, 0x00,       /* push_word 2           */
		0x39, 0x10, 0x00, 0x02, /* die_roll, discarded   */
		0x38, 0x3A, 0x00, 0x00, /* an intrinsic's result */
		0x12, 0x00, 0x00,       /* pop_local v0          */
		0x25                    /* R: return             */
	};
	static const char *functions[65536];
	static const char *flags[65536];
	functions[0x00B2] = "Plank";
	functions[0x0401] = "Iolo";
	flags[10] = "MetIolo";
	static const char *objects[65536];
	static uint8_t object_arguments[256];
	objects[65535] = "_Iolo";
	object_arguments[0x03] = 1U;   /* The face shown is the last argument pushed. */
	static uint16_t object_parameters[65536];
	object_parameters[0x0401] = 1U;
	U7Names names = {0};
	names.functions = functions;
	names.flags = flags;
	names.objects = objects;
	names.object_arguments = object_arguments;
	names.object_parameters = object_parameters;

	char text[2048];
	emit_styled(code, sizeof code, "x", 2U, 0U, 1U, &names, U7_EMIT_ANNOTATED, text,
			sizeof text);
	check(strstr(text, "Usable Plank #id(0x00B2)") != NULL, "declaration named, number kept");
	check(strstr(text, "Iolo(7);") != NULL, "call named");
	check(strstr(text, "$MetIolo = ") != NULL, "flag assignment named with its sigil");
	check(strstr(text, "flag(") == NULL && strstr(text, "fn_") == NULL, "no number left");
	check(strstr(text, "(0, _Iolo)") != NULL, "an NPC where an intrinsic takes an object");
	check(strstr(text, "Iolo(_Iolo);") != NULL, "an NPC where a routine takes an object");
	check(strstr(text, "v0 = ~intrinsic_3A();") != NULL, "a value-returning call written ~");
	check(strstr(text, "~Iolo") == NULL, "a routine written without ~");
	check(strstr(text, "(65535, 2)") != NULL, "the same constant elsewhere stays a number");

	emit_styled(code, sizeof code, "x", 2U, 0U, 1U, &names, U7_EMIT_LINEAR, text,
			sizeof text);
	check(strstr(text, "fn_0401!(7)") != NULL, "linear form calls by number");
	check(strstr(text, "flag(10)") != NULL && strstr(text, "$") == NULL,
			"linear form keeps flag numbers");
	check(strstr(text, "~") == NULL, "linear form has no sigils");
}

/*
 * Tests of v0 against 1, 2, 3...: chained through else arms ending in a jump
 * to the shared end, or as separate ifs.
 */
static size_t build_tests(uint8_t *code, int arms, bool chained, bool otherwise) {
	size_t at = 0;
	size_t jumps[16];
	for (int i = 0; i < arms; ++i) {
		const uint8_t test[] = {
			0x21, 0x00, 0x00,                        /* push_local v0      */
			0x1F, (uint8_t) (i + 1), 0x00,           /* push_word i+1      */
			0x22,                                    /* equal              */
			0x05, (uint8_t) (chained ? 8 : 5), 0x00  /* jump_if_false next */
		};
		memcpy(code + at, test, sizeof test);
		at += sizeof test;
		code[at++] = 0x44;                           /* push_byte          */
		code[at++] = (uint8_t) (10 + i);
		code[at++] = 0x12;                           /* pop_local v1       */
		code[at++] = 0x01;
		code[at++] = 0x00;
		if (chained) {
			jumps[i] = at;
			code[at++] = 0x06;                       /* jump to the end    */
			at += 2U;
		}
	}
	if (otherwise) {
		const uint8_t fallback[] = {0x44, 99, 0x12, 0x01, 0x00};
		memcpy(code + at, fallback, sizeof fallback);
		at += sizeof fallback;
	}
	for (int i = 0; chained && i < arms; ++i) {
		u7_write_u16(code + jumps[i] + 1U, (uint16_t) (at - (jumps[i] + 3U)));
	}
	code[at++] = 0x25;                               /* return             */
	return at;
}

/* Four or more else-chained tests of one value against constants read as a switch. */
static void test_switch_views(void) {
	uint8_t code[512];
	char text[4096];

	size_t size = build_tests(code, 4, true, true);
	emit_to_buffer(code, size, "x", 2U, 0U, 2U, text, sizeof text);
	check(strstr(text, "switch v0 {") != NULL, "an else ladder of four becomes a switch");
	check(strstr(text, "case 4 {") != NULL && strstr(text, "default {") != NULL,
			"its cases and its default");

	size = build_tests(code, 3, true, true);
	emit_to_buffer(code, size, "x", 2U, 0U, 2U, text, sizeof text);
	check(strstr(text, "switch") == NULL, "three tests stay an if ladder");

	/* Separate ifs compile without the jumps a switch implies, so stay ifs. */
	size = build_tests(code, 4, false, false);
	emit_to_buffer(code, size, "x", 2U, 0U, 2U, text, sizeof text);
	check(strstr(text, "switch") == NULL, "four separate tests stay separate ifs");
}

static const char *test_constant(const void *context, unsigned kind, unsigned value) {
	(void) context;
	return kind == U7_CONST_EVENT && value == 2U ? "_Action" : NULL;
}

/* Constants of a known kind take their names, and a script reads action by action. */
static void test_constant_names(void) {
	static const uint8_t code[] = {
		0x48,                   /* push_event                   */
		0x1F, 0x02, 0x00,       /* push_word 2                  */
		0x22,                   /* equal                        */
		0x05, 0x16, 0x00,       /* jump_if_false ->R            */
		0x1F, 0x07, 0x00,       /* push_word 7                  */
		0x44, 0x46,             /* push_byte 70                 */
		0x1F, 0x01, 0x00,       /* push_word 1                  */
		0x44, 0x27,             /* push_byte 39                 */
		0x1E, 0x04, 0x00,       /* make_array 4                 */
		0x44, 0x05,             /* push_byte 5                  */
		0x38, 0x01, 0x00, 0x02, /* PostAction, keeping a result */
		0x12, 0x00, 0x00,       /* pop_local v0                 */
		0x25                    /* R: return                    */
	};
	static uint8_t argument_kinds[256][8];
	static U7ScriptAction actions[256];
	argument_kinds[0x01][1] = U7_CONST_SCRIPT;
	actions[39] = (U7ScriptAction) {"_aWAIT_i", 1, {0}};
	actions[70] = (U7ScriptAction) {"_aFRAME", 1, {0}};
	U7Names names = {0};
	names.constant_name = test_constant;
	names.argument_kinds = argument_kinds;
	names.script_actions = actions;
	char text[2048];
	emit_styled(code, sizeof code, "x", 2U, 0U, 1U, &names, U7_EMIT_ANNOTATED, text, sizeof text);
	check(strstr(text, "(event = _Action)") != NULL, "an event compared by name");
	check(strstr(text, "<< _aWAIT_i, 1, _aFRAME, 7 >>") != NULL, "a script read action by action");
	emit_styled(code, sizeof code, "x", 2U, 0U, 1U, &names, U7_EMIT_LINEAR, text, sizeof text);
	check(strstr(text, "_a") == NULL && strstr(text, "_Action") == NULL,
			"the compiler's form keeps the numbers");
}

int main(void) {
	test_constant_names();
	test_switch_views();
	test_names_in_annotated_listing();
	test_loop_working_before_its_test();
	test_print_ending_a_block();
	test_conditional_emits_agil();
	test_print_construct();
	test_list_forms();
	if (failures == 0) {
		printf("test_emit: all checks passed\n");
	}
	return failures == 0 ? 0 : 1;
}
