/* Serpent Isle SI.EXE, overlay segment 288 (file offsets 0x07cea0 to 0x07d264, 964 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "activity.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "sortitem.h"
#include "random.h"
#include "actitem.h"
#include "script.h"
#include "search.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Read: sit at a book, with the one in front set to the reading frame of its three until done. */
void far DoWorkRead(objref *npc)
{
	AreaSearch book;
	int i = 7;  /* the search tries dir + 7, dir and dir + 1 */
	int x, y;
	int dir;

	x = Item_getX(*npc);
	y = Item_getY(*npc);
	dir = (unsigned char) (NPC(npc)->status & 7);

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_SIT, 642 /* book */, -1, 0);
		break;
	case 1:
		if (IsBlocked(npc))
			Npc_popSchedule(npc, -1);
		else {
			do {
				FindItemInArea(&book, WrapCoord(x + DirDeltaX[dir + (i & 7)]), WrapCoord(y + DirDeltaY[dir + (i & 7)]),
					WrapCoord(x + DirDeltaX[dir + (i & 7)]), WrapCoord(y + DirDeltaY[dir + (i & 7)]),
					0, 642 /* book */, 0xff, 0xff);
				i++;
			} while (!book.found() && i <= 9);
			/* books come in threes of frames: the first is shown while read, the second after */
			if (book.found() && book.current.frame() % 3 != 0)
				Item_setFrame(&book.current, book.current.frame() / 3 * 3);
			PostScheduleScript(npc, MakeScript(SCRIPT_WAIT, GenerateRandomIntegerInRange(16) + 25, SCRIPT_END));
		}
		break;
	case 2:
		PostScheduleScript(npc, MakeScript(SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2,
			SCRIPT_END));
		do {
			FindItemInArea(&book, WrapCoord(x + DirDeltaX[dir + (i & 7)]), WrapCoord(y + DirDeltaY[dir + (i & 7)]),
				WrapCoord(x + DirDeltaX[dir + (i & 7)]), WrapCoord(y + DirDeltaY[dir + (i & 7)]),
				0, 642 /* book */, 0xff, 0xff);
			i++;
		} while (!book.found() && i <= 9);
		if (book.found() && book.current.frame() % 3 == 0)
			Item_setFrame(&book.current, book.current.frame() + 1);
		break;
	case 3:
		Npc_popSchedule(npc, 0);
		break;
	}
}
