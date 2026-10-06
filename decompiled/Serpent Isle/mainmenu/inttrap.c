/* Serpent Isle MAINMENU.EXE, resident segment 41 (file offsets 0x012a92 to 0x012cf1, 607 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include <dos.h>
#include "dosio.h"
#include "u7event.h"
#include "colbuf.h"

char IretStub = 0xcf;   /* an IRET: a handler that ignores its interrupt */

InterruptHook::InterruptHook()
{
	int *p;

	rec.next = 0;
	rec.vector = -1;
	/* clear the saved handler a word at a time */
	p = &rec.vector;
	p++;
	*p = 0;
	p++;
	*p = 0;
}

void interrupt NullInterrupt(void) {}

void far *InterruptHook::install(void far *handler)
{
	HookInterruptVector(vector, (InterruptHandler)handler, &rec, (long far *)&old);
	return old;
}

void InterruptHook::restore()
{
	UnhookInterrupt(vector);
}

InterruptHook::~InterruptHook()
{
	restore();
}

void InterruptHook::disableBreak()
{
	union REGS r;
	install(&IretStub);
	if (vector == 0x23 || vector == 0x1b) {
		r.h.ah = 0x33;
		r.h.al = 1;
		r.h.dl = 0;
		int86(0x21, &r, &r);
	}
}

CtrlBreakTrap::CtrlBreakTrap()
{
	setVector(0x1b);
	disableBreak();
}

BiosHook::BiosHook()
{
	setVector(0x15);
	PrevKeyIntercept = install((void far *)(long) InterceptCtrlC);
}

void NullRoutine(void) {}

CtrlCTrap::CtrlCTrap()
{
	setVector(0x23);
	disableBreak();
}

void interrupt HandleDivideByZero(void)
{
	FatalError("Divide by Zero!");
}

DivideTrap::DivideTrap()
{
	setVector(0);
	install((void far *) HandleDivideByZero);
}

void far SetDosVerify(char enabled)
{
	union REGS r;
	r.h.ah = 0x2e;
	r.h.dl = 0;
	r.h.al = enabled ? 1 : 0;
	int86(0x21, &r, &r);
}
