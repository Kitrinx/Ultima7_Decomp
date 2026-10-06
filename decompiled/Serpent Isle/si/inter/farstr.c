/* Serpent Isle SI.EXE, overlay segment 305 (file offsets 0x084d50 to 0x085613, 2243 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include <string.h>
#include <stdlib.h>
#include "dosio.h"
#include "lowlevel.h"
#include "oops.h"
#include "ucvalue.h"
#include "uclist.h"
#include "memapi.h"
#include "farstr.h"

extern void far WaitForTextClick();
extern void far AddConversationText(char far *);
extern int far GetTextCharWidth(char);
extern void far GetTextBoxSize(int *, int *);
extern void far ClearSpeakerText();
extern unsigned char far IsOnLeftPage();
extern void far AdvanceTextBox();
extern int ConversationMode;
extern int SpeakerLineCount;

/* the text's room, with its terminating zero */
#define FAR_STRING_SIZE     1000

/* the conversation's gumps: faces and text boxes, a scroll, or a book's two pages */
#define CONVERSATION_TALK   1
#define CONVERSATION_SCROLL 2
#define CONVERSATION_BOOK   3
#define CONVERSATION_SCROLL2 5      /* scroll and book in the second font */
#define CONVERSATION_BOOK2  6

inline char Capitalize(char c)
{
	return c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c;
}

void far PauseForClick()
{
	WaitForTextClick();
}

void far ShowTextLine(char far *text)
{
	AddConversationText(text);
}

int far MeasureTextChar(char c)
{
	return GetTextCharWidth(c);
}

void far GetTextBoxArea(int *width, int *height)
{
	GetTextBoxSize(width, height);
}

FarString::FarString()
{
	text = (char far *)AllocateFarHeap((long)FAR_STRING_SIZE, 0);
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

void FarString::append(int number)
{
	char buffer[8];

	itoa(number, buffer, 10);
	_fstrncat(text, buffer, FAR_STRING_SIZE - 1 - _fstrlen(buffer));
}

void FarString::append(char c)
{
	char buffer[2];

	buffer[0] = c;
	buffer[1] = 0;
	_fstrncat(text, buffer, FAR_STRING_SIZE - 1);
}

void FarString::append(Value *values)
{
	int count = LinkList_count(values);
	Node value;
	int index;

	for (index = 1; index <= count; index++) {
		value = *GetListNode(values, index);
		switch (value.type) {
		case NODE_CHAR:
			append((char)value.integer());
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

void FarString::appendMemory(unsigned long address)
{
	int length = _fstrlen(text);
	int remaining = FAR_STRING_SIZE - 1 - length;

	CopyLinearStringN(PointerToLinear(text + length), address, remaining, -1);
}

void FarString::append(char far *string)
{
	int remaining = _fstrlen(text);

	remaining = FAR_STRING_SIZE - 1 - remaining;
	_fstrncat(text, string, remaining);
}

/* Scan from start for as much text as fits in width pixels, noting where it could break: '~' ends
 * a line, '*' a page, and '^' capitalizes the next letter. */
void FarString::scan(int width, int start, int *pageBreak, int *lineBreak, int *sentence, int *end, int *space,
	int *punctuation)
{
	int scanned = 0;
	int pixels = 0;
	int index;

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

void FarString::printLines(int offset, int *breaks, int count)
{
	char saved;
	int line, end, trimmed, start;

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
			char c = text[trimmed - 1];

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
	int pageDone, textDone, cursor, pageStart;
	int pageBreak, lineBreak, sentence, space, end, punctuation;
	int lastLineBreak = -1;
	int lastSentence = -1;
	int lastPunctuation = -1;
	int firstLine, lineBreakLines, sentenceLines, punctuationLines;
	int width, lastLine;
	unsigned char handled = 0;
	unsigned char wait = 0;
	unsigned char clear = 0;
	int length;
	int breaks[60];
	int line;

	GetTextBoxArea(&width, &lastLine);
	lastLine--;
	if (ConversationMode != CONVERSATION_BOOK && ConversationMode != CONVERSATION_BOOK2 &&
		ConversationMode != CONVERSATION_SCROLL && ConversationMode != CONVERSATION_SCROLL2) {
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
					if (ConversationMode == CONVERSATION_BOOK || ConversationMode == CONVERSATION_BOOK2) {
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
					if (ConversationMode == CONVERSATION_BOOK || ConversationMode == CONVERSATION_BOOK2) {
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
