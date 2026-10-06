/* Black Gate U7.EXE, resident segment 126 (file offsets 0x03dc4d to 0x03dcdd, 144 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from compiler flags and link order.
 */

#include <alloc.h>
#include <dos.h>
#include "init.h"

/* Point an interrupt vector at handler, recording the old one on the hook list; a null hook gets
 * one allocated. Returns the old handler, also stored in *previous when given. */
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
