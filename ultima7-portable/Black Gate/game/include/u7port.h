/* Portable base for the game: fixed-width types, the C library, and Borland helpers.
 * Every game source includes this first. System headers come before the packing pragma
 * so only game structs are packed to one byte, as Borland laid them out.
 */
#ifndef U7PORT_H
#define U7PORT_H

#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Borland string and number helpers with no standard equivalent. */
int16_t u7_stricmp(const char *a, const char *b);
int16_t u7_strnicmp(const char *a, const char *b, size_t n);
char *u7_itoa(int16_t value, char *out, int16_t radix);
char *u7_ltoa(int32_t value, char *out, int16_t radix);
char *u7_strupr(char *s);
int16_t u7_random(int16_t range);

#ifdef __cplusplus
}
#endif

#define stricmp u7_stricmp
#define strcmpi u7_stricmp
#define strnicmp u7_strnicmp
#define itoa(v, s, r) u7_itoa((v), (s), (r))
#define ltoa(v, s, r) u7_ltoa((v), (s), (r))
#define strupr u7_strupr
#define random u7_random

/* Far pointers are ordinary pointers now. */
#define _fmemcpy memcpy
#define _fmemmove memmove
#define _fmemset memset
#define _fmemcmp memcmp
#define _fstrlen strlen
#define _fstrcpy strcpy
#define _fstrncpy strncpy
#define _fstrcat strcat
#define _fstrncat strncat
#define _fstrcmp strcmp
#define _fstrncmp strncmp
#define _fstricmp u7_stricmp
#define _fstrnicmp u7_strnicmp
#define _fstrchr strchr

#ifndef __cplusplus
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#else
template <class T> inline T min(T a, T b) { return a < b ? a : b; }
template <class T> inline T max(T a, T b) { return a > b ? a : b; }
#endif

#pragma pack(1)

#endif
