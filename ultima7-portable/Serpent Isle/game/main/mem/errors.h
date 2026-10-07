#ifndef ERRORS_H
#define ERRORS_H

/* Called before a fatal error ends the game. */
typedef void (*FatalHandler)(void);

extern "C" char *WorkString;
extern int16_t WorkstringSize;
void DefaultFatalHook(void);
extern FatalHandler FatalHook;
void SetFatalHook(FatalHandler handler);
FatalHandler SwapFatalHook(FatalHandler handler);
void RunFatalHook(void);
#ifdef __cplusplus
extern "C"
#endif
void ReportError(int16_t code);
void ReportErrorSubtype(int16_t code, int16_t subtype);
int16_t LargerOf(int16_t a, int16_t b);
int16_t SmallerOf(int16_t a, int16_t b);
void RaiseToAtLeast(int16_t *p, int16_t v);
void LowerToAtMost(int16_t *p, int16_t v);
int16_t ClampInt(int16_t lo, int16_t v, int16_t hi);

#endif
