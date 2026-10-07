#ifndef LOOK_H
#define LOOK_H

struct LookScanner;
struct objref;

void ProduceItemDisplayName(char *out, objref *item, uint8_t quantity);
void LookAtItem(void);

extern char *DescriptFileName;
void AppendChar(char *s, int8_t c);
void AppendTextFromTable(char *s, int16_t n, int16_t count, int16_t *table);
void AppendTextEntry(char *s, int16_t n, int16_t count, int16_t first);
void AppendTextChoice(char *s, int16_t a, int16_t b, int16_t same, int16_t other);
int8_t ScanToDelimiter(LookScanner *sc, char *out, char *delims);
void FormatName(char *out, char *fmt, ...);

#endif
