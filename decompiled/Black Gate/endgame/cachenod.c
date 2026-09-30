/* Black Gate ENDGAME.EXE, resident segment 47 (file offsets 0x00f8f7 to 0x00faab, 436 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include "cache.h"
#include "dosio.h"
#include "shutdown.h"

#define MAX_USES 65000

void *CacheNode::operator new(unsigned)
{
	CacheNode *node = ::new CacheNode;

	if (node == 0)
		FatalMessage("New node not created in CacheNode::new.\n");
	return node;
}

void CacheNode::reset()
{
	address = 0;
	uses = 0;
}

void CacheNode::addUse()
{
	uses++;
	if (list)
		list->addUse();
	if (uses > MAX_USES)
		FatalMessage("Danger Will Robinson!  One of your nodes is about to fall off!\n");
}

/* The address just past this node's bytes. */
void far *CacheNode::end()
{
	long linear = PointerToLinear(address);

	linear = linear + size + 1;
	return LinearToPointer(linear);
}

CacheItem *CacheNode::getItem()
{
	return item;
}

CacheNode::CacheNode()
{
	item = 0;
	address = 0;
	size = 0;
	uses = 0;
	list = 0;
}

CacheNode::CacheNode(CacheItem *item)
{
	this->item = item;
	size = item->size();
	address = 0;
	uses = 0;
	list = 0;
}

CacheNode::CacheNode(CacheItem *item, CacheList *list)
{
	this->item = item;
	size = item->size();
	address = 0;
	uses = 0;
	this->list = list;
}
