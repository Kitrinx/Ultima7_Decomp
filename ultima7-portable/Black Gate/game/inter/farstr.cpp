/* Black Gate U7.EXE, overlay segment 326 (file offsets 0x0981a0 to 0x098a40, 2208 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
#include <string.h>
#include <stdlib.h>
#include "dosio.h"
#include "lowlevel.h"
#include "oops.h"
#include "ucvalue.h"
#include "uclist.h"
#include "memapi.h"
#include "farstr.h"

extern void WaitForTextClick();
extern void AddConversationText(char *);
extern int16_t GetTextCharWidth(int8_t);
extern void GetTextBoxSize(int16_t *, int16_t *);
extern void ClearSpeakerText();
extern uint8_t IsOnLeftPage();
extern void AdvanceTextBox();
extern int16_t ConversationMode;
extern int16_t SpeakerLineCount;

/* the text's room, with its terminating zero */
#define FAR_STRING_SIZE     1000

/* the conversation's gumps: faces and text boxes, a scroll, or a book's two pages */
#define CONVERSATION_TALK   1
#define CONVERSATION_SCROLL 2
#define CONVERSATION_BOOK   3

inline int8_t Capitalize(int8_t c)
{
	return c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c;
}

void PauseForClick()
{
	WaitForTextClick();
}

void ShowTextLine(char *text)
{
	AddConversationText(text);
}

int16_t MeasureTextChar(int8_t c)
{
	return GetTextCharWidth(c);
}

void GetTextBoxArea(int16_t *width, int16_t *height)
{
	GetTextBoxSize(width, height);
}

FarString::FarString()
{
	text = (char *)AllocateFarHeap((int32_t)FAR_STRING_SIZE, 0);
	if (text == 0) {
		ReportOutOfFarMemory();
	}
	*text = 0;
}

FarString::~FarString()
{
	FreeFarHeap(text);
	text = 0;
}

void FarString::append(int16_t number)
{
	char buffer[8];

	itoa(number, buffer, 10);
	_fstrncat(text, buffer, FAR_STRING_SIZE - 1 - _fstrlen(buffer));
}

void FarString::append(int8_t c)
{
	char buffer[2];

	buffer[0] = c;
	buffer[1] = 0;
	_fstrncat(text, buffer, FAR_STRING_SIZE - 1);
}

void FarString::append(Value *values)
{
	int16_t count = LinkList_count(values);
	Node value;
	int16_t index;

	for (index = 1; index <= count; index++) {
		value = *GetListNode(values, index);
		switch (value.type) {
		case NODE_CHAR:
			append((int8_t)value.integer());
			break;
		case NODE_INT:
			append(value.integer());
			break;
		case NODE_TEXT:
			append(value.text.str);
			break;
		}
	}
}

void FarString::appendMemory(uint32_t address)
{
	int16_t length = _fstrlen(text);
	int16_t remaining = FAR_STRING_SIZE - 1 - length;

	CopyLinearStringOut(text + length, address, remaining);
}

void FarString::append(char *string)
{
	int16_t remaining = _fstrlen(text);

	remaining = FAR_STRING_SIZE - 1 - remaining;
	_fstrncat(text, string, remaining);
}

/* Scan from start for as much text as fits in width pixels, noting where it could break: '~' ends
 * a line, '*' a page, and '^' capitalizes the next letter. */
void FarString::scan(int16_t width, int16_t start, int16_t *pageBreak, int16_t *lineBreak, int16_t *sentence, int16_t *end, int16_t *space,
	int16_t *punctuation)
{
	int16_t scanned = 0;
	int16_t pixels = 0;
	int16_t index;

	*pageBreak = -1;
	*lineBreak = -1;
	*sentence = -1;
	*end = -1;
	*space = -1;
	*punctuation = -1;
	while (pixels < width) {
		index = start + scanned;
		switch (text[index]) {
		case '\t':
			text[index] = ' ';
			*space = index;
			break;
		case '^':
			text[index + 1] = Capitalize(text[index + 1]);
			index++;
			break;
		case ' ':
			*space = index;
			break;
		case ',':
		case ':':
		case ';':
			*punctuation = index;
			break;
		case '.':
		case '!':
		case '?':
			if (text[index + 1] == '"' || text[index + 1] == '\'') {
				*sentence = index + 1;
			} else {
				*sentence = index;
			}
			break;
		case '~':
			*lineBreak = index;
			return;
		case '*':
			*pageBreak = index;
			return;
		case 0:
			*end = index;
			return;
		}
		pixels += MeasureTextChar(text[index]);
		scanned++;
	}
}

void FarString::printLines(int16_t offset, int16_t *breaks, int16_t count)
{
	int8_t saved;
	int16_t line, end, trimmed, start;

	if (ConversationMode == CONVERSATION_TALK) {
		ClearSpeakerText();
	}
	start = offset;
	for (line = 0; line < count; line++) {
		end = breaks[line];
		if (line != 0) {
			while (start < FAR_STRING_SIZE) {
				if (text[start] != ' ') {
					break;
				}
				start++;
			}
		}
		trimmed = end;
		while (trimmed > start) {
			int8_t c = text[trimmed - 1];

			if (c != ' ' && c != '~') {
				break;
			}
			trimmed--;
		}
		saved = text[trimmed];
		text[trimmed] = 0;
		ShowTextLine(text + start);
		text[trimmed] = saved;
		start = end;
	}
}

void FarString::show()
{
	int16_t pageDone, textDone, cursor, pageStart;
	int16_t pageBreak, lineBreak, sentence, space, end, punctuation;
	int16_t lastLineBreak = -1;
	int16_t lastSentence = -1;
	int16_t lastPunctuation = -1;
	int16_t firstLine, lineBreakLines, sentenceLines, punctuationLines;
	int16_t width, lastLine;
	uint8_t handled = 0;
	uint8_t wait = 0;
	uint8_t clear = 0;
	int16_t length;
	int16_t breaks[60];
	int16_t line;

	GetTextBoxArea(&width, &lastLine);
	lastLine--;
	if (ConversationMode != CONVERSATION_BOOK) {
		line = 0;
	} else {
		line = SpeakerLineCount;
		if (line >= lastLine) {
			line = 0;
			if (!IsOnLeftPage()) {
				PauseForClick();
			}
			AdvanceTextBox();
		}
	}
	firstLine = line;
	textDone = pageDone = 0;
	cursor = pageStart = 0;
	length = _fstrlen(text);
	while (!textDone && cursor <= length) {
		while (!pageDone && !textDone && cursor <= length) {
			scan(width, cursor, &pageBreak, &lineBreak, &sentence, &end, &space, &punctuation);
			if (line - firstLine == 0 && cursor == lineBreak) {
				pageStart++;
				cursor++;
				continue;
			}
			if (end != -1 && end < cursor) {
				textDone = 1;
				break;
			}
			if (end != -1 && line < lastLine) {
				if ((breaks[line - firstLine] = end) != pageStart) {
					printLines(pageStart, breaks, line - firstLine + 1);
					handled = 0;
					pageDone = 1;
				}
				textDone = 1;
			} else if (pageBreak != -1) {
				if ((breaks[line - firstLine] = pageBreak) != pageStart) {
					printLines(pageStart, breaks, line - firstLine + 1);
					handled = 0;
					pageDone = 1;
					if (ConversationMode >= CONVERSATION_SCROLL) {
						clear = 1;
					}
				}
				pageStart = cursor = pageBreak + 1;
				if (pageBreak + 1 == end) {
					textDone = 1;
				}
			} else {
				if (lineBreak != -1) {
					lastLineBreak = lineBreak;
					breaks[line - firstLine] = lineBreak + 1;
					cursor = lineBreak + 1;
					lineBreakLines = line - firstLine + 1;
				} else {
					if (sentence != -1) {
						lastSentence = sentence + 1;
						sentenceLines = line - firstLine + 1;
					}
					if (punctuation != -1) {
						lastPunctuation = punctuation + 1;
						punctuationLines = line - firstLine + 1;
					}
					if (space == -1) {
						space = cursor + width - 1;
					}
					breaks[line - firstLine] = space + 1;
					cursor = space + 1;
				}
				if (line >= lastLine) {
					wait = 1;
					if (ConversationMode == CONVERSATION_BOOK) {
						if (IsOnLeftPage()) {
							wait = 0;
						}
					} else if (ConversationMode == CONVERSATION_SCROLL) {
						wait = 1;
						clear = 1;
					}
					if (lastLineBreak != -1) {
						printLines(pageStart, breaks, lineBreakLines);
						pageStart = cursor = lastLineBreak + 1;
					} else if (lastSentence != -1) {
						breaks[sentenceLines - 1] = lastSentence;
						printLines(pageStart, breaks, sentenceLines);
						while (text[lastSentence] == ' ') {
							lastSentence++;
						}
						pageStart = cursor = lastSentence;
					} else if (lastPunctuation != -1) {
						breaks[punctuationLines - 1] = lastPunctuation;
						printLines(pageStart, breaks, punctuationLines);
						while (text[lastPunctuation] == ' ') {
							lastPunctuation++;
						}
						pageStart = cursor = lastPunctuation;
					} else {
						printLines(pageStart, breaks, line - firstLine);
						pageStart = cursor = breaks[line - firstLine - 1];
					}
					if (ConversationMode == CONVERSATION_BOOK) {
						line = -1;
						firstLine = 0;
						lastLineBreak = lastSentence = lastPunctuation = -1;
						lineBreakLines = sentenceLines = punctuationLines = 0;
						if (wait) {
							PauseForClick();
						}
						AdvanceTextBox();
					}
					handled = 0;
					pageDone = 1;
				}
			}
			line++;
		}
		if (ConversationMode >= CONVERSATION_SCROLL && !wait && !textDone && !clear) {
			handled = 1;
			pageDone = 0;
		}
		if (!textDone && !handled) {
			if (ConversationMode == CONVERSATION_TALK) {
				PauseForClick();
			} else if (clear && !IsOnLeftPage()) {
				PauseForClick();
			}
			if (clear) {
				AdvanceTextBox();
				clear = 0;
			}
			pageDone = 0;
			handled = 1;
			wait = 0;
			line = 0;
			firstLine = 0;
			lastLineBreak = lastSentence = lastPunctuation = -1;
			lineBreakLines = sentenceLines = punctuationLines = 0;
		}
	}
	if (!handled) {
		if (ConversationMode < CONVERSATION_SCROLL || clear) {
			PauseForClick();
			if (clear) {
				AdvanceTextBox();
				clear = 0;
			}
		}
	}
}
