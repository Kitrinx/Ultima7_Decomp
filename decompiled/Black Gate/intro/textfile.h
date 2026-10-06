#ifndef TEXTFILE_H
#define TEXTFILE_H

/* A text read whole into the far heap, with a far pointer to each of its lines. */
struct TextFile {
	char far * far *lines;
	char far *text;
	int count;
	TextFile(char *name);
	TextFile(char *flexName, int entry);
	~TextFile();
	void allocate(long size);
	void findLines(long length);
	void load(char *name);
	void load(char *flexName, int entry);
	char far *getLine(int n);
	int split(long length, char mark);
};

#endif
