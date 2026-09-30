/* Black Gate U7.EXE, overlay segment 325 (file offsets 0x098140 to 0x098196, 86 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "dosio.h"
#include "ucctrl.h"

void CallStack_push(struct CallStack *s, int value)
{
	if (s->count + 1 >= CALL_DEPTH)
		ReportError(0x6100);
	s->items[s->count++] = value;
}

int CallStack_pop(struct CallStack *s)
{
	if (s->count == 0)
		ReportError(0x6101);
	return s->items[--s->count];
}
