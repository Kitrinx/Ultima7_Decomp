#ifndef SHUTDOWN_H
#define SHUTDOWN_H

/* Something that must be undone on the way out, however the program ends. */
struct ShutdownHook;
extern ShutdownHook far *ShutdownHooks;

struct ShutdownHook {
	ShutdownHook far *next;
	ShutdownHook() { next = ShutdownHooks; ShutdownHooks = this; return; }
	virtual void shutdown();
};

void RunShutdownHooks();
void ExitProgram(int code);
void ExitMessage(char *fmt, ...);

/* Messages for FatalCode and FatalCode2. */
extern char *HaltMessage;
extern char *ErrorFormat;
extern char *SubErrorFormat;

/* Prints the message, undoes everything and exits with 1. */
void FatalMessage(char *fmt, ...);
void FatalCode(int code);
void FatalCode2(int code, int subtype);

void ShowMessage(char *fmt, ...);

#endif
