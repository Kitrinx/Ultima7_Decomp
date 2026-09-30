/* Black Gate ENDGAME.EXE, resident segment 48 (file offsets 0x00faab to 0x00fc9e, 499 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include "cache.h"

CacheNode *CacheList::addFirst(CacheItem *item)
{
	CacheNode *node = new CacheNode(item);

	insertAtHead(node);
	return node;
}

CacheNode *CacheList::addLast(CacheItem *item)
{
	CacheNode *node = new CacheNode(item);

	insertAtTail(node);
	return node;
}

/* Forget where every node was loaded, then drop them all. */
void CacheList::clear()
{
	DoubleLink *node = 0;

	while (stepForward(&node))
		((CacheNode *) node)->reset();
	removeAll();
}

void CacheList::addUse()
{
	used++;
}

CacheList::CacheList()
{
	base = 0;
	used = 0;
	size = 0;
	free = 0;
}

CacheList::CacheList(void far *base, long size)
{
	this->base = base;
	used = 0;
	this->size = size;
	free = this->size;
}

CacheList::~CacheList()
{
	clear();
}
