/* Serpent Isle SI.EXE, resident segment 125 (file offsets 0x03d6ab to 0x03d73b, 144 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 */

#include <alloc.h>
#include <dos.h>
#include "init.h"

/* Save the replaced vector and link its hook record. */
long far HookInterruptVector(int vector, InterruptHandler handler, struct HookRecord *hook, long far *previous)
{
	long old;
	int allocated;

	allocated = 0;
	if (hook == 0) {
		hook = (struct HookRecord *) malloc(sizeof(struct HookRecord));
		if (hook == 0)
			return 0;
		allocated++;
	}
	old = (long) getvect(vector);
	if (previous != 0)
		*previous = old;
	hook->allocated = allocated;
	hook->vector = vector;
	hook->previous = old;
	hook->next = InterruptHookList;
	InterruptHookList = hook;
	setvect(vector, handler);
	return old;
}
