#include "u7port.h"
#include "vstring.h"

static int8_t IsSeparator(char c, const char *separators)
{
	for (; *separators != 0; separators++)
		if (*separators == c)
			return 1;
	return 0;
}

/* Skips leading separators, returns the token start and ends it with a zero, leaving the
 * cursor after it. With text 0 it continues from the cursor. Returns 0 when nothing is left. */
char *NextToken(char *text, char *separators, char **cursor)
{
	char *start;

	if (text != 0)
		*cursor = text;
	while (**cursor != 0 && IsSeparator(**cursor, separators))
		(*cursor)++;
	if (**cursor == 0)
		return 0;
	start = *cursor;
	while (**cursor != 0) {
		if (IsSeparator(**cursor, separators)) {
			**cursor = 0;
			(*cursor)++;
			break;
		}
		(*cursor)++;
	}
	return start;
}
