#ifndef LOOK_H
#define LOOK_H

struct LookScanner;

void far ProduceItemDisplayName(char *out, unsigned type, unsigned char quantity, int frame);
void far LookAtItem(void);

extern char *DescriptFileName;
void far AppendChar(char *s, char c);
void far AppendTextFromTable(char *s, int n, int count, int *table);
void far AppendTextEntry(char *s, int n, int count, int first);
void far AppendTextChoice(char *s, int a, int b, int same, int other);
char far ScanToDelimiter(LookScanner *sc, char *out, char *delims);
void far FormatName(char *out, char *fmt, ...);

#endif
