#include "u7/emit.h"

#include <stdlib.h>
#include <string.h>

typedef struct Emitter {
	FILE *out;
	const size_t *targets;   /* Blocks some goto names, so they need a label. */
	size_t target_count;
	bool *labelled;          /* Which of those have had their label printed. */
	const U7Function *function;
	const U7Lifted *lifted;
	const U7Names *names;
	U7EmitStyle style;
	const U7Stmt *epilogue;  /* The closing return, which the source leaves implicit. */
} Emitter;

static void emit_expr(Emitter *emitter, const U7Expr *expr);
static unsigned line_at(const U7Names *names, size_t offset);

static void emit_indent(Emitter *emitter, int depth) {
	for (int i = 0; i < depth; ++i) {
		fputs("    ", emitter->out);
	}
}

/* Original spelling where the corpus carries one, slot number otherwise. */
static void emit_local(Emitter *emitter, int slot) {
	if (emitter->style == U7_EMIT_ANNOTATED && emitter->names != NULL &&
			emitter->names->debug != NULL && slot >= 0 &&
			slot < (int) emitter->names->debug->local_count) {
		const char *text = NULL;
		size_t length = 0;
		if (u7_debug_local_name(emitter->names->function, emitter->names->debug,
				(uint16_t) slot, &text, &length) == U7_OK && length > 0) {
			fprintf(emitter->out, "%.*s", (int) length, text);
			return;
		}
	}
	fprintf(emitter->out, "v%d", slot);
}

/* Origin writes anything that returns a value with a leading ~. */
static void emit_intrinsic_name(Emitter *emitter, int ordinal, bool returns_value) {
	if (emitter->style == U7_EMIT_ANNOTATED && returns_value) {
		fputc('~', emitter->out);
	}
	if (emitter->style == U7_EMIT_ANNOTATED && emitter->names != NULL &&
			emitter->names->intrinsics != NULL && ordinal >= 0 && ordinal < 256 &&
			emitter->names->intrinsics[ordinal] != NULL) {
		fputs(emitter->names->intrinsics[ordinal], emitter->out);
		return;
	}
	fprintf(emitter->out, "intrinsic_%02X", (unsigned) ordinal & 0xFFU);
}

static const char *function_name(const Emitter *emitter, unsigned id) {
	if (emitter->style == U7_EMIT_ANNOTATED && emitter->names != NULL &&
			emitter->names->functions != NULL && id < 65536U) {
		return emitter->names->functions[id];
	}
	return NULL;
}

static void emit_function_name(Emitter *emitter, unsigned id) {
	const char *name = function_name(emitter, id);
	if (emitter->style == U7_EMIT_ANNOTATED && emitter->names != NULL &&
			emitter->names->returns_value != NULL && id < 65536U &&
			emitter->names->returns_value[id] != 0) {
		fputc('~', emitter->out);
	}
	if (name != NULL) {
		fputs(name, emitter->out);
	} else {
		fprintf(emitter->out, "fn_%04X", id);
	}
}

static bool takes_object(const U7Names *names, const U7Expr *call, size_t i) {
	if (call->kind == U7_EXPR_INTRINSIC) {
		size_t from_last = call->operand_count - 1U - i;
		return names->object_arguments != NULL && call->value >= 0 &&
				call->value <= 0xFF && from_last < 8U &&
				(names->object_arguments[call->value] & (1U << from_last)) != 0;
	}
	return names->object_parameters != NULL && call->value >= 0 &&
			call->value <= 0xFFFF && i < 16U &&
			(names->object_parameters[call->value] & (1U << i)) != 0;
}

/* A constant where an object is taken is a reference to an NPC. */
static bool emit_object(Emitter *emitter, const U7Expr *call, size_t i) {
	const U7Names *names = emitter->names;
	const U7Expr *operand = call->operands[i];
	if (emitter->style != U7_EMIT_ANNOTATED || names == NULL || names->objects == NULL ||
			operand == NULL || operand->kind != U7_EXPR_CONSTANT ||
			operand->value < 0 || operand->value > 0xFFFF ||
			names->objects[operand->value] == NULL || !takes_object(names, call, i)) {
		return false;
	}
	fputs(names->objects[operand->value], emitter->out);
	return true;
}

static const char *constant_named(const Emitter *emitter, unsigned kind, const U7Expr *expr) {
	const U7Names *names = emitter->names;
	/* Only a word push takes a name; the others are spelled by their own bytes. */
	if (emitter->style != U7_EMIT_ANNOTATED || names == NULL || names->constant_name == NULL ||
			kind == U7_CONST_NONE || expr == NULL || expr->kind != U7_EXPR_CONSTANT ||
			expr->opcode != 0x1FU || expr->value < 0 || expr->value > 0xFFFF) {
		return NULL;
	}
	return names->constant_name(names->constant_context, kind, (unsigned) expr->value);
}

/* The kind of value an expression yields, where that is known. */
static unsigned kind_of(const Emitter *emitter, const U7Expr *expr) {
	const U7Names *names = emitter->names;
	if (expr == NULL || names == NULL) {
		return U7_CONST_NONE;
	}
	if (expr->kind == U7_EXPR_EVENT) {
		return U7_CONST_EVENT;
	}
	if (expr->kind == U7_EXPR_INTRINSIC && names->result_kinds != NULL && expr->value >= 0 &&
			expr->value < 256) {
		return names->result_kinds[expr->value];
	}
	return U7_CONST_NONE;
}

static void emit_as(Emitter *emitter, const U7Expr *expr, unsigned kind);

static bool is_constant(const U7Expr *expr, int low, int high) {
	return expr != NULL && expr->kind == U7_EXPR_CONSTANT && expr->value >= low &&
			expr->value <= high;
}

/*
 * A script list read action by action: each action's name, then its operands
 * by kind. The engine reads the list as bytes, a value above 255 taking two,
 * so a routine number below 256 is written as its low byte and a 0. Past
 * anything the engine would read differently, the rest is left as it is.
 */
static void emit_script(Emitter *emitter, const U7Expr *list) {
	const U7ScriptAction *actions = emitter->names->script_actions;
	bool reading = true;
	fputs("<<", emitter->out);
	for (size_t i = 0; i < list->operand_count;) {
		const U7Expr *code = list->operands[i];
		fputs(i > 0 ? ", " : " ", emitter->out);
		/* Action codes are byte pushes; a word there is not read as one. */
		const U7ScriptAction *action = reading && is_constant(code, 0, 0xFF) &&
				code->opcode == 0x44U && actions[code->value].name != NULL
				? &actions[code->value] : NULL;
		i += 1U;
		if (action == NULL) {
			reading = false;
			emit_expr(emitter, code);
			continue;
		}
		fputs(action->name, emitter->out);
		for (unsigned k = 0; k < action->operand_count && i < list->operand_count; ++k) {
			unsigned kind = action->operand_kinds[k];
			const U7Expr *operand = list->operands[i];
			fputs(", ", emitter->out);
			if (kind == U7_CONST_USABLE && is_constant(operand, 0, 0xFF)) {
				if (i + 1U < list->operand_count && is_constant(list->operands[i + 1U], 0, 0)) {
					emit_as(emitter, operand, kind);
					fputs(", 0", emitter->out);
					i += 2U;
					continue;
				}
				reading = false;
			} else if (kind != U7_CONST_USABLE && is_constant(operand, 0x100, 0xFFFF)) {
				reading = false;
			}
			if (!reading) {
				emit_expr(emitter, operand);
				i += 1U;
				break;
			}
			emit_as(emitter, operand, kind);
			i += 1U;
		}
	}
	fputs(" >>", emitter->out);
}

/* A value whose kind is known, by name where it has one. */
static void emit_as(Emitter *emitter, const U7Expr *expr, unsigned kind) {
	const char *name = constant_named(emitter, kind, expr);
	if (name != NULL) {
		fputs(name, emitter->out);
	} else if (kind == U7_CONST_SCRIPT && expr != NULL && expr->kind == U7_EXPR_ARRAY &&
			emitter->style == U7_EMIT_ANNOTATED && emitter->names->script_actions != NULL) {
		emit_script(emitter, expr);
	} else {
		emit_expr(emitter, expr);
	}
}

/* The kind an argument of a call takes, counted as the engine counts it. */
static unsigned argument_kind(const Emitter *emitter, const U7Expr *call, size_t i) {
	const U7Names *names = emitter->names;
	size_t from_last = call->operand_count - 1U - i;
	if (names == NULL || call->value < 0) {
		return U7_CONST_NONE;
	}
	if (call->kind == U7_EXPR_CALL) {
		return names->parameter_kinds != NULL && call->value <= 0xFFFF && i < 16U
				? names->parameter_kinds[call->value][i] : U7_CONST_NONE;
	}
	if (names->argument_kinds == NULL || call->value > 0xFF || from_last >= 8U) {
		return U7_CONST_NONE;
	}
	return names->argument_kinds[call->value][from_last];
}

static void emit_flag(Emitter *emitter, int number) {
	if (emitter->style == U7_EMIT_ANNOTATED && emitter->names != NULL &&
			emitter->names->flags != NULL && number >= 0 && number < 65536 &&
			emitter->names->flags[number] != NULL) {
		fprintf(emitter->out, "$%s", emitter->names->flags[number]);
	} else {
		fprintf(emitter->out, "flag(%d)", number);
	}
}

static void emit_string(Emitter *emitter, const char *text, size_t length) {
	fputc('"', emitter->out);
	for (size_t i = 0; i < length; ++i) {
		unsigned char c = (unsigned char) text[i];
		if (c == '"' || c == '\\') {
			fputc('\\', emitter->out);
			fputc(c, emitter->out);
		} else if (c == '\n') {
			fputs("\\n", emitter->out);
		} else if (c < 0x20U || c >= 0x7FU) {
			fprintf(emitter->out, "\\x%02X", c);
		} else {
			fputc((int) c, emitter->out);
		}
	}
	fputc('"', emitter->out);
}

/* Spellings follow the original language document where it gives one. */
static const char *binary_spelling(uint8_t opcode) {
	switch (opcode) {
		case 0x09U: return "+";
		case 0x0AU: return "-";
		case 0x0BU: return "/";
		case 0x0CU: return "*";
		case 0x0DU: return "modulo";
		case 0x0EU: return "and";
		case 0x0FU: return "or";
		case 0x16U: return ">";
		case 0x17U: return "<";
		case 0x18U: return ">=";
		case 0x19U: return "<=";
		case 0x1AU: return "<>";
		case 0x22U: return "=";
		case 0x30U: return "in";
		default: return "?";
	}
}

/*
 * A data segment may hold the same text at two offsets, and both are used, so
 * the form the compiler reads names which of them this is.
 */
static void emit_string_slot(Emitter *emitter, const U7Expr *expr) {
	if (emitter->style != U7_EMIT_LINEAR || emitter->function == NULL) {
		return;
	}
	unsigned matches = 0;
	unsigned rank = 0;
	size_t at = 0;
	while (at < emitter->function->data_size) {
		const char *text = NULL;
		size_t length = 0;
		if (u7_function_string(emitter->function, at, &text, &length) != U7_OK) {
			break;
		}
		if (length == expr->text_length &&
				memcmp(text, expr->text, length) == 0) {
			if (at < (size_t) expr->detail) {
				rank += 1U;
			}
			matches += 1U;
		}
		at += length + 1U;
	}
	if (matches > 1U) {
		fprintf(emitter->out, "#%u", rank);
	}
}

static void emit_expr(Emitter *emitter, const U7Expr *expr) {
	if (expr == NULL) {
		fputs("#missing", emitter->out);
		return;
	}
	switch (expr->kind) {
		case U7_EXPR_INCOMING:
			fprintf(emitter->out, "#incoming(%d)", expr->value);
			break;
		case U7_EXPR_CONSTANT:
			/* The two push widths and the two boolean pushes are different
			 * bytes, so the form the compiler reads keeps them apart. */
			if (expr->opcode == 0x13U) {
				fputs("true", emitter->out);
			} else if (expr->opcode == 0x14U) {
				fputs("false", emitter->out);
			} else if (expr->opcode == 0x44U) {
				fprintf(emitter->out, "byte(%d)", expr->value);
			} else {
				fprintf(emitter->out, "%d", expr->value);
			}
			break;
		case U7_EXPR_STRING:
			emit_string(emitter, expr->text, expr->text_length);
			emit_string_slot(emitter, expr);
			break;
		case U7_EXPR_LOCAL:
			emit_local(emitter, expr->value);
			break;
		case U7_EXPR_FLAG:
			emit_flag(emitter, expr->value);
			break;
		case U7_EXPR_EVENT:
			fputs("event", emitter->out);
			break;
		case U7_EXPR_ITEM:
			fputs("item", emitter->out);
			break;
		case U7_EXPR_ELEMENT:
			emit_local(emitter, expr->value);
			fputs("::", emitter->out);
			emit_expr(emitter, expr->operands[0]);
			break;
		case U7_EXPR_UNARY:
			fputs("not ", emitter->out);
			emit_expr(emitter, expr->operands[0]);
			break;
		case U7_EXPR_BINARY: {
			/* A constant compared with a value of known kind takes that kind's name. */
			bool compare = expr->opcode == 0x22U || expr->opcode == 0x1AU;
			fputc('(', emitter->out);
			emit_as(emitter, expr->operands[0],
					compare ? kind_of(emitter, expr->operands[1]) : U7_CONST_NONE);
			fprintf(emitter->out, " %s ", binary_spelling(expr->opcode));
			emit_as(emitter, expr->operands[1],
					compare ? kind_of(emitter, expr->operands[0]) : U7_CONST_NONE);
			fputc(')', emitter->out);
			break;
		}
		case U7_EXPR_JOIN:
			fputc('(', emitter->out);
			emit_expr(emitter, expr->operands[0]);
			fputs(" & ", emitter->out);
			emit_expr(emitter, expr->operands[1]);
			fputc(')', emitter->out);
			break;
		case U7_EXPR_ARRAY:
			fputs("<<", emitter->out);
			for (size_t i = 0; i < expr->operand_count; ++i) {
				fputs(i > 0 ? ", " : " ", emitter->out);
				emit_expr(emitter, expr->operands[i]);
			}
			fputs(" >>", emitter->out);
			break;
		case U7_EXPR_CALL:
		case U7_EXPR_INTRINSIC:
			if (expr->kind == U7_EXPR_CALL) {
				emit_function_name(emitter, (unsigned) expr->value);
				if (emitter->style == U7_EMIT_LINEAR && expr->opcode == 0x47U) {
					fputc('!', emitter->out);   /* Called without the table. */
				}
				/* Name the slot when the declaration lists this target twice. */
				if (expr->opcode == 0x24U && emitter->function != NULL) {
					unsigned matches = 0;
					for (uint16_t i = 0; i < emitter->function->link_count; ++i) {
						uint16_t target = 0;
						(void) u7_function_link(emitter->function, i, &target);
						if (target == (uint16_t) expr->value) {
							matches += 1U;
						}
					}
					if (matches > 1U) {
						fprintf(emitter->out, "#%d", expr->detail);
					}
				}
			} else {
				emit_intrinsic_name(emitter, expr->value, expr->opcode == 0x38U);
			}
			fputc('(', emitter->out);
			for (size_t i = 0; i < expr->operand_count; ++i) {
				if (i > 0) {
					fputs(", ", emitter->out);
				}
				unsigned kind = argument_kind(emitter, expr, i);
				const char *special = expr->kind == U7_EXPR_INTRINSIC &&
						constant_named(emitter, kind, expr->operands[i]) == NULL
						? constant_named(emitter, U7_CONST_SPECIAL, expr->operands[i]) : NULL;
				if (emit_object(emitter, expr, i)) {
				} else if (special != NULL) {
					fputs(special, emitter->out);
				} else {
					emit_as(emitter, expr->operands[i], kind);
				}
			}
			fputc(')', emitter->out);
			break;
	}
}

static unsigned line_at(const U7Names *names, size_t offset) {
	unsigned line = 0;
	if (names == NULL) {
		return 0;
	}
	for (size_t i = 0; i < names->line_count; ++i) {
		if (names->lines[i].code_offset <= offset) {
			line = names->lines[i].line;
		}
	}
	return line;
}

/* Inside brackets these carry meaning, so they are escaped as the language
 * document describes. */
static void emit_print_text(Emitter *emitter, const char *text, size_t length) {
	for (size_t i = 0; i < length; ++i) {
		unsigned char c = (unsigned char) text[i];
		if (c == '\\' || c == '@' || c == '<' || c == '>' || c == '[' || c == ']') {
			fputc('\\', emitter->out);
			fputc((int) c, emitter->out);
		} else if (c == '\n') {
			fputs("\\n", emitter->out);
		} else if (c < 0x20U || c >= 0x7FU) {
			fprintf(emitter->out, "\\x%02X", c);
		} else {
			fputc((int) c, emitter->out);
		}
	}
}

/*
 * A run of message appends ending in say is the bracketed print construct:
 * literal text, with a variable named between angle brackets.
 */
static bool emit_print(Emitter *emitter, const U7Stmt *statement, int depth,
		const U7Stmt **out_next) {
	/* Text runs together in the print, so an empty or adjacent text part could
	 * not be told apart there; such a run stays as its statements. */
	const U7Stmt *scan = statement;
	bool text_before = false;
	while (scan != NULL && scan->kind == U7_STMT_APPEND) {
		bool text = scan->value != NULL && scan->value->kind == U7_EXPR_STRING;
		if (text && (text_before || scan->value->text_length == 0)) {
			return false;
		}
		text_before = text;
		scan = scan->next;
	}
	if (scan == NULL || scan->kind != U7_STMT_SAY) {
		return false;
	}

	emit_indent(emitter, depth);
	if (emitter->style == U7_EMIT_ANNOTATED) {
		unsigned line = line_at(emitter->names, statement->offset);
		if (line != 0) {
			fprintf(emitter->out, "/* %u */ ", line);
		}
	}
	fputc('[', emitter->out);
	for (const U7Stmt *part = statement; part != scan; part = part->next) {
		const U7Expr *value = part->value;
		if (value != NULL && value->kind == U7_EXPR_STRING) {
			emit_print_text(emitter, value->text, value->text_length);
		} else if (value != NULL && value->kind == U7_EXPR_LOCAL) {
			fputc('<', emitter->out);
			emit_local(emitter, value->value);
			fputc('>', emitter->out);
		}
	}
	fputs("];\n", emitter->out);
	/* The print can end its block, so what follows may be nothing at all;
	 * success is reported separately from where to carry on. */
	*out_next = scan->next;
	return true;
}

/* A branch names a code offset; the linear form names the block it lands in. */
static size_t branch_block(const Emitter *emitter, size_t offset) {
	if (emitter->names != NULL && emitter->names->cfg != NULL) {
		return (size_t) emitter->names->cfg->block_at_offset[offset];
	}
	return offset;
}

/* Writes the branch a structured listing would have carried implicitly. */
static void emit_branch_statement(Emitter *emitter, const U7Stmt *statement,
		int depth) {
	size_t target = branch_block(emitter, statement->branch_target);
	emit_indent(emitter, depth);
	switch (statement->kind) {
		case U7_STMT_BRANCH:
			fprintf(emitter->out, "goto label_%zu;\n", target);
			return;
		case U7_STMT_BRANCH_IF_FALSE:
			fputs("if not ", emitter->out);
			emit_expr(emitter, statement->value);
			fprintf(emitter->out, " goto label_%zu;\n", target);
			return;
		case U7_STMT_ASK:
			fprintf(emitter->out, "ask goto label_%zu;\n", target);
			return;
		case U7_STMT_COMPARE_ANSWERS:
			fputs("answer (", emitter->out);
			if (statement->value != NULL) {
				for (size_t i = 0; i < statement->value->operand_count; ++i) {
					if (i > 0) {
						fputs(", ", emitter->out);
					}
					emit_expr(emitter, statement->value->operands[i]);
				}
			}
			fprintf(emitter->out, ") goto label_%zu;\n", target);
			return;
		case U7_STMT_DEFAULT_ANSWER:
			fprintf(emitter->out, "default (%d) goto label_%zu;\n",
					statement->operands[0], target);
			return;
		case U7_STMT_LOOP:
			fprintf(emitter->out, "iterate (v%d, v%d, v%d, v%d) goto label_%zu;\n",
					statement->operands[0], statement->operands[1],
					statement->operands[2], statement->operands[3],
					target);
			return;
		case U7_STMT_LOOP_START:
			fputs("iterate_setup;\n", emitter->out);
			return;
		default:
			return;
	}
}

static void emit_statements(Emitter *emitter, const U7Stmt *statement, int depth) {
	while (statement != NULL) {
		/* A reading listing shows lines in the margin, not as statements. */
		if (statement->kind == U7_STMT_DEBUG &&
				emitter->style == U7_EMIT_ANNOTATED) {
			statement = statement->next;
			continue;
		}

		if (statement == emitter->epilogue) {
			statement = statement->next;
			continue;
		}

		/* Branches are carried by the structure that encloses them, except
		 * in the linear form, where they are written out. */
		if (emitter->style == U7_EMIT_LINEAR) {
			switch (statement->kind) {
				case U7_STMT_BRANCH:
				case U7_STMT_BRANCH_IF_FALSE:
				case U7_STMT_COMPARE_ANSWERS:
				case U7_STMT_DEFAULT_ANSWER:
				case U7_STMT_ASK:
				case U7_STMT_LOOP:
				case U7_STMT_LOOP_START:
					emit_branch_statement(emitter, statement, depth);
					statement = statement->next;
					continue;
				default:
					break;
			}
		}

		switch (statement->kind) {
			case U7_STMT_BRANCH:
			case U7_STMT_BRANCH_IF_FALSE:
			case U7_STMT_COMPARE_ANSWERS:
			case U7_STMT_DEFAULT_ANSWER:
			case U7_STMT_ASK:
			case U7_STMT_LOOP:
			case U7_STMT_LOOP_START:
				statement = statement->next;
				continue;
			default:
				break;
		}

		if (statement->kind == U7_STMT_APPEND && emitter->style != U7_EMIT_LINEAR) {
			const U7Stmt *after = NULL;
			if (emit_print(emitter, statement, depth, &after)) {
				statement = after;
				continue;
			}
		}

		emit_indent(emitter, depth);
		if (emitter->style == U7_EMIT_ANNOTATED) {
			unsigned line = line_at(emitter->names, statement->offset);
			if (line != 0) {
				fprintf(emitter->out, "/* %u */ ", line);
			}
		}

		switch (statement->kind) {
			case U7_STMT_ASSIGN_LOCAL:
				emit_local(emitter, statement->slot);
				fputs(" = ", emitter->out);
				emit_expr(emitter, statement->value);
				fputs(";", emitter->out);
				break;
			case U7_STMT_ASSIGN_FLAG:
				emit_flag(emitter, statement->slot);
				fputs(" = ", emitter->out);
				emit_expr(emitter, statement->value);
				fputs(";", emitter->out);
				break;
			case U7_STMT_ASSIGN_ELEMENT:
				emit_local(emitter, statement->slot);
				fputs("::", emitter->out);
				emit_expr(emitter, statement->subscript);
				fputs(" = ", emitter->out);
				emit_expr(emitter, statement->value);
				fputs(";", emitter->out);
				break;
			case U7_STMT_ASSIGN_EVENT:
				fputs("event = ", emitter->out);
				emit_expr(emitter, statement->value);
				fputs(";", emitter->out);
				break;
			case U7_STMT_CALL:
				if (statement->opcode == 0x40U) {
					fputs("clear_answers;", emitter->out);
				} else {
					emit_expr(emitter, statement->value);
					fputs(";", emitter->out);
				}
				break;
			case U7_STMT_APPEND:
				fputs("append ", emitter->out);
				emit_expr(emitter, statement->value);
				fputs(";", emitter->out);
				break;
			case U7_STMT_SAY:
				fputs("say;", emitter->out);
				break;
			case U7_STMT_RETURN:
				if (statement->value != NULL) {
					fputs("return ", emitter->out);
					emit_expr(emitter, statement->value);
				} else if (emitter->style != U7_EMIT_LINEAR) {
					fputs("return", emitter->out);
				} else if (statement->opcode == 0x2CU) {
					fputs("return_alt", emitter->out);
				} else if (statement->opcode == 0x32U) {
					fputs("return_zero", emitter->out);
				} else {
					fputs("return", emitter->out);
				}
				fputs(";", emitter->out);
				break;
			case U7_STMT_ABORT:
				fputs("terminate;", emitter->out);   /* Origin's own keyword. */
				break;
			case U7_STMT_DISCARD:
				fputs("#discard ", emitter->out);
				emit_expr(emitter, statement->value);
				fputs(";", emitter->out);
				break;
			case U7_STMT_DEBUG:
				if (statement->opcode == 0x4CU) {
					fprintf(emitter->out, "#line(%d);", statement->operands[0]);
				} else {
					fprintf(emitter->out, "#names(%d, %d);", statement->operands[0],
							statement->operands[1]);
				}
				break;
			default:
				break;
		}
		fputc('\n', emitter->out);
		statement = statement->next;
	}
}

static void emit_region(Emitter *emitter, const U7Region *region, int depth,
		bool tail);

/*
 * A loop body ends by looping, so the continue that ends it says nothing; the
 * compiler adds it wherever the body can reach its end. One ending an arm is a
 * jump of its own and is written.
 */
static void emit_loop_body(Emitter *emitter, const U7Region *body, int depth) {
	emit_region(emitter, body, depth, true);
}

/* True when emitting this region would print nothing at all. */
static bool region_is_empty(const Emitter *emitter, const U7Region *region, bool tail) {
	if (region == NULL) {
		return true;
	}
	switch (region->kind) {
		case U7_REGION_SEQUENCE:
			for (size_t i = 0; i < region->child_count; ++i) {
				bool last = i + 1U == region->child_count;
				if (!region_is_empty(emitter, region->children[i], tail && last)) {
					return false;
				}
			}
			return true;
		case U7_REGION_CONTINUE:
			return tail;
		case U7_REGION_LABEL:
			return true;
		case U7_REGION_BLOCK:
			for (size_t i = 0; i < emitter->target_count; ++i) {
				if (emitter->targets[i] == region->block) {
					return false;   /* It carries a label. */
				}
			}
			for (const U7Stmt *st = emitter->lifted->blocks[region->block].first;
					st != NULL; st = st->next) {
				if (st == emitter->epilogue) {
					continue;
				}
				switch (st->kind) {
					case U7_STMT_BRANCH: case U7_STMT_BRANCH_IF_FALSE:
					case U7_STMT_COMPARE_ANSWERS: case U7_STMT_DEFAULT_ANSWER:
					case U7_STMT_ASK: case U7_STMT_LOOP: case U7_STMT_LOOP_START:
						continue;
					case U7_STMT_DEBUG:
						if (emitter->style == U7_EMIT_ANNOTATED) {
							continue;
						}
						return false;
					default:
						return false;
				}
			}
			return true;
		default:
			return false;
	}
}

static bool is_target(const Emitter *emitter, size_t block) {
	for (size_t i = 0; i < emitter->target_count; ++i) {
		if (emitter->targets[i] == block) {
			return true;
		}
	}
	return false;
}

/* One expression written twice: a ladder repeats its selector in every test. */
static bool same_selector(const U7Expr *a, const U7Expr *b) {
	if (a == NULL || b == NULL) {
		return a == b;
	}
	if (a->kind != b->kind || a->value != b->value || a->opcode != b->opcode ||
			a->operand_count != b->operand_count || a->text_length != b->text_length ||
			(a->text_length > 0 && memcmp(a->text, b->text, a->text_length) != 0)) {
		return false;
	}
	for (size_t i = 0; i < a->operand_count; ++i) {
		if (!same_selector(a->operands[i], b->operands[i])) {
			return false;
		}
	}
	return true;
}

/* The values a test compares one selector with: (s = a) or (s = b). */
static bool case_values(const U7Expr *test, const U7Expr **selector, const U7Expr **values,
		size_t *count, size_t capacity) {
	if (test == NULL || test->kind != U7_EXPR_BINARY) {
		return false;
	}
	if (test->opcode == 0x0FU) {
		return case_values(test->operands[0], selector, values, count, capacity) &&
				case_values(test->operands[1], selector, values, count, capacity);
	}
	const U7Expr *left = test->operands[0];
	const U7Expr *right = test->operands[1];
	if (test->opcode != 0x22U || left == NULL || right == NULL ||
			(right->kind != U7_EXPR_CONSTANT && right->kind != U7_EXPR_STRING) ||
			*count >= capacity) {
		return false;
	}
	if (*selector == NULL) {
		*selector = left;
	} else if (!same_selector(*selector, left)) {
		return false;
	}
	values[(*count)++] = right;
	return true;
}

static void emit_region(Emitter *emitter, const U7Region *region, int depth, bool tail);
static bool region_is_empty(const Emitter *emitter, const U7Region *region, bool tail);

/* The conditional an else arm holds, when all else in it prints nothing. */
static const U7Region *sole_conditional(const Emitter *emitter, const U7Region *region) {
	while (region != NULL && region->kind == U7_REGION_SEQUENCE) {
		const U7Region *only = NULL;
		for (size_t i = 0; i < region->child_count; ++i) {
			if (!region_is_empty(emitter, region->children[i], false)) {
				if (only != NULL) {
					return NULL;
				}
				only = region->children[i];
			}
		}
		region = only;
	}
	return region != NULL && (region->kind == U7_REGION_IF ||
			region->kind == U7_REGION_IF_ELSE) ? region : NULL;
}

/*
 * A ladder of four or more tests of one value against constants is written as
 * Ultima 8 writes it: switch s { case 1, 2 { ... } default { ... } }. Its switch
 * tests the selector afresh for each case, just as the ladder does, and the
 * bytecode is the same either way, so this is a way of reading it.
 */
static bool emit_switch(Emitter *emitter, const U7Region *region, int depth) {
	enum { MIN_ARMS = 4, MAX_VALUES = 1024 };
	const U7Expr *selector = NULL;
	const U7Expr **values = malloc(MAX_VALUES * sizeof *values);
	size_t *ends = malloc(MAX_VALUES * sizeof *ends);
	const U7Region **bodies = malloc(MAX_VALUES * sizeof *bodies);
	size_t value_count = 0;
	size_t arm_count = 0;
	const U7Region *otherwise = NULL;
	bool emitted = false;
	if (values == NULL || ends == NULL || bodies == NULL) {
		goto done;
	}
	for (const U7Region *r = region; r != NULL;) {
		size_t before = value_count;
		if (r->negated || r->tests > 1U || (r != region && is_target(emitter, r->block)) ||
				!case_values(r->condition, &selector, values, &value_count, MAX_VALUES)) {
			value_count = before;
			break;
		}
		bodies[arm_count] = r->child_count > 0 ? r->children[0] : NULL;
		ends[arm_count++] = value_count;
		otherwise = r->child_count > 1 ? r->children[1] : NULL;
		const U7Region *next = sole_conditional(emitter, otherwise);
		if (next == NULL) {
			break;
		}
		r = next;
		/* A further test of something else is the default, as it stands. */
		const U7Expr *probe = NULL;
		size_t probe_count = 0;
		const U7Expr *scratch[16];
		if (r->negated || r->tests > 1U || is_target(emitter, r->block) ||
				!case_values(r->condition, &probe, scratch, &probe_count, 16) ||
				!same_selector(probe, selector)) {
			break;
		}
	}
	if (arm_count < MIN_ARMS) {
		goto done;
	}
	emit_indent(emitter, depth);
	fputs("switch ", emitter->out);
	emit_expr(emitter, selector);
	fputs(" {\n", emitter->out);
	for (size_t arm = 0, v = 0; arm < arm_count; ++arm) {
		emit_indent(emitter, depth + 1);
		fputs("case ", emitter->out);
		for (size_t first = v; v < ends[arm]; ++v) {
			fputs(v > first ? ", " : "", emitter->out);
			emit_as(emitter, values[v], kind_of(emitter, selector));
		}
		fputs(" {\n", emitter->out);
		emit_region(emitter, bodies[arm], depth + 2, false);
		emit_indent(emitter, depth + 1);
		fputs("}\n", emitter->out);
	}
	/* An empty default is still the jump past the last case, so it is written. */
	if (otherwise != NULL) {
		emit_indent(emitter, depth + 1);
		fputs("default {\n", emitter->out);
		emit_region(emitter, otherwise, depth + 2, false);
		emit_indent(emitter, depth + 1);
		fputs("}\n", emitter->out);
	}
	emit_indent(emitter, depth);
	fputs("}\n", emitter->out);
	emitted = true;
done:
	free(values);
	free(ends);
	free(bodies);
	return emitted;
}

/* Only more answer arms: what Origin writes as keys one after another. */
static bool only_keys(const Emitter *emitter, const U7Region *region) {
	if (region == NULL) {
		return false;
	}
	if (region->kind == U7_REGION_CASE) {
		return true;
	}
	if (region->kind != U7_REGION_SEQUENCE) {
		return false;
	}
	bool any = false;
	for (size_t i = 0; i < region->child_count; ++i) {
		const U7Region *child = region->children[i];
		/* A key's test block prints at most its label, which reads the same flat. */
		bool test_only = child->kind == U7_REGION_BLOCK;
		for (const U7Stmt *st = test_only ? emitter->lifted->blocks[child->block].first : NULL;
				st != NULL; st = st->next) {
			test_only = test_only && (st->kind == U7_STMT_BRANCH ||
					st->kind == U7_STMT_BRANCH_IF_FALSE || st->kind == U7_STMT_COMPARE_ANSWERS ||
					st->kind == U7_STMT_DEBUG);
		}
		if (test_only || region_is_empty(emitter, child, false)) {
			continue;
		}
		if (!only_keys(emitter, region->children[i])) {
			return false;
		}
		any = true;
	}
	return any;
}

static void emit_region(Emitter *emitter, const U7Region *region, int depth,
		bool tail) {
	if (region == NULL) {
		return;
	}
	/* A jump may land on a block that heads a construct, not just on a plain
	 * one, so every region that owns a block can carry the label. */
	/* Several regions can own one block — a test block and the conditional
	 * after it, a loop and the body that starts at its header — so a label is
	 * printed once, at the first of them. */
	if (region->kind != U7_REGION_GOTO && region->kind != U7_REGION_LABEL &&
			region->kind != U7_REGION_SEQUENCE) {
		for (size_t i = 0; i < emitter->target_count; ++i) {
			if (emitter->targets[i] == region->block && !emitter->labelled[i]) {
				emitter->labelled[i] = true;
				emit_indent(emitter, depth);
				fprintf(emitter->out, "label_%zu:\n", region->block);
				break;
			}
		}
	}

	switch (region->kind) {
		case U7_REGION_BLOCK:
			emit_statements(emitter, emitter->lifted->blocks[region->block].first, depth);
			return;
		case U7_REGION_SEQUENCE:
			for (size_t i = 0; i < region->child_count; ++i) {
				bool last = i + 1U == region->child_count;
				emit_region(emitter, region->children[i], depth, tail && last);
			}
			return;
		case U7_REGION_GOTO:
			emit_indent(emitter, depth);
			fprintf(emitter->out, "goto label_%zu;\n", region->target);
			return;
		case U7_REGION_LABEL:
			/* Whatever a jump lands on already carries its label. */
			return;
		case U7_REGION_BREAK:
			emit_indent(emitter, depth);
			fputs("break;\n", emitter->out);
			return;
		case U7_REGION_CONTINUE:
			if (tail) {
				return;
			}
			emit_indent(emitter, depth);
			fputs("continue;\n", emitter->out);
			return;
		default:
			break;
	}

	if ((region->kind == U7_REGION_IF || region->kind == U7_REGION_IF_ELSE) &&
			emitter->style != U7_EMIT_LINEAR && emit_switch(emitter, region, depth)) {
		return;
	}

	emit_indent(emitter, depth);
	switch (region->kind) {
		case U7_REGION_IF:
		case U7_REGION_IF_ELSE:
			fputs("if ", emitter->out);
			if (region->negated) {
				fputs("not ", emitter->out);
			}
			emit_expr(emitter, region->condition);
			/* Later tests of a short-circuit and, each alone in its block. */
			for (size_t k = 1; k < region->tests; ++k) {
				fputs(" && ", emitter->out);
				emit_expr(emitter, emitter->lifted->blocks[region->block + k].first->value);
			}
			fputs(" {\n", emitter->out);
			break;
		case U7_REGION_WHILE: {
			/* A loop that does work before its test each time round is written
			 * loop { work } while test { body }, as the compiler lays it out. */
			const U7Stmt *first = emitter->lifted->blocks[region->block].first;
			bool work_first = first != NULL && first->kind != U7_STMT_BRANCH_IF_FALSE;
			/* A loop with no test of its own is left only by break or return;
			 * its header's work is the first thing in its body. */
			if (region->condition == NULL) {
				fputs("loop {\n", emitter->out);
				break;
			}
			if (!work_first) {
				fputs("While ", emitter->out);
				emit_expr(emitter, region->condition);
				fputs(" {\n", emitter->out);
				break;
			}
			fputs("loop {\n", emitter->out);
			emit_statements(emitter, first, depth + 1);
			emit_indent(emitter, depth);
			fputs("} while ", emitter->out);
			emit_expr(emitter, region->condition);
			fputs(" {\n", emitter->out);
			break;
		}
		case U7_REGION_DO_WHILE:
			fputs("do {\n", emitter->out);
			break;
		case U7_REGION_FOREACH: {
			/* The iteration opcode names four slots: a counter and a count the
			 * compiler made up, then the element and the list the source named.
			 * Origin's compiler gave the first two digit names, and they never
			 * appear in the language's own form. */
			const U7Stmt *iterate = NULL;
			for (const U7Stmt *s = emitter->lifted->blocks[region->block].first;
					s != NULL; s = s->next) {
				if (s->kind == U7_STMT_LOOP) {
					iterate = s;
				}
			}
			if (iterate == NULL) {
				fputs("Foreach {\n", emitter->out);
				break;
			}
			fputs("Foreach ", emitter->out);
			emit_local(emitter, iterate->operands[2]);
			fputs(" in ", emitter->out);
			emit_local(emitter, iterate->operands[3]);
			fputs(" {\n", emitter->out);
			break;
		}
		case U7_REGION_CONVERSE:
			fputs("converse {\n", emitter->out);
			break;
		case U7_REGION_CASE:
			if (region->condition == NULL) {
				/* The reply taken when no key matched, with its count. */
				const U7Stmt *test = NULL;
				for (const U7Stmt *st = emitter->lifted->blocks[region->block].first; st != NULL;
						st = st->next) {
					test = st;
				}
				fprintf(emitter->out, "key default (%d) {\n", test != NULL ? test->operands[0] : 0);
				break;
			}
			fputs("key (", emitter->out);
			if (region->condition != NULL) {
				for (size_t i = 0; i < region->condition->operand_count; ++i) {
					if (i > 0) {
						fputs(", ", emitter->out);
					}
					emit_expr(emitter, region->condition->operands[i]);
				}
			}
			fputs(") {\n", emitter->out);
			break;
		default:
			fputs("{\n", emitter->out);
			break;
	}

	if (region->child_count > 0) {
		bool loop = region->kind == U7_REGION_WHILE ||
				region->kind == U7_REGION_DO_WHILE ||
				region->kind == U7_REGION_FOREACH ||
				region->kind == U7_REGION_CONVERSE;
		if (region->kind == U7_REGION_DO_WHILE) {
			/* The body ends with the work its test block does before testing. */
			emit_region(emitter, region->children[0], depth + 1, false);
			emit_statements(emitter, emitter->lifted->blocks[region->target].first,
					depth + 1);
			emit_indent(emitter, depth);
			fputs("} while ", emitter->out);
			emit_expr(emitter, region->condition);
			fputs(";\n", emitter->out);
			return;
		}
		if (loop) {
			emit_loop_body(emitter, region->children[0], depth + 1);
		} else {
			emit_region(emitter, region->children[0], depth + 1, false);
		}
	}
	emit_indent(emitter, depth);
	/* The first key that matches runs, so the keys after one that never falls
	 * through read the same written after it, as Origin writes them. */
	if (region->kind == U7_REGION_CASE && region->child_count > 1 &&
			only_keys(emitter, region->children[1])) {
		fputs("}\n", emitter->out);
		emit_region(emitter, region->children[1], depth, tail);
		return;
	}
	/* An empty else is still the compiler's jump past it, so it is written. */
	if (region->child_count > 1 && (region->kind == U7_REGION_IF_ELSE ||
			!region_is_empty(emitter, region->children[1], tail))) {
		fputs("} else {\n", emitter->out);
		emit_region(emitter, region->children[1], depth + 1, false);
		emit_indent(emitter, depth);
	}
	fputs("}\n", emitter->out);
}

/* Collects every block a goto names, so each can be labelled where it lands. */
static void collect_targets(const U7Region *region, size_t *targets, size_t capacity,
		size_t *count) {
	if (region == NULL) {
		return;
	}
	if (region->kind == U7_REGION_GOTO) {
		for (size_t i = 0; i < *count; ++i) {
			if (targets[i] == region->target) {
				return;
			}
		}
		if (*count < capacity) {
			targets[(*count)++] = region->target;
		}
		return;
	}
	for (size_t i = 0; i < region->child_count; ++i) {
		collect_targets(region->children[i], targets, capacity, count);
	}
}

U7Result u7_emit_function(FILE *out, const U7Function *function,
		const U7Lifted *lifted, const U7Structured *structured,
		const U7Names *names, U7EmitStyle style) {
	if (out == NULL || function == NULL || lifted == NULL || structured == NULL) {
		return U7_ERROR_ARGUMENT;
	}

	Emitter emitter;
	memset(&emitter, 0, sizeof emitter);
	emitter.out = out;
	emitter.function = function;
	emitter.lifted = lifted;
	emitter.names = names;
	emitter.style = style;

	static size_t targets[256];
	static bool labelled[256];
	size_t target_count = 0;
	collect_targets(structured->root, targets, 256U, &target_count);
	memset(labelled, 0, sizeof labelled);
	emitter.targets = targets;
	emitter.target_count = target_count;
	emitter.labelled = labelled;

	if (style == U7_EMIT_LINEAR) {
		/* Kind and parameters first, then every block under its own label. */
		bool value_returned = false;
		for (size_t b = 0; b < lifted->block_count; ++b) {
			for (const U7Stmt *s = lifted->blocks[b].first; s != NULL; s = s->next) {
				if (s->kind == U7_STMT_RETURN && s->value != NULL) {
					value_returned = true;
				}
			}
		}
		fprintf(out, "%s fn_%04X",
				value_returned ? "Function"
						: function->argument_count > 0 ? "Routine" : "Usable",
				function->function_id);
		if (function->argument_count > 0) {
			fputs(" accepts", out);
			for (uint16_t i = 0; i < function->argument_count; ++i) {
				fprintf(out, "%s a%u", i > 0 ? "," : "", i);
			}
		}
		fprintf(out, " #locals(%u)", function->local_count);
		if (function->link_count > 0) {
			fputs(" #uses(", out);
			for (uint16_t i = 0; i < function->link_count; ++i) {
				uint16_t target = 0;
				(void) u7_function_link(function, i, &target);
				fprintf(out, "%sfn_%04X", i > 0 ? ", " : "", target);
			}
			fputc(')', out);
		}
		fputs(" {\n", out);
		for (size_t b = 0; b < lifted->block_count; ++b) {
			fprintf(out, "label_%zu:\n", b);
			emit_statements(&emitter, lifted->blocks[b].first, 1);
		}
		fputs("}\n", out);
		return U7_OK;
	}

	if (style == U7_EMIT_ANNOTATED && names != NULL && names->debug != NULL &&
			names->debug->module != NULL) {
		fprintf(out, "// module %.*s\n", (int) names->debug->module_length,
				names->debug->module);
	}

	/* The compiler closes every function with a return the source leaves out:
	 * one that returns the value set, for a Function, a plain one otherwise. */
	const U7Stmt *closing = NULL;
	if (lifted->block_count > 0) {
		for (const U7Stmt *s = lifted->blocks[lifted->block_count - 1U].first; s != NULL;
				s = s->next) {
			closing = s;
		}
	}
	if (closing != NULL && (closing->kind != U7_STMT_RETURN ||
			(closing->opcode != 0x25U && closing->opcode != 0x32U))) {
		closing = NULL;
	}
	emitter.epilogue = closing;

	const char *kind = closing != NULL && closing->opcode == 0x32U ? "Function"
			: function->argument_count > 0 ? "Routine" : "Usable";
	fprintf(out, "%s ", kind);
	emit_function_name(&emitter, function->function_id);
	if (function_name(&emitter, function->function_id) != NULL && !names->without_ids) {
		fprintf(out, " #id(0x%04X)", function->function_id);
	}
	if (function->argument_count > 0) {
		fputs(" accepts", out);
		for (uint16_t i = 0; i < function->argument_count; ++i) {
			fprintf(out, "%s a%u", i > 0 ? "," : "", i);
		}
	}
	fprintf(out, " #locals(%u)", function->local_count);
	if (function->link_count > 0) {
		/* The table is declared because the code alone cannot recover it. */
		fputs(" #uses(", out);
		for (uint16_t i = 0; i < function->link_count; ++i) {
			uint16_t target = 0;
			(void) u7_function_link(function, i, &target);
			fputs(i > 0 ? ", " : "", out);
			emit_function_name(&emitter, target);
		}
		fputc(')', out);
	}
	fputs(" {\n", out);
	emit_region(&emitter, structured->root, 1, false);
	fputs("}\n", out);
	return U7_OK;
}
