#ifndef REPORTER_H
#define REPORTER_H

#include "strbuf.h"
#include "shutdown.h"

typedef void (far *ErrorHandler)(char *fmt, ...);

/* Collects an object's error messages and hands each to its handler, FatalMessage unless changed. */
struct ErrorReporter {
	ErrorHandler handler;
	Message message;
	ErrorReporter() { handler = FatalMessage; }
	virtual void describe();
	void setHandler(ErrorHandler h) { handler = h; }
	void error(char *fmt, ...);
	void errorCode(int code);
	void errorCode2(int code, int subtype);
};

#endif
