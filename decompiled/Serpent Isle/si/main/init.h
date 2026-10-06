#ifndef INIT_H
#define INIT_H

/* Start-up, shutdown and fatal errors. */

/* One hooked interrupt vector and the handler it replaced. */
struct HookRecord {
	struct HookRecord *next;
	int vector;
	long previous;
	int allocated;
};

typedef void interrupt (far *InterruptHandler)();

extern struct HookRecord *InterruptHookList;

#ifdef __cplusplus
extern "C" {
#endif
long far HookInterruptVector(int vector, InterruptHandler handler, struct HookRecord *hook, long far *previous);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C"
#endif
void far FatalError(char *, ...);

#ifdef __cplusplus
void far AssertFail(char *, int);
void far HaltWithMessage(char *, int, char *, ...);
void far QuitToDos(void);
#endif

#ifdef __cplusplus
extern "C" {
#endif
void far InitEnvironment(void);
void far ShutDown(void);
#ifdef __cplusplus
}
#endif
void far EndGame(void);
void far ExitForEndgame(void);

extern unsigned char PlainErrors;

extern long ViewportFirstRow;
extern int FlatModeFlags;

#endif
