#ifndef TEXTFILE_H
#define TEXTFILE_H

namespace Shared {

/* A text read whole into the far heap, with a far pointer to each of its lines. */
struct TextFile {
	char * *lines;
	char *text;
	int16_t count;
	TextFile(char *name);
	TextFile(char *flexName, int16_t entry);
	~TextFile();
	void allocate(int32_t size);
	void findLines(int32_t length);
	void load(char *name);
	void load(char *flexName, int16_t entry);
	char *getLine(int16_t n);
	int16_t getCount() { return count; }
	int16_t split(int32_t length, int8_t mark);
};

}

#endif
