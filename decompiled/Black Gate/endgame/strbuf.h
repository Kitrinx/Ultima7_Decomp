#ifndef STRBUF_H
#define STRBUF_H

/* A string that owns its buffer. size is the buffer's length, terminator included. */
struct String {
	char *text;
	unsigned size;
	String() { text = 0; size = 0; }
	String(unsigned n) { text = 0; allocate(n); }
	~String() { free(); }
	operator char *() { return text; }
	char *get() { return text; }
	void free();
	char *allocate(unsigned n);
	void concat(char far *s);
	void clear();
	char *assign(char far *s);
	char *append(char far *s);
	void swap(String *other);
	char *format(char *fmt, ...);
	char *chop();
};

/* A String kept inside another object. */
struct Message : String {
};

/* A 256-byte buffer for formatting messages. */
extern String WorkBuffer;

#endif
