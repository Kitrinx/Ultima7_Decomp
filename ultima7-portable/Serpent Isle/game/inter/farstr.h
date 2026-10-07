#ifndef FARSTR_H
#define FARSTR_H

struct Value;

/* text built up in far memory for the conversation window */
struct FarString {
	char *text;
	FarString();
	~FarString();
	void append(int16_t);
	void append(Value *);
	void append(int8_t);
	void append(char *);
	void appendMemory(uint32_t);
	void scan(int16_t width, int16_t start, int16_t *pageBreak, int16_t *lineBreak, int16_t *sentence, int16_t *end, int16_t *space,
		int16_t *punctuation);
	void printLines(int16_t offset, int16_t *breaks, int16_t count);
	void show();
};

void PauseForClick();
void ShowTextLine(char *text);
int16_t MeasureTextChar(int8_t c);
void GetTextBoxArea(int16_t *width, int16_t *height);

#endif
