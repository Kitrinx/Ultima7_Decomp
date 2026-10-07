#ifndef VSTRING_H
#define VSTRING_H

extern char EmptyString[];

/* Text on the near heap. Every empty string shares one static byte. */
struct String {
	char *str;
	int16_t len;
	String() { str = 0; len = 0; }
	String(int16_t n);
	~String();
	char *allocate(int16_t n);
	void release(char *p);
	void assign(int16_t n, char *s);
	void assignUntil(char *s, char *stops);
	String &operator+=(char *s);
	String &operator+=(int8_t c);
	String &operator+=(String &s);
	String &operator+(char *s);
	String &operator+(String &s);
	String &operator=(String &s);
	String &operator=(char *s);
	String &format(char *fmt, ...);
	void append(char *s);
	int8_t charAt(int16_t n);
	void clear();
	uint8_t isEmpty() { return *str == 0; }
};

extern String TempString;
uint8_t FindCharInSet(int8_t c, char *s);
int16_t FindCharIndex(int8_t c, char *s);
int16_t MatchPrefix(char *a, char *b);

#ifdef __cplusplus
extern "C" {
#endif
char * NextToken(char *text, char *separators, char **cursor);
#ifdef __cplusplus
}
#endif

#endif
