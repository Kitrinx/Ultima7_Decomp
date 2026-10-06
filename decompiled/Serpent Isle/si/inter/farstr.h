#ifndef FARSTR_H
#define FARSTR_H

struct Value;

/* text built up in far memory for the conversation window */
struct FarString {
	char far *text;
	FarString();
	~FarString();
	void append(int);
	void append(Value *);
	void append(char);
	void append(char far *);
	void appendMemory(unsigned long);
	void scan(int width, int start, int *pageBreak, int *lineBreak, int *sentence, int *end, int *space,
		int *punctuation);
	void printLines(int offset, int *breaks, int count);
	void show();
};

void far PauseForClick();
void far ShowTextLine(char far *text);
int far MeasureTextChar(char c);
void far GetTextBoxArea(int *width, int *height);

#endif
