#include "u7/parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Slot {
	const char *text;
	size_t length;
	unsigned rank;
} Slot;

typedef struct Block {
	U7Stmt *first;
	U7Stmt *last;
	size_t count;
} Block;

/* A label and the block it marks, once one starts after it. */
typedef struct Label {
	size_t number;
	size_t block;
	bool placed;
	bool waiting;             /* Placed, but no block has started since. */
} Label;

/* Where break and continue go inside a loop. */
typedef struct Loop {
	size_t top;
	size_t end;
	bool broken;              /* Some break leaves it. */
} Loop;

typedef struct Parser {
	const char *at;
	const char *end;
	size_t line;
	U7Parsed *parsed;

	/* The readable form, whose names resolve through a lookup. */
	bool readable;
	U7NameLookup lookup;
	const void *context;

	/* Per function. */
	uint16_t *links;
	size_t link_count;
	Slot *slots;              /* Data slots in first-reference order. */
	size_t slot_count;
	Block *blocks;
	size_t block_count;
	size_t block_capacity;
	bool open;                /* The last block takes more statements. */
	Label *labels;
	size_t label_count;
	size_t label_capacity;
	size_t next_label;        /* Labels made for constructs, above any written. */
	bool *seen;               /* Local slots in the order the text first names them. */
	size_t seen_count;
	long highest_local;       /* The highest slot the code uses, or -1. */
	bool statement_start;     /* The next primary starts a statement. */
	int top_call_returns;     /* What a statement's leading call returns: -1 unknown. */
} Parser;

static bool fail(Parser *parser, const char *what) {
	if (parser->parsed->error[0] == 0) {
		parser->parsed->error_line = parser->line;
		snprintf(parser->parsed->error, sizeof parser->parsed->error, "%s", what);
	}
	return false;
}

static void skip_space(Parser *parser) {
	while (parser->at < parser->end) {
		char c = *parser->at;
		if (c == '\n') {
			parser->line += 1U;
			parser->at += 1;
		} else if (c == ' ' || c == '\t' || c == '\r') {
			parser->at += 1;
		} else if (c == '/' && parser->at + 1 < parser->end && parser->at[1] == '/') {
			while (parser->at < parser->end && *parser->at != '\n') {
				parser->at += 1;
			}
		} else if (c == '/' && parser->at + 1 < parser->end && parser->at[1] == '*') {
			parser->at += 2;
			while (parser->at < parser->end &&
					!(*parser->at == '*' && parser->at + 1 < parser->end && parser->at[1] == '/')) {
				parser->line += *parser->at == '\n' ? 1U : 0U;
				parser->at += 1;
			}
			parser->at = parser->at < parser->end ? parser->at + 2 : parser->end;
		} else {
			return;
		}
	}
}

static bool at_text(Parser *parser, const char *text) {
	skip_space(parser);
	size_t length = strlen(text);
	return (size_t) (parser->end - parser->at) >= length &&
			memcmp(parser->at, text, length) == 0;
}

static bool take(Parser *parser, const char *text) {
	if (!at_text(parser, text)) {
		return false;
	}
	parser->at += strlen(text);
	return true;
}

static bool expect(Parser *parser, const char *text) {
	if (take(parser, text)) {
		return true;
	}
	char message[64];
	snprintf(message, sizeof message, "expected '%s'", text);
	return fail(parser, message);
}

static bool is_word_char(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') || c == '_';
}

/* Matches a keyword only when it is not the start of a longer word. */
static bool at_word(Parser *parser, const char *word) {
	skip_space(parser);
	size_t length = strlen(word);
	return (size_t) (parser->end - parser->at) >= length &&
			memcmp(parser->at, word, length) == 0 &&
			!(parser->at + length < parser->end && is_word_char(parser->at[length]));
}

static bool take_word(Parser *parser, const char *word) {
	if (!at_word(parser, word)) {
		return false;
	}
	parser->at += strlen(word);
	return true;
}

/* A name: letters, digits and underscores. */
static bool take_name(Parser *parser, const char **out_name, size_t *out_length) {
	skip_space(parser);
	const char *start = parser->at;
	while (parser->at < parser->end && is_word_char(*parser->at)) {
		parser->at += 1;
	}
	*out_name = start;
	*out_length = (size_t) (parser->at - start);
	return *out_length > 0;
}

static bool take_number(Parser *parser, long *out_value) {
	skip_space(parser);
	const char *start = parser->at;
	if (start < parser->end && (*start == '-' || *start == '+')) {
		start += 1;
	}
	if (start >= parser->end || *start < '0' || *start > '9') {
		return false;
	}
	char *stop = NULL;
	long value = strtol(parser->at, &stop, 10);
	if (stop == parser->at) {
		return false;
	}
	parser->at = stop;
	*out_value = value;
	return true;
}

static bool take_hex(Parser *parser, unsigned long *out_value) {
	skip_space(parser);
	char *stop = NULL;
	unsigned long value = strtoul(parser->at, &stop, 16);
	if (stop == parser->at) {
		return false;
	}
	parser->at = stop;
	*out_value = value;
	return true;
}

/* Undoes one escape the listings write: \n, \xHH, or a backslashed character. */
static bool take_escape(Parser *parser, char *out) {
	if (parser->at >= parser->end) {
		return fail(parser, "text ends in a backslash");
	}
	char escape = *parser->at++;
	if (escape == 'n') {
		*out = '\n';
	} else if (escape == 'x') {
		char digits[3] = {0};
		if (parser->end - parser->at < 2) {
			return fail(parser, "short hex escape");
		}
		digits[0] = parser->at[0];
		digits[1] = parser->at[1];
		parser->at += 2;
		*out = (char) strtoul(digits, NULL, 16);
	} else {
		*out = escape;
	}
	return true;
}

/* Reads a quoted string into arena storage, undoing the emitted escapes. */
static bool take_string(Parser *parser, const char **out_text, size_t *out_length) {
	skip_space(parser);
	if (parser->at >= parser->end || *parser->at != '"') {
		return false;
	}
	parser->at += 1;

	char *buffer = u7_arena_alloc(&parser->parsed->arena,
			(size_t) (parser->end - parser->at) + 1U);
	if (buffer == NULL) {
		return fail(parser, "out of memory");
	}
	size_t length = 0;
	while (parser->at < parser->end && *parser->at != '"') {
		char c = *parser->at++;
		if (c == '\\' && !take_escape(parser, &c)) {
			return false;
		}
		buffer[length++] = c;
	}
	if (parser->at >= parser->end) {
		return fail(parser, "unterminated string");
	}
	parser->at += 1;
	*out_text = buffer;
	*out_length = length;
	return true;
}

static U7Expr *new_expr(Parser *parser, U7ExprKind kind, uint8_t opcode) {
	U7Expr *expr = u7_arena_alloc(&parser->parsed->arena, sizeof *expr);
	if (expr != NULL) {
		expr->kind = kind;
		expr->opcode = opcode;
	}
	return expr;
}

/*
 * Data slots are identified by their text and, when repeated, their rank. The
 * readable form gives every literal its own slot, as the compiler did.
 */
static int32_t slot_for(Parser *parser, const char *text, size_t length, unsigned rank) {
	for (size_t i = 0; !parser->readable && i < parser->slot_count; ++i) {
		if (parser->slots[i].rank == rank && parser->slots[i].length == length &&
				memcmp(parser->slots[i].text, text, length) == 0) {
			return (int32_t) i;
		}
	}
	Slot *grown = realloc(parser->slots, (parser->slot_count + 1U) * sizeof *grown);
	if (grown == NULL) {
		return -1;
	}
	parser->slots = grown;
	parser->slots[parser->slot_count].text = text;
	parser->slots[parser->slot_count].length = length;
	parser->slots[parser->slot_count].rank = rank;
	return (int32_t) parser->slot_count++;
}

/* Notes that the text names a local slot, for the loops' hidden slots. */
static void mark_local(Parser *parser, long slot) {
	if (slot >= 0 && (size_t) slot < parser->seen_count) {
		parser->seen[slot] = true;
	}
	if (slot > parser->highest_local) {
		parser->highest_local = slot;
	}
}

/* The lowest slot the text has not named yet, which a loop takes for itself. */
static int32_t claim_local(Parser *parser) {
	for (size_t i = 0; i < parser->seen_count; ++i) {
		if (!parser->seen[i]) {
			mark_local(parser, (long) i);
			return (int32_t) i;
		}
	}
	return -1;
}

/* Code laid out in blocks; a label marks the block that starts after it. */
static Block *here(Parser *parser) {
	if (parser->open) {
		return &parser->blocks[parser->block_count - 1U];
	}
	if (parser->block_count == parser->block_capacity) {
		size_t capacity = parser->block_capacity > 0 ? parser->block_capacity * 2U : 64U;
		Block *grown = realloc(parser->blocks, capacity * sizeof *grown);
		if (grown == NULL) {
			fail(parser, "out of memory");
			return NULL;
		}
		parser->blocks = grown;
		parser->block_capacity = capacity;
	}
	Block *block = &parser->blocks[parser->block_count++];
	memset(block, 0, sizeof *block);
	for (size_t i = 0; i < parser->label_count; ++i) {
		if (parser->labels[i].waiting) {
			parser->labels[i].block = parser->block_count - 1U;
			parser->labels[i].waiting = false;
		}
	}
	parser->open = true;
	return block;
}

static Label *find_label(Parser *parser, size_t number, bool create) {
	for (size_t i = 0; i < parser->label_count; ++i) {
		if (parser->labels[i].number == number) {
			return &parser->labels[i];
		}
	}
	if (!create) {
		return NULL;
	}
	if (parser->label_count == parser->label_capacity) {
		size_t capacity = parser->label_capacity > 0 ? parser->label_capacity * 2U : 64U;
		Label *grown = realloc(parser->labels, capacity * sizeof *grown);
		if (grown == NULL) {
			fail(parser, "out of memory");
			return NULL;
		}
		parser->labels = grown;
		parser->label_capacity = capacity;
	}
	Label *label = &parser->labels[parser->label_count++];
	memset(label, 0, sizeof *label);
	label->number = number;
	return label;
}

/* The label marks whatever comes next. */
static bool place_label(Parser *parser, size_t number) {
	Label *label = find_label(parser, number, true);
	if (label == NULL) {
		return false;
	}
	if (label->placed) {
		return fail(parser, "a label is placed twice");
	}
	label->placed = true;
	if (parser->open && parser->blocks[parser->block_count - 1U].count == 0) {
		label->block = parser->block_count - 1U;
	} else {
		parser->open = false;
		label->waiting = true;
	}
	return true;
}

static size_t new_label(Parser *parser) {
	return parser->next_label++;
}

static U7Stmt *new_stmt(Parser *parser, Block *block, U7StmtKind kind, uint8_t opcode) {
	if (block == NULL) {
		return NULL;
	}
	U7Stmt *statement = u7_arena_alloc(&parser->parsed->arena, sizeof *statement);
	if (statement == NULL) {
		return NULL;
	}
	statement->kind = kind;
	statement->opcode = opcode;
	if (block->last == NULL) {
		block->first = statement;
	} else {
		block->last->next = statement;
	}
	block->last = statement;
	block->count += 1U;
	return statement;
}

/* A statement that transfers control to a label. */
static U7Stmt *jump_to(Parser *parser, U7StmtKind kind, uint8_t opcode, size_t label) {
	U7Stmt *statement = new_stmt(parser, here(parser), kind, opcode);
	if (statement != NULL) {
		statement->branch_target = label;
	}
	return statement;
}

/* The language document's own tables give two spellings for several of these:
 * "is" for "=", "!=" for "<>", and "yes"/"no" for "true"/"false". */
static const struct { const char *text; uint8_t opcode; } BINARY_OPS[] = {
	{"modulo", 0x0DU}, {"and", 0x0EU}, {"or", 0x0FU}, {"in", 0x30U}, {"is", 0x22U},
	{">=", 0x18U}, {"<=", 0x19U}, {"<>", 0x1AU}, {"!=", 0x1AU},
	{"+", 0x09U}, {"-", 0x0AU}, {"/", 0x0BU}, {"*", 0x0CU},
	{">", 0x16U}, {"<", 0x17U}, {"=", 0x22U}, {"&", 0x4AU}
};

static U7Expr *parse_expr(Parser *parser);

static bool parse_arguments(Parser *parser, U7Expr *expr, const char *close) {
	U7Expr *collected[256];
	size_t count = 0;
	if (!take(parser, close)) {
		do {
			if (count == 256U) {
				return fail(parser, "too many arguments");
			}
			U7Expr *argument = parse_expr(parser);
			if (argument == NULL) {
				return false;
			}
			collected[count++] = argument;
		} while (take(parser, ","));
		if (!expect(parser, close)) {
			return false;
		}
	}
	if (count > 0) {
		expr->operands = u7_arena_alloc(&parser->parsed->arena,
				count * sizeof *expr->operands);
		if (expr->operands == NULL) {
			return fail(parser, "out of memory");
		}
		memcpy(expr->operands, collected, count * sizeof *collected);
	}
	expr->operand_count = count;
	return true;
}

static U7Expr *binary(Parser *parser, uint8_t opcode, U7Expr *left, U7Expr *right) {
	U7Expr *expr = new_expr(parser, opcode == 0x4AU ? U7_EXPR_JOIN : U7_EXPR_BINARY, opcode);
	if (expr == NULL) {
		return NULL;
	}
	expr->operands = u7_arena_alloc(&parser->parsed->arena, 2U * sizeof *expr->operands);
	if (expr->operands == NULL) {
		return NULL;
	}
	expr->operands[0] = left;
	expr->operands[1] = right;
	expr->operand_count = 2U;
	return expr;
}

static U7Expr *constant(Parser *parser, uint8_t opcode, int32_t value) {
	U7Expr *expr = new_expr(parser, U7_EXPR_CONSTANT, opcode);
	if (expr != NULL) {
		expr->value = value;
	}
	return expr;
}

/*
 * The link slot a call uses: the first listing its target. A target the table
 * lacks is added at its end, since the engine loads what a function links to
 * before running it.
 */
static int32_t link_slot(Parser *parser, uint16_t target) {
	for (size_t i = 0; i < parser->link_count; ++i) {
		if (parser->links[i] == target) {
			return (int32_t) i;
		}
	}
	uint16_t *grown = realloc(parser->links, (parser->link_count + 1U) * sizeof *grown);
	if (grown == NULL) {
		fail(parser, "out of memory");
		return -1;
	}
	parser->links = grown;
	parser->links[parser->link_count] = target;
	return (int32_t) parser->link_count++;
}

static bool resolve(Parser *parser, U7NameKind kind, const char *name, size_t length,
		int32_t *out_value) {
	return parser->lookup != NULL &&
			parser->lookup(parser->context, kind, name, length, out_value);
}

/* A function named by its number, fn_XXXX, or by the name it is declared with. */
static bool function_id(Parser *parser, const char *name, size_t length, int32_t *out_id) {
	if (length == 7U && memcmp(name, "fn_", 3U) == 0) {
		char digits[5] = {0};
		memcpy(digits, name + 3, 4U);
		char *stop = NULL;
		*out_id = (int32_t) strtoul(digits, &stop, 16);
		return stop == digits + 4;
	}
	return resolve(parser, U7_NAME_FUNCTION, name, length, out_id);
}

/*
 * A call by name in the readable form. Usables are called directly and
 * routines through the link table, as the compiler always did; an engine
 * call's ~ says it leaves a result.
 */
static U7Expr *parse_named_call(Parser *parser, const char *name, size_t length,
		bool result, bool leading) {
	long slot = -1;
	if (take(parser, "#") && !take_number(parser, &slot)) {
		fail(parser, "expected a link slot");
		return NULL;
	}
	if (!expect(parser, "(")) {
		return NULL;
	}
	int32_t value = 0;
	U7Expr *expr = NULL;
	if (function_id(parser, name, length, &value)) {
		bool direct = value < 0x800;
		expr = new_expr(parser, U7_EXPR_CALL, direct ? 0x47U : 0x24U);
		int32_t linked = link_slot(parser, (uint16_t) value);
		if (expr == NULL || linked < 0) {
			return NULL;
		}
		expr->value = value;
		expr->detail = direct ? value : slot >= 0 ? (int32_t) slot : linked;
	} else if ((length > 10U && memcmp(name, "intrinsic_", 10U) == 0) ||
			resolve(parser, U7_NAME_INTRINSIC, name, length, &value)) {
		if (length > 10U && memcmp(name, "intrinsic_", 10U) == 0) {
			value = (int32_t) strtoul(name + 10, NULL, 16);
		}
		expr = new_expr(parser, U7_EXPR_INTRINSIC, result ? 0x38U : 0x39U);
		if (expr == NULL) {
			return NULL;
		}
		expr->value = value;
	} else {
		char message[160];
		snprintf(message, sizeof message, "unknown name '%.*s'", (int) length, name);
		fail(parser, message);
		return NULL;
	}
	if (!parse_arguments(parser, expr, ")")) {
		return NULL;
	}
	if (expr->kind == U7_EXPR_INTRINSIC) {
		expr->detail = (int32_t) expr->operand_count;
		return expr;
	}
	/* A call the engine would run with the wrong stack is refused here. */
	int32_t expected = 0;
	int32_t returns = -1;
	char message[200];
	if (resolve(parser, U7_NAME_ARGUMENTS, name, length, &expected) &&
			(size_t) expected != expr->operand_count) {
		snprintf(message, sizeof message, "%.*s takes %d arguments, not %zu", (int) length,
				name, (int) expected, expr->operand_count);
		fail(parser, message);
		return NULL;
	}
	if (!resolve(parser, U7_NAME_RETURNS, name, length, &returns)) {
		returns = -1;
	}
	if (leading) {
		parser->top_call_returns = returns;
	} else if (returns == 0) {
		snprintf(message, sizeof message, "%.*s returns no value to use", (int) length, name);
		fail(parser, message);
		return NULL;
	}
	return expr;
}

/* The names the readable form writes: calls, $flags, #usables and _constants. */
static U7Expr *parse_name(Parser *parser, bool leading) {
	const char *name = NULL;
	size_t length = 0;
	int32_t value = 0;
	if (take(parser, "~")) {
		if (!take_name(parser, &name, &length)) {
			fail(parser, "expected a name after ~");
			return NULL;
		}
		return parse_named_call(parser, name, length, true, leading);
	}
	if (take(parser, "$")) {
		if (!take_name(parser, &name, &length) ||
				!resolve(parser, U7_NAME_FLAG, name, length, &value)) {
			fail(parser, "unknown flag");
			return NULL;
		}
		U7Expr *expr = new_expr(parser, U7_EXPR_FLAG, 0x42U);
		if (expr != NULL) {
			expr->value = value;
		}
		return expr;
	}
	if (take(parser, "#")) {
		if (!take_name(parser, &name, &length) || !function_id(parser, name, length, &value)) {
			fail(parser, "unknown usable");
			return NULL;
		}
		return constant(parser, 0x1FU, value);
	}
	if (!take_name(parser, &name, &length)) {
		fail(parser, "expected an expression");
		return NULL;
	}
	if (name[0] == '_') {
		if (resolve(parser, U7_NAME_ACTION, name, length, &value)) {
			return constant(parser, 0x44U, value);
		}
		if (resolve(parser, U7_NAME_CONSTANT, name, length, &value)) {
			return constant(parser, 0x1FU, value);
		}
		char message[160];
		snprintf(message, sizeof message, "unknown constant '%.*s'", (int) length, name);
		fail(parser, message);
		return NULL;
	}
	return parse_named_call(parser, name, length, false, leading);
}

static U7Expr *parse_primary(Parser *parser) {
	long number = 0;
	unsigned long hex = 0;
	/* Only a call that starts a statement may leave its value unused. */
	bool leading = parser->statement_start;
	parser->statement_start = false;

	if (take_word(parser, "true") || take_word(parser, "yes")) {
		return constant(parser, 0x13U, 1);
	}
	if (take_word(parser, "false") || take_word(parser, "no")) {
		return new_expr(parser, U7_EXPR_CONSTANT, 0x14U);
	}
	/* A value the block was handed rather than computed; it emits nothing. */
	if (take(parser, "#incoming")) {
		if (!expect(parser, "(") || !take_number(parser, &number) ||
				!expect(parser, ")")) {
			return NULL;
		}
		U7Expr *expr = new_expr(parser, U7_EXPR_INCOMING, 0);
		if (expr != NULL) {
			expr->value = (int32_t) number;
		}
		return expr;
	}
	if (take_word(parser, "item")) {
		return new_expr(parser, U7_EXPR_ITEM, 0x3EU);
	}
	if (take_word(parser, "event")) {
		return new_expr(parser, U7_EXPR_EVENT, 0x48U);
	}
	if (take_word(parser, "not")) {
		U7Expr *inner = parse_expr(parser);
		U7Expr *expr = new_expr(parser, U7_EXPR_UNARY, 0x10U);
		if (inner == NULL || expr == NULL) {
			return NULL;
		}
		expr->operands = u7_arena_alloc(&parser->parsed->arena, sizeof *expr->operands);
		if (expr->operands == NULL) {
			return NULL;
		}
		expr->operands[0] = inner;
		expr->operand_count = 1U;
		return expr;
	}
	if (take_word(parser, "byte")) {
		if (!expect(parser, "(") || !take_number(parser, &number) ||
				!expect(parser, ")")) {
			return NULL;
		}
		return constant(parser, 0x44U, (int32_t) number);
	}
	if (take_word(parser, "flag")) {
		if (!expect(parser, "(") || !take_number(parser, &number) ||
				!expect(parser, ")")) {
			return NULL;
		}
		U7Expr *expr = new_expr(parser, U7_EXPR_FLAG, 0x42U);
		if (expr != NULL) {
			expr->value = (int32_t) number;
		}
		return expr;
	}
	if (take(parser, "<<")) {
		U7Expr *expr = new_expr(parser, U7_EXPR_ARRAY, 0x1EU);
		if (expr == NULL || !parse_arguments(parser, expr, ">>")) {
			return NULL;
		}
		return expr;
	}
	if (take(parser, "(")) {
		U7Expr *left = parse_expr(parser);
		if (left == NULL) {
			return NULL;
		}
		for (size_t i = 0; i < sizeof BINARY_OPS / sizeof BINARY_OPS[0]; ++i) {
			bool matched = is_word_char(BINARY_OPS[i].text[0])
					? take_word(parser, BINARY_OPS[i].text)
					: take(parser, BINARY_OPS[i].text);
			if (!matched) {
				continue;
			}
			U7Expr *right = parse_expr(parser);
			if (right == NULL || !expect(parser, ")")) {
				return NULL;
			}
			return binary(parser, BINARY_OPS[i].opcode, left, right);
		}
		fail(parser, "expected an operator inside parentheses");
		return NULL;
	}
	if (at_text(parser, "\"")) {
		const char *text = NULL;
		size_t length = 0;
		if (!take_string(parser, &text, &length)) {
			return NULL;
		}
		unsigned rank = 0;
		if (take(parser, "#")) {
			if (!take_number(parser, &number)) {
				fail(parser, "expected a slot number");
				return NULL;
			}
			rank = (unsigned) number;
		}
		int32_t slot = slot_for(parser, text, length, rank);
		U7Expr *expr = new_expr(parser, U7_EXPR_STRING, 0x1DU);
		if (expr == NULL || slot < 0) {
			return NULL;
		}
		expr->text = text;
		expr->text_length = length;
		expr->detail = slot;
		return expr;
	}
	if (!parser->readable && at_text(parser, "intrinsic_")) {
		parser->at += strlen("intrinsic_");
		if (!take_hex(parser, &hex) || !expect(parser, "(")) {
			return NULL;
		}
		U7Expr *expr = new_expr(parser, U7_EXPR_INTRINSIC, 0x38U);
		if (expr == NULL || !parse_arguments(parser, expr, ")")) {
			return NULL;
		}
		expr->value = (int32_t) hex;
		expr->detail = (int32_t) expr->operand_count;
		return expr;
	}
	if (!parser->readable && at_text(parser, "fn_")) {
		parser->at += strlen("fn_");
		if (!take_hex(parser, &hex)) {
			fail(parser, "expected a function number");
			return NULL;
		}
		bool direct = take(parser, "!");
		long slot = -1;
		if (take(parser, "#")) {
			if (!take_number(parser, &slot)) {
				fail(parser, "expected a link slot");
				return NULL;
			}
		}
		if (!expect(parser, "(")) {
			return NULL;
		}
		U7Expr *expr = new_expr(parser, U7_EXPR_CALL, direct ? 0x47U : 0x24U);
		if (expr == NULL || !parse_arguments(parser, expr, ")")) {
			return NULL;
		}
		expr->value = (int32_t) hex;
		expr->detail = direct ? (int32_t) hex
				: slot >= 0 ? (int32_t) slot : link_slot(parser, (uint16_t) hex);
		return expr;
	}
	if (at_text(parser, "v")) {
		const char *save = parser->at;
		parser->at += 1;
		if (take_number(parser, &number)) {
			U7Expr *expr = new_expr(parser, U7_EXPR_LOCAL, 0x21U);
			if (expr == NULL) {
				return NULL;
			}
			expr->value = (int32_t) number;
			mark_local(parser, number);
			return expr;
		}
		parser->at = save;
	}
	if (take_number(parser, &number)) {
		return constant(parser, 0x1FU, (int32_t) number);
	}
	if (parser->readable) {
		return parse_name(parser, leading);
	}

	fail(parser, "expected an expression");
	return NULL;
}

static U7Expr *parse_expr(Parser *parser) {
	U7Expr *expr = parse_primary(parser);
	if (expr == NULL) {
		return NULL;
	}
	if (!take(parser, "::")) {
		return expr;
	}
	if (expr->kind != U7_EXPR_LOCAL) {
		fail(parser, "a subscript needs a local on its left");
		return NULL;
	}
	/* The index may be a subscript itself, so it binds to the right. */
	U7Expr *index = parse_expr(parser);
	U7Expr *element = new_expr(parser, U7_EXPR_ELEMENT, 0x26U);
	if (index == NULL || element == NULL) {
		return NULL;
	}
	element->value = expr->value;
	element->operands = u7_arena_alloc(&parser->parsed->arena,
			sizeof *element->operands);
	if (element->operands == NULL) {
		return NULL;
	}
	element->operands[0] = index;
	element->operand_count = 1U;
	return element;
}

/* Reads "goto label_N;" and stores the label number. */
static bool take_goto(Parser *parser, U7Stmt *statement) {
	long number = 0;
	if (statement == NULL || !expect(parser, "goto") || !expect(parser, "label_") ||
			!take_number(parser, &number) || !expect(parser, ";")) {
		return false;
	}
	statement->branch_target = (size_t) number;
	return true;
}

/* A statement both forms share, and the linear form's own jumps. */
static bool parse_statement(Parser *parser) {
	long number = 0;
	long second = 0;

	if (take_word(parser, "goto")) {
		if (!expect(parser, "label_") || !take_number(parser, &number) ||
				!expect(parser, ";")) {
			return false;
		}
		return jump_to(parser, U7_STMT_BRANCH, 0x06U, (size_t) number) != NULL;
	}
	if (take_word(parser, "if")) {
		if (!expect(parser, "not")) {
			return false;
		}
		U7Expr *condition = parse_expr(parser);
		U7Stmt *statement = new_stmt(parser, here(parser), U7_STMT_BRANCH_IF_FALSE, 0x05U);
		if (condition == NULL || statement == NULL) {
			return false;
		}
		statement->value = condition;
		return take_goto(parser, statement);
	}
	if (take_word(parser, "ask")) {
		return take_goto(parser, new_stmt(parser, here(parser), U7_STMT_ASK, 0x04U));
	}
	if (take_word(parser, "answer")) {
		U7Stmt *statement = new_stmt(parser, here(parser), U7_STMT_COMPARE_ANSWERS, 0x07U);
		U7Expr *list = new_expr(parser, U7_EXPR_ARRAY, 0x07U);
		if (statement == NULL || list == NULL || !expect(parser, "(") ||
				!parse_arguments(parser, list, ")")) {
			return false;
		}
		statement->value = list;
		statement->operands[0] = (int32_t) list->operand_count;
		return take_goto(parser, statement);
	}
	if (take_word(parser, "default")) {
		U7Stmt *statement = new_stmt(parser, here(parser), U7_STMT_DEFAULT_ANSWER, 0x31U);
		if (statement == NULL || !expect(parser, "(") ||
				!take_number(parser, &number) || !expect(parser, ")")) {
			return false;
		}
		statement->operands[0] = (int32_t) number;
		return take_goto(parser, statement);
	}
	if (take_word(parser, "iterate_setup")) {
		return new_stmt(parser, here(parser), U7_STMT_LOOP_START, 0x2EU) != NULL &&
				expect(parser, ";");
	}
	if (take_word(parser, "iterate")) {
		U7Stmt *statement = new_stmt(parser, here(parser), U7_STMT_LOOP, 0x02U);
		if (statement == NULL || !expect(parser, "(")) {
			return false;
		}
		for (size_t i = 0; i < 4U; ++i) {
			if (!expect(parser, "v") || !take_number(parser, &number)) {
				return false;
			}
			statement->operands[i] = (int32_t) number;
			mark_local(parser, number);
			if (i < 3U && !expect(parser, ",")) {
				return false;
			}
		}
		if (!expect(parser, ")")) {
			return false;
		}
		return take_goto(parser, statement);
	}
	if (take_word(parser, "return_alt")) {
		return new_stmt(parser, here(parser), U7_STMT_RETURN, 0x2CU) != NULL &&
				expect(parser, ";");
	}
	if (take_word(parser, "return_zero")) {
		return new_stmt(parser, here(parser), U7_STMT_RETURN, 0x32U) != NULL &&
				expect(parser, ";");
	}
	if (take_word(parser, "return")) {
		/* The linear form's bare return is the closing one; the readable
		 * form's is an early return, the closing one being implicit there. */
		if (take(parser, ";")) {
			return new_stmt(parser, here(parser), U7_STMT_RETURN,
					parser->readable ? 0x2CU : 0x25U) != NULL;
		}
		U7Expr *value = parse_expr(parser);
		U7Stmt *statement = new_stmt(parser, here(parser), U7_STMT_RETURN, 0x2DU);
		if (value == NULL || statement == NULL) {
			return false;
		}
		statement->value = value;
		return expect(parser, ";");
	}
	if (take_word(parser, "clear_answers")) {
		return new_stmt(parser, here(parser), U7_STMT_CALL, 0x40U) != NULL &&
				expect(parser, ";");
	}
	/* "terminate" is Origin's keyword; "abort" is kept for older listings. */
	if (take_word(parser, "terminate") || take_word(parser, "abort")) {
		return new_stmt(parser, here(parser), U7_STMT_ABORT, 0x3FU) != NULL &&
				expect(parser, ";");
	}
	if (take_word(parser, "say")) {
		return new_stmt(parser, here(parser), U7_STMT_SAY, 0x33U) != NULL &&
				expect(parser, ";");
	}
	if (take_word(parser, "append")) {
		U7Expr *value = parse_expr(parser);
		if (value == NULL) {
			return false;
		}
		/* A string joins the message from the data segment, a local by slot. */
		bool literal = value->kind == U7_EXPR_STRING;
		U7Stmt *statement = new_stmt(parser, here(parser), U7_STMT_APPEND,
				literal ? 0x1CU : 0x2FU);
		if (statement == NULL) {
			return false;
		}
		if (literal) {
			value->opcode = 0x1CU;
		} else if (value->kind != U7_EXPR_LOCAL) {
			return fail(parser, "append takes a string or a local");
		} else {
			statement->operands[0] = value->value;
		}
		statement->value = value;
		return expect(parser, ";");
	}
	if (take(parser, "#line")) {
		U7Stmt *statement = new_stmt(parser, here(parser), U7_STMT_DEBUG, 0x4CU);
		if (statement == NULL || !expect(parser, "(") ||
				!take_number(parser, &number) || !expect(parser, ")") ||
				!expect(parser, ";")) {
			return false;
		}
		statement->operands[0] = (int32_t) number;
		return true;
	}
	if (take(parser, "#names")) {
		U7Stmt *statement = new_stmt(parser, here(parser), U7_STMT_DEBUG, 0x4DU);
		if (statement == NULL || !expect(parser, "(") ||
				!take_number(parser, &number) || !expect(parser, ",") ||
				!take_number(parser, &second) || !expect(parser, ")") ||
				!expect(parser, ";")) {
			return false;
		}
		statement->operands[0] = (int32_t) number;
		statement->operands[1] = (int32_t) second;
		return true;
	}
	if (take(parser, "#discard")) {
		U7Expr *value = parse_expr(parser);
		U7Stmt *statement = new_stmt(parser, here(parser), U7_STMT_DISCARD, 0);
		if (value == NULL || statement == NULL) {
			return false;
		}
		statement->value = value;
		return expect(parser, ";");
	}

	/* Everything else starts with an expression: an assignment or a call. */
	parser->statement_start = true;
	parser->top_call_returns = -1;
	U7Expr *target = parse_expr(parser);
	parser->statement_start = false;
	if (target == NULL) {
		return false;
	}
	if (!take(parser, "=")) {
		if (target->kind == U7_EXPR_CALL && parser->top_call_returns == 1) {
			return fail(parser, "the call's value goes unused, but the function returns one");
		}
		U7Stmt *statement = new_stmt(parser, here(parser), U7_STMT_CALL, 0);
		if (statement == NULL) {
			return false;
		}
		if (target->kind == U7_EXPR_INTRINSIC) {
			target->opcode = 0x39U;   /* A statement keeps no result. */
		}
		statement->value = target;
		return expect(parser, ";");
	}

	if (target->kind == U7_EXPR_CALL && parser->top_call_returns == 0) {
		return fail(parser, "a call cannot be assigned to");
	}
	U7Expr *value = parse_expr(parser);
	if (value == NULL) {
		return false;
	}
	U7Stmt *statement = NULL;
	switch (target->kind) {
		case U7_EXPR_LOCAL:
			statement = new_stmt(parser, here(parser), U7_STMT_ASSIGN_LOCAL, 0x12U);
			break;
		case U7_EXPR_FLAG:
			statement = new_stmt(parser, here(parser), U7_STMT_ASSIGN_FLAG, 0x43U);
			break;
		case U7_EXPR_EVENT:
			statement = new_stmt(parser, here(parser), U7_STMT_ASSIGN_EVENT, 0x4BU);
			break;
		case U7_EXPR_ELEMENT:
			statement = new_stmt(parser, here(parser), U7_STMT_ASSIGN_ELEMENT, 0x46U);
			break;
		default:
			return fail(parser, "this cannot be assigned to");
	}
	if (statement == NULL) {
		return false;
	}
	statement->slot = target->kind == U7_EXPR_EVENT ? 0 : target->value;
	statement->value = value;
	if (target->kind == U7_EXPR_ELEMENT) {
		statement->subscript = target->operands[0];
	}
	return expect(parser, ";");
}

static bool parse_body(Parser *parser, Loop *loop, bool *out_falls);

/* A braced body; says whether its end can be reached. */
static bool parse_braced(Parser *parser, Loop *loop, bool *out_falls) {
	return expect(parser, "{") && parse_body(parser, loop, out_falls);
}

/* The bracketed print: text and <local>s appended in turn, then said. */
static bool parse_print(Parser *parser) {
	char *buffer = u7_arena_alloc(&parser->parsed->arena,
			(size_t) (parser->end - parser->at) + 1U);
	if (buffer == NULL) {
		return fail(parser, "out of memory");
	}
	size_t length = 0;
	while (parser->at < parser->end && *parser->at != ']') {
		char c = *parser->at++;
		if (c == '\n') {
			return fail(parser, "a print runs past its line");
		}
		if (c == '\\') {
			if (!take_escape(parser, &c)) {
				return false;
			}
			buffer[length++] = c;
			continue;
		}
		if (c != '<') {
			buffer[length++] = c;
			continue;
		}
		if (length > 0) {
			U7Expr *text = new_expr(parser, U7_EXPR_STRING, 0x1CU);
			U7Stmt *statement = new_stmt(parser, here(parser), U7_STMT_APPEND, 0x1CU);
			int32_t slot = slot_for(parser, buffer, length, 0);
			if (text == NULL || statement == NULL || slot < 0) {
				return fail(parser, "out of memory");
			}
			text->text = buffer;
			text->text_length = length;
			text->detail = slot;
			statement->value = text;
			buffer += length;
			length = 0;
		}
		U7Expr *local = parse_expr(parser);
		U7Stmt *statement = new_stmt(parser, here(parser), U7_STMT_APPEND, 0x2FU);
		if (local == NULL || statement == NULL || local->kind != U7_EXPR_LOCAL ||
				!expect(parser, ">")) {
			return fail(parser, "a print names a local between < and >");
		}
		statement->operands[0] = local->value;
		statement->value = local;
	}
	if (length > 0) {
		U7Expr *text = new_expr(parser, U7_EXPR_STRING, 0x1CU);
		U7Stmt *statement = new_stmt(parser, here(parser), U7_STMT_APPEND, 0x1CU);
		int32_t slot = slot_for(parser, buffer, length, 0);
		if (text == NULL || statement == NULL || slot < 0) {
			return fail(parser, "out of memory");
		}
		text->text = buffer;
		text->text_length = length;
		text->detail = slot;
		statement->value = text;
	}
	return expect(parser, "]") && expect(parser, ";") &&
			new_stmt(parser, here(parser), U7_STMT_SAY, 0x33U) != NULL;
}

/* An if: each test of a short-circuit and fails to the else, or past the arm. */
static bool parse_if(Parser *parser, Loop *loop, bool *reachable) {
	U7Expr *tests[16];
	size_t count = 0;
	do {
		if (count == 16U) {
			return fail(parser, "too many tests joined by &&");
		}
		tests[count] = parse_expr(parser);
		if (tests[count++] == NULL) {
			return false;
		}
	} while (take(parser, "&&"));

	/* The linear form's own jump: if not c goto label_N. */
	if (count == 1U && at_word(parser, "goto")) {
		if (tests[0]->kind != U7_EXPR_UNARY || tests[0]->opcode != 0x10U) {
			return fail(parser, "a conditional goto is written if not");
		}
		U7Stmt *statement = new_stmt(parser, here(parser), U7_STMT_BRANCH_IF_FALSE, 0x05U);
		if (statement == NULL) {
			return false;
		}
		statement->value = tests[0]->operands[0];
		return take_goto(parser, statement);
	}

	size_t otherwise = new_label(parser);
	for (size_t i = 0; i < count; ++i) {
		U7Stmt *test = jump_to(parser, U7_STMT_BRANCH_IF_FALSE, 0x05U, otherwise);
		if (test == NULL) {
			return false;
		}
		test->value = tests[i];
	}
	bool first_falls = true;
	if (!parse_braced(parser, loop, &first_falls)) {
		return false;
	}
	if (!take_word(parser, "else")) {
		*reachable = true;
		return place_label(parser, otherwise);
	}
	/* The compiler ends the first arm with a jump past the else, reached or not. */
	size_t end = new_label(parser);
	bool second_falls = true;
	if (jump_to(parser, U7_STMT_BRANCH, 0x06U, end) == NULL ||
			!place_label(parser, otherwise) ||
			!parse_braced(parser, loop, &second_falls) || !place_label(parser, end)) {
		return false;
	}
	*reachable = first_falls || second_falls;
	return true;
}

/* A switch is an else ladder: each case tests the selector afresh. */
static bool parse_switch(Parser *parser, Loop *loop, bool *reachable) {
	U7Expr *selector = parse_expr(parser);
	if (selector == NULL || !expect(parser, "{")) {
		return false;
	}
	size_t end = new_label(parser);
	bool falls = false;
	bool otherwise = false;
	while (take_word(parser, "case")) {
		U7Expr *values[64];
		size_t count = 0;
		do {
			if (count == 64U) {
				return fail(parser, "too many values in a case");
			}
			values[count] = parse_expr(parser);
			if (values[count++] == NULL) {
				return false;
			}
		} while (take(parser, ","));
		/* (s = a) or ((s = b) or (s = c)), as the ladder's tests were written. */
		U7Expr *test = binary(parser, 0x22U, selector, values[count - 1U]);
		for (size_t i = count - 1U; test != NULL && i > 0; --i) {
			U7Expr *left = binary(parser, 0x22U, selector, values[i - 1U]);
			test = left != NULL ? binary(parser, 0x0FU, left, test) : NULL;
		}
		size_t next = new_label(parser);
		U7Stmt *statement = jump_to(parser, U7_STMT_BRANCH_IF_FALSE, 0x05U, next);
		bool arm_falls = true;
		if (test == NULL || statement == NULL || !parse_braced(parser, loop, &arm_falls)) {
			return false;
		}
		statement->value = test;
		falls = falls || arm_falls;
		if ((at_word(parser, "case") || at_word(parser, "default")) &&
				jump_to(parser, U7_STMT_BRANCH, 0x06U, end) == NULL) {
			return false;
		}
		if (!place_label(parser, next)) {
			return false;
		}
	}
	if (take_word(parser, "default")) {
		bool arm_falls = true;
		if (!parse_braced(parser, loop, &arm_falls)) {
			return false;
		}
		falls = falls || arm_falls;
		otherwise = true;
	}
	*reachable = falls || !otherwise;
	return expect(parser, "}") && place_label(parser, end);
}

/* Reads vN without noting it, so a loop's hidden slots can be taken first. */
static bool take_local(Parser *parser, long *out_slot) {
	return expect(parser, "v") && take_number(parser, out_slot);
}

/* The loops. A break goes past the loop, a continue back to its top. */
static bool parse_loop(Parser *parser, const char *keyword, bool *reachable) {
	Loop loop = {new_label(parser), new_label(parser), false};
	bool falls = true;
	bool tested = true;
	if (strcmp(keyword, "Foreach") == 0) {
		/* The counter and count are the compiler's, the next two slots free. */
		long element = 0;
		long list = 0;
		if (!take_local(parser, &element) || !take_word(parser, "in") ||
				!take_local(parser, &list)) {
			return fail(parser, "expected Foreach vN in vN");
		}
		int32_t counter = claim_local(parser);
		int32_t count = claim_local(parser);
		mark_local(parser, element);
		mark_local(parser, list);
		if (new_stmt(parser, here(parser), U7_STMT_LOOP_START, 0x2EU) == NULL ||
				!place_label(parser, loop.top)) {
			return false;
		}
		U7Stmt *iterate = jump_to(parser, U7_STMT_LOOP, 0x02U, loop.end);
		if (iterate == NULL) {
			return false;
		}
		iterate->operands[0] = counter;
		iterate->operands[1] = count;
		iterate->operands[2] = (int32_t) element;
		iterate->operands[3] = (int32_t) list;
		if (!parse_braced(parser, &loop, &falls)) {
			return false;
		}
	} else if (strcmp(keyword, "converse") == 0) {
		if (!place_label(parser, loop.top) ||
				jump_to(parser, U7_STMT_ASK, 0x04U, loop.end) == NULL ||
				!parse_braced(parser, &loop, &falls)) {
			return false;
		}
	} else if (strcmp(keyword, "While") == 0) {
		U7Expr *condition = parse_expr(parser);
		U7Stmt *test = NULL;
		if (condition == NULL || !place_label(parser, loop.top) ||
				(test = jump_to(parser, U7_STMT_BRANCH_IF_FALSE, 0x05U, loop.end)) == NULL ||
				!parse_braced(parser, &loop, &falls)) {
			return false;
		}
		test->value = condition;
	} else if (strcmp(keyword, "loop") == 0) {
		/* loop { body }, or loop { work } while test { body }. */
		if (!place_label(parser, loop.top) || !parse_braced(parser, &loop, &falls)) {
			return false;
		}
		tested = take_word(parser, "while");
		if (tested) {
			U7Expr *condition = parse_expr(parser);
			U7Stmt *test = jump_to(parser, U7_STMT_BRANCH_IF_FALSE, 0x05U, loop.end);
			if (condition == NULL || test == NULL || !parse_braced(parser, &loop, &falls)) {
				return false;
			}
			test->value = condition;
		}
	} else {
		/* do { body } while test; the jump back always follows the test. */
		U7Stmt *test = NULL;
		U7Expr *condition = NULL;
		if (!place_label(parser, loop.top) || !parse_braced(parser, &loop, &falls) ||
				!take_word(parser, "while") || (condition = parse_expr(parser)) == NULL ||
				!expect(parser, ";") ||
				(test = jump_to(parser, U7_STMT_BRANCH_IF_FALSE, 0x05U, loop.end)) == NULL) {
			return false;
		}
		test->value = condition;
		falls = true;
	}
	/* A converse or Foreach body always ends in its jump back, reached or not; a
	 * While or loop body only where its end can be reached. */
	bool always = strcmp(keyword, "converse") == 0 || strcmp(keyword, "Foreach") == 0;
	if ((falls || always) && jump_to(parser, U7_STMT_BRANCH, 0x06U, loop.top) == NULL) {
		return false;
	}
	*reachable = tested || loop.broken;
	return place_label(parser, loop.end);
}

/* A conversation key: the arm runs when one of its answers was chosen. */
static bool parse_key(Parser *parser, Loop *loop) {
	size_t next = new_label(parser);
	U7Stmt *statement = NULL;
	long number = 0;
	if (take_word(parser, "default")) {
		statement = jump_to(parser, U7_STMT_DEFAULT_ANSWER, 0x31U, next);
		if (statement == NULL || !expect(parser, "(") || !take_number(parser, &number) ||
				!expect(parser, ")")) {
			return false;
		}
		statement->operands[0] = (int32_t) number;
	} else {
		statement = jump_to(parser, U7_STMT_COMPARE_ANSWERS, 0x07U, next);
		U7Expr *list = new_expr(parser, U7_EXPR_ARRAY, 0x07U);
		if (statement == NULL || list == NULL || !expect(parser, "(") ||
				!parse_arguments(parser, list, ")")) {
			return false;
		}
		statement->value = list;
		statement->operands[0] = (int32_t) list->operand_count;
	}
	bool falls = true;
	return parse_braced(parser, loop, &falls) && place_label(parser, next);
}

/* One statement of the readable form; says whether what follows can be reached. */
static bool parse_readable_statement(Parser *parser, Loop *loop, bool *reachable) {
	long number = 0;
	if (take(parser, "label_")) {
		*reachable = true;
		return take_number(parser, &number) && expect(parser, ":") &&
				place_label(parser, (size_t) number);
	}
	if (take(parser, "[")) {
		return parse_print(parser);
	}
	if (take_word(parser, "if")) {
		return parse_if(parser, loop, reachable);
	}
	if (take_word(parser, "switch")) {
		return parse_switch(parser, loop, reachable);
	}
	if (take_word(parser, "key")) {
		return parse_key(parser, loop);
	}
	const char *loops[] = {"Foreach", "converse", "While", "loop", "do"};
	for (size_t i = 0; i < sizeof loops / sizeof loops[0]; ++i) {
		if (take_word(parser, loops[i])) {
			return parse_loop(parser, loops[i], reachable);
		}
	}
	bool leaving = take_word(parser, "break");
	if (leaving || take_word(parser, "continue")) {
		if (loop == NULL || !expect(parser, ";")) {
			return fail(parser, "break and continue belong in a loop");
		}
		loop->broken = loop->broken || leaving;
		*reachable = false;
		return jump_to(parser, U7_STMT_BRANCH, 0x06U, leaving ? loop->end : loop->top) != NULL;
	}
	if (at_word(parser, "goto") || at_word(parser, "return") || at_word(parser, "terminate") ||
			at_word(parser, "abort") || at_word(parser, "return_alt") ||
			at_word(parser, "return_zero")) {
		*reachable = false;
	}
	return parse_statement(parser);
}

static bool parse_body(Parser *parser, Loop *loop, bool *out_falls) {
	bool reachable = true;
	while (!take(parser, "}")) {
		if (parser->at >= parser->end) {
			return fail(parser, "a body is not closed");
		}
		if (!parse_readable_statement(parser, loop, &reachable)) {
			return false;
		}
	}
	*out_falls = reachable;
	return true;
}

/* Turns every jump's label into the block it marks. */
static bool resolve_labels(Parser *parser) {
	for (size_t i = 0; i < parser->label_count; ++i) {
		if (parser->labels[i].waiting && here(parser) == NULL) {
			return false;
		}
	}
	for (size_t b = 0; b < parser->block_count; ++b) {
		for (U7Stmt *statement = parser->blocks[b].first; statement != NULL;
				statement = statement->next) {
			if (statement->kind != U7_STMT_BRANCH &&
					statement->kind != U7_STMT_BRANCH_IF_FALSE &&
					statement->kind != U7_STMT_ASK &&
					statement->kind != U7_STMT_COMPARE_ANSWERS &&
					statement->kind != U7_STMT_DEFAULT_ANSWER &&
					statement->kind != U7_STMT_LOOP) {
				continue;
			}
			const Label *label = find_label(parser, statement->branch_target, false);
			if (label == NULL || !label->placed) {
				return fail(parser, "a jump names a label that is not there");
			}
			statement->branch_target = label->block;
		}
	}
	return true;
}

/* Keeps the function laid out so far, then clears the per-function state. */
static bool finish_function(Parser *parser, bool ok, uint16_t id, uint16_t arguments,
		uint16_t locals) {
	if (ok) {
		ok = resolve_labels(parser);
	}
	if (ok) {
		U7ParsedFunction *grown = realloc(parser->parsed->functions,
				(parser->parsed->count + 1U) * sizeof *grown);
		if (grown != NULL) {
			parser->parsed->functions = grown;
		}
		U7LiftedBlock *lifted = u7_arena_alloc(&parser->parsed->arena,
				(parser->block_count + 1U) * sizeof *lifted);
		if (grown == NULL || lifted == NULL) {
			ok = fail(parser, "out of memory");
		} else {
			for (size_t b = 0; b < parser->block_count; ++b) {
				lifted[b].first = parser->blocks[b].first;
				lifted[b].statement_count = parser->blocks[b].count;
			}
			U7ParsedFunction *function = &grown[parser->parsed->count++];
			memset(function, 0, sizeof *function);
			function->function_id = id;
			function->argument_count = arguments;
			function->local_count = locals;
			function->links = parser->links;
			function->link_count = parser->link_count;
			function->blocks = lifted;
			function->block_count = parser->block_count;
			parser->links = NULL;
		}
	}
	free(parser->links);
	free(parser->slots);
	free(parser->seen);
	parser->links = NULL;
	parser->link_count = 0;
	parser->slots = NULL;
	parser->slot_count = 0;
	parser->seen = NULL;
	parser->seen_count = 0;
	parser->block_count = 0;
	parser->open = false;
	parser->label_count = 0;
	return ok;
}

/* A declared function's number: fn_XXXX, #id(0x...), or its name's. */
static bool parse_function_name(Parser *parser, unsigned long *out_id) {
	(void) take(parser, "~");
	const char *name = NULL;
	size_t length = 0;
	int32_t value = 0;
	if (!take_name(parser, &name, &length)) {
		return fail(parser, "expected a function name");
	}
	bool numbered = function_id(parser, name, length, &value);
	if (take(parser, "#id")) {
		return expect(parser, "(") && take_hex(parser, out_id) && expect(parser, ")");
	}
	if (!numbered) {
		return fail(parser, "a function needs a number");
	}
	*out_id = (unsigned long) value;
	return true;
}

/* A record written out as its bytes in hex, between braces. */
static bool parse_record(Parser *parser, uint16_t id) {
	long number = 0;
	uint16_t arguments = 0;
	if (take_word(parser, "accepts")) {
		do {
			if (!expect(parser, "a") || !take_number(parser, &number)) {
				return false;
			}
			arguments += 1U;
		} while (take(parser, ","));
	}
	if (!expect(parser, "{")) {
		return false;
	}
	uint8_t *bytes = u7_arena_alloc(&parser->parsed->arena,
			(size_t) (parser->end - parser->at) / 2U + 1U);
	if (bytes == NULL) {
		return fail(parser, "out of memory");
	}
	size_t size = 0;
	char digits[3] = {0};
	size_t have = 0;
	while (!take(parser, "}")) {
		if (parser->at >= parser->end) {
			return fail(parser, "a record's bytes are not closed");
		}
		char c = *parser->at++;
		bool hex = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
		if (!hex) {
			return fail(parser, "a record's bytes are written in hex");
		}
		digits[have++] = c;
		if (have == 2U) {
			bytes[size++] = (uint8_t) strtoul(digits, NULL, 16);
			have = 0;
		}
	}
	uint16_t declared = 0;
	uint16_t payload = 0;
	uint16_t data_size = 0;
	uint16_t held = 0;
	if (have != 0 || !u7_read_u16(bytes, size, 0, &declared) ||
			!u7_read_u16(bytes, size, 2U, &payload) || declared != id ||
			(size_t) payload + 4U != size || !u7_read_u16(bytes, size, 4U, &data_size) ||
			!u7_read_u16(bytes, size, 6U + (size_t) data_size, &held) || held != arguments) {
		return fail(parser, "a record's bytes do not hold that function as declared");
	}
	U7ParsedFunction *grown = realloc(parser->parsed->functions,
			(parser->parsed->count + 1U) * sizeof *grown);
	if (grown == NULL) {
		return fail(parser, "out of memory");
	}
	parser->parsed->functions = grown;
	U7ParsedFunction *function = &grown[parser->parsed->count++];
	memset(function, 0, sizeof *function);
	function->function_id = id;
	function->record = bytes;
	function->record_size = size;
	return true;
}

static bool parse_function(Parser *parser) {
	unsigned long id = 0;
	long number = 0;

	uint16_t arguments = 0;
	bool closes_with_value = take_word(parser, "Function");
	if (!closes_with_value && !take_word(parser, "Routine") && !take_word(parser, "Usable")) {
		return fail(parser, "expected Function, Routine or Usable");
	}
	/* #linear marks a function the readable form carries in the linear form. */
	bool linear = !parser->readable;
	if (parser->readable) {
		if (!parse_function_name(parser, &id)) {
			return false;
		}
		if (take(parser, "#bytes")) {
			return parse_record(parser, (uint16_t) id);
		}
		linear = take(parser, "#linear");
	} else if (!expect(parser, "fn_") || !take_hex(parser, &id)) {
		return false;
	}
	if (take_word(parser, "accepts")) {
		do {
			if (!expect(parser, "a") || !take_number(parser, &number)) {
				return false;
			}
			arguments += 1U;
		} while (take(parser, ","));
	}
	/* The local count may be left out: the code's own slots set it. */
	uint16_t locals = 0;
	if (take(parser, "#locals")) {
		if (!expect(parser, "(") || !take_number(parser, &number) || !expect(parser, ")")) {
			return false;
		}
		locals = (uint16_t) number;
	}

	if (take(parser, "#uses")) {
		if (!expect(parser, "(")) {
			return false;
		}
		do {
			unsigned long target = 0;
			if (parser->readable) {
				const char *name = NULL;
				size_t length = 0;
				int32_t value = 0;
				(void) take(parser, "~");
				if (!take_name(parser, &name, &length) ||
						!function_id(parser, name, length, &value)) {
					return fail(parser, "#uses names an unknown function");
				}
				target = (unsigned long) value;
			} else if (!expect(parser, "fn_") || !take_hex(parser, &target)) {
				return false;
			}
			uint16_t *grown = realloc(parser->links,
					(parser->link_count + 1U) * sizeof *grown);
			if (grown == NULL) {
				return fail(parser, "out of memory");
			}
			parser->links = grown;
			parser->links[parser->link_count++] = (uint16_t) target;
		} while (take(parser, ","));
		if (!expect(parser, ")")) {
			return false;
		}
	}
	if (!expect(parser, "{")) {
		return false;
	}

	parser->seen_count = 65536U;
	parser->highest_local = arguments > 0 ? (long) arguments - 1 : -1;
	parser->seen = calloc(parser->seen_count, sizeof *parser->seen);
	if (parser->seen == NULL) {
		return finish_function(parser, fail(parser, "out of memory"), 0, 0, 0);
	}
	for (uint16_t a = 0; a < arguments; ++a) {
		parser->seen[a] = true;
	}

	bool ok = true;
	bool readable = parser->readable;
	parser->readable = !linear;
	if (!linear) {
		bool falls = true;
		ok = parse_body(parser, NULL, &falls) &&
				new_stmt(parser, here(parser), U7_STMT_RETURN,
						closes_with_value ? 0x32U : 0x25U) != NULL;
	} else {
		while (ok && !at_text(parser, "}")) {
			if (take(parser, "label_")) {
				ok = take_number(parser, &number) && expect(parser, ":") &&
						place_label(parser, (size_t) number);
				continue;
			}
			ok = parse_statement(parser);
		}
		ok = ok && expect(parser, "}");
	}
	parser->readable = readable;
	/* Enough locals for every slot the code uses, and no fewer than declared. */
	long needed = parser->highest_local + 1 - (long) arguments;
	if (needed > (long) locals && needed <= 0xFFFF) {
		locals = (uint16_t) needed;
	}
	return finish_function(parser, ok, (uint16_t) id, arguments, locals);
}

void u7_parsed_release(U7Parsed *parsed) {
	if (parsed == NULL) {
		return;
	}
	for (size_t i = 0; i < parsed->count; ++i) {
		free(parsed->functions[i].links);
	}
	free(parsed->functions);
	u7_arena_release(&parsed->arena);
	memset(parsed, 0, sizeof *parsed);
}

void u7_parsed_unit(const U7ParsedFunction *function, U7Unit *out_unit) {
	if (function == NULL || out_unit == NULL) {
		return;
	}
	memset(out_unit, 0, sizeof *out_unit);
	out_unit->function_id = function->function_id;
	out_unit->argument_count = function->argument_count;
	out_unit->local_count = function->local_count;
	out_unit->links = function->links;
	out_unit->link_count = function->link_count;
	out_unit->blocks = function->blocks;
	out_unit->block_count = function->block_count;
}

static U7Result parse_all(Parser *parser) {
	U7Result result = U7_OK;
	while (true) {
		skip_space(parser);
		if (parser->at >= parser->end) {
			break;
		}
		if (!parse_function(parser)) {
			result = U7_ERROR_MISMATCH;
			break;
		}
	}
	free(parser->blocks);
	free(parser->labels);
	return result;
}

U7Result u7_parse_source(const char *text, size_t length, U7Parsed *out_parsed) {
	if (text == NULL || out_parsed == NULL) {
		return U7_ERROR_ARGUMENT;
	}
	memset(out_parsed, 0, sizeof *out_parsed);

	Parser parser;
	memset(&parser, 0, sizeof parser);
	parser.at = text;
	parser.end = text + length;
	parser.line = 1U;
	parser.parsed = out_parsed;
	parser.next_label = SIZE_MAX / 2U;
	return parse_all(&parser);
}

U7Result u7_compile_source(const char *text, size_t length, U7NameLookup lookup,
		const void *context, U7Parsed *out_parsed) {
	if (text == NULL || out_parsed == NULL) {
		return U7_ERROR_ARGUMENT;
	}
	memset(out_parsed, 0, sizeof *out_parsed);

	Parser parser;
	memset(&parser, 0, sizeof parser);
	parser.at = text;
	parser.end = text + length;
	parser.line = 1U;
	parser.parsed = out_parsed;
	parser.readable = true;
	parser.lookup = lookup;
	parser.context = context;
	parser.next_label = SIZE_MAX / 2U;
	return parse_all(&parser);
}
