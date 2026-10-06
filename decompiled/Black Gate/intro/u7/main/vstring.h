#ifndef VSTRING_H
#define VSTRING_H

/* Text on the near heap. Every empty string shares one static byte. */
struct String {
	char *str;
	int len;
	String() { str = 0; len = 0; }
	String(int n);
	~String();
	char *allocate(int n);
	void release(char *p);
	void assign(int n, char *s);
	void assignUntil(char *s, char *stops);
	String &operator+=(char *s);
	String &operator+=(char c);
	String &operator+=(String &s);
	String &operator+(char *s);
	String &operator+(String &s);
	String &operator=(String &s);
	String &operator=(char far *s);
	String &format(char *fmt, ...);
	void append(char *s);
	char charAt(int n);
	void clear();
	unsigned char isEmpty() { return *str == 0; }
};

extern String TempString;
unsigned char FindCharInSet(char c, char *s);
int FindCharIndex(char c, char *s);
int MatchPrefix(char *a, char *b);

#ifdef __cplusplus
extern "C" {
#endif
char *far NextToken(char *text, char *separators, char **cursor);
#ifdef __cplusplus
}
#endif

#endif
