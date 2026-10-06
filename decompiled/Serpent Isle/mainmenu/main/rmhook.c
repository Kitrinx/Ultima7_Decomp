/* Serpent Isle MAINMENU.EXE, resident segment 50 (file offsets 0x016124 to 0x016184, 96 bytes).
 * Borland C++ 2.0 -mm -O -1 rebuilds it byte for byte.
 */

#include <stdlib.h>
#include "dosio.h"
#include <dos.h>
#include "init.h"

/* Restores an interrupt vector's previous handler and drops its hook from the list. */
void UnhookInterrupt(int vector)
{
	struct HookRecord *n, *prev;

	n = InterruptHookList;
	prev = 0;
	if (n != 0) {
		while (n->vector != vector) {
			prev = n;
			n = n->next;
			if (n == 0)
				break;
		}
		if (n != 0) {
			n->vector = -1;
			setvect(vector, (InterruptHandler)n->previous);
			if (prev != 0)
				prev->next = n->next;
			else
				InterruptHookList = n->next;
			if (n->allocated != 0) {
				n->allocated = 0;
				free(n);
			}
		}
	}
}
