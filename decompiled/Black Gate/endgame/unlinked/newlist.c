/* Not in ENDGAME.EXE: it stands in for a member of Origin's library that this program does not link.
 * Which module that was is not known; this one only has to use DoubleList. TLINK reads every
 * library member before it drops the unused ones, so the names this one uses are met first, and
 * DoubleList's virtual table and destructor copy are placed ahead of CacheList's.
 */
/* flags: -O -1 -P -d */

#include "cache.h"

DoubleList *NewList()
{
	return new DoubleList;
}
