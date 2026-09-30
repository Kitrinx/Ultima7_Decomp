#ifndef ERRORS_H
#define ERRORS_H

/* Called before a fatal error ends the game. */
typedef void (*FatalHandler)(void);

extern char *WorkString;
extern int WorkstringSize;
void DefaultFatalHook(void);
extern FatalHandler FatalHook;
void SetFatalHook(FatalHandler handler);
FatalHandler SwapFatalHook(FatalHandler handler);
void RunFatalHook(void);
void ReportError(int code);
void ReportErrorSubtype(int code, int subtype);
int LargerOf(int a, int b);
int SmallerOf(int a, int b);
void RaiseToAtLeast(int *p, int v);
void LowerToAtMost(int *p, int v);
int ClampInt(int lo, int v, int hi);

#endif
