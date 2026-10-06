/* Serpent Isle INTRO.EXE, resident segment 46 (file offsets 0x010116 to 0x010ddf, 3273 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include <mem.h>
#include <stdio.h>
#include "cache.h"
#include "dosio.h"
#include "memsys.h"
#include "shutdown.h"

#define MAX_NODES       500
#define MAX_BLOCKS      100
#define MIN_ITEM_SIZE   4
#define EMS_BLOCK_SIZE  65528L

#include "ems.h"

/* Put a new item on the unloaded list, loading it at once if asked. */
CacheNode *Cache::add(CacheItem *item, char loadNow)
{
	CacheList *unloaded = (CacheList *) lists.tail;
	CacheNode *node = unloaded->addLast(item);

	if (loadNow)
		load(node);
	return node;
}

/* Take a loaded node out of its list and give its room back. */
int Cache::remove(CacheNode **node)
{
	DoubleLink *link;

	for (link = (*node)->list->head; link; link = link->next)
		if (*node == link) {
			free += (*node)->size;
			(*node)->reset();
			(*node)->list->unlink(*node);
			*node = 0;
			return 1;
		}
	return 0;
}

/* Unload the least used nodes of a list until size bytes are free in it. */
void Cache::makeRoom(long size, CacheList *list)
{
	int i;
	int count = 0;
	CacheNode *swap;
	DoubleLink *link;
	CacheNode *nodes[MAX_NODES + 1];
	int j;
	unsigned gap;

	if (list->count() > MAX_NODES)
		FatalMessage("Error: 0x0701\n");
	i = 1;
	link = list->head;
	while (link) {
		count++;
		nodes[i] = (CacheNode *) link;
		link = link->next;
		i++;
	}
	for (gap = count / 2; gap > 0; gap = gap >> 1)
		for (i = gap + 1; i <= count; i++)
			for (j = i - gap; j > 0; )
				if (nodes[j]->uses > nodes[j + gap]->uses) {
					swap = nodes[j];
					nodes[j] = nodes[j + gap];
					nodes[j + gap] = swap;
					j = j - gap;
				} else
					j = 0;
	for (i = 1; list->free < size; i++)
		unload(nodes[i]);
}

/* Slide each list's nodes down so its free room is in one piece at the end. */
void Cache::pack()
{
	CacheList *list = (CacheList *) lists.head;
	CacheList *last = (CacheList *) lists.tail;
	CacheNode *node;
	CacheNode *next;

	while (list != last) {
		node = (CacheNode *) list->head;
		if (node == 0)
			return;
		for (next = (CacheNode *) node->next; node; node = next) {
			next = (CacheNode *) node->next;
			if (next == 0)
				break;
			if (PointerToLinear(next->address) < PointerToLinear(node->end()))
				FatalMessage("Cache: BUDDY, YOU HAVE SCREWED THE POOCH!\n");
			if (node->end() != next->address) {
				_fmemcpy(node->end(), next->address, next->size);
				next->address = node->end();
			}
		}
		list = (CacheList *) list->next;
	}
}

/* Find room for size bytes: the list, the address and the node to follow. */
void Cache::findSpace(long size, void far **address, CacheNode **after, CacheList **list)
{
	long gap;
	CacheNode *next;
	CacheList *block = (CacheList *) lists.head;
	CacheList *last = (CacheList *) lists.tail;
	int i;
	CacheList *swap;
	unsigned step;
	CacheList *sorted[MAX_BLOCKS + 1];
	CacheNode *node;
	int j;

	if (type == FAR_MEMORY && free < size)
		makeRoom(size, *list);
	for (; block != last; block = (CacheList *) block->next) {
		if (type == EMS_MEMORY)
			memory = Memory.lock(block->base, EMS_MEMORY);
		node = (CacheNode *) block->head;
		if (node == 0) {
			*address = memory;
			*after = 0;
			*list = block;
			return;
		}
		gap = PointerToLinear(node->address) - PointerToLinear(memory);
		if (gap >= size) {
			*address = memory;
			*after = 0;
			*list = block;
			return;
		}
		while (node) {
			next = (CacheNode *) node->next;
			if (next == 0) {
				gap = PointerToLinear(listEnd(*list)) - PointerToLinear(node->end());
				if (gap >= size) {
					*address = node->end();
					*after = node;
					*list = block;
					return;
				}
				break;
			}
			gap = PointerToLinear(next->address) - PointerToLinear(node->end());
			if (gap < 0)
				FatalMessage("Cache: AGAIN, YOU HAVE SCREWED THE POOCH, BUDDY!\n");
			if (gap >= size) {
				*address = node->end();
				*after = node;
				*list = block;
				return;
			}
			node = next;
		}
	}

	/* Nothing free: take the least used EMS block, or the far list, and pack it. */
	if (type == EMS_MEMORY) {
		block = (CacheList *) lists.head;
		if (blocks > MAX_BLOCKS)
			FatalMessage("Error: 0x0702\n");
		for (i = 1; i <= blocks; i++) {
			sorted[i] = block;
			block = (CacheList *) block->next;
		}
		for (step = blocks / 2; step > 0; step = step >> 1)
			for (i = step + 1; i <= blocks; i++)
				for (j = i - step; j > 0; )
					if (sorted[j]->used > sorted[j + step]->used) {
						swap = sorted[j];
						sorted[j] = sorted[j + step];
						sorted[j + step] = swap;
						j = j - step;
					} else
						j = 0;
		*list = sorted[1];
		memory = Memory.lock(sorted[1]->base, EMS_MEMORY);
		if ((*list)->free < size)
			makeRoom(size, *list);
	}
	node = (CacheNode *) (*list)->head;
	if (node == 0) {
		*address = memory;
		*after = 0;
		*list = block;
		return;
	}
	gap = PointerToLinear(node->address) - PointerToLinear(memory);
	if (gap > 0) {
		_fmemcpy(memory, node->address, gap);
		node->address = memory;
	}
	while (node) {
		next = (CacheNode *) node->next;
		if (next == 0) {
			gap = PointerToLinear(listEnd(*list)) - PointerToLinear(node->end());
			if (gap >= size) {
				*address = node->end();
				*after = node;
				return;
			}
			FatalMessage("Cache: THAT POOR, POOR POOCHIE!\n");
		}
		gap = PointerToLinear(next->address) - PointerToLinear(node->end());
		if (gap < 0)
			FatalMessage("Cache: BUDDY, THAT POOCH IS NOT GONNA LIKE YOU IN THE MORNING\n");
		if (gap > 0) {
			if (gap >= size) {
				*address = node->end();
				*after = node;
				return;
			}
			_fmemcpy(node->end(), next->address, next->size);
			next->address = node->end();
		}
		node = next;
	}
	FatalMessage("Error: 0x0703\n");
}

/* Read a node's item into the cache. */
void Cache::load(CacheNode *node)
{
	CacheList *list = (CacheList *) lists.head;
	void far *address;
	CacheNode *after;
	long needed;
	CacheList *unloaded;

	needed = node->item->size();
	if (needed < MIN_ITEM_SIZE)
		needed = MIN_ITEM_SIZE;
	if (needed % 2)
		needed++;
	if (size < needed)
		FatalMessage("Error: 0x0704\n");
	findSpace(needed, &address, &after, &list);
	if (type == EMS_MEMORY)
		free = list->free;
	if (address == 0)
		FatalMessage("Cache: Boy, something is REALLY wrong!\n");
	node->item->read(address);
	node->address = address;
	node->list = list;
	free -= node->size;
	list->free -= node->size;
	unloaded = (CacheList *) lists.tail;
	unloaded->unlink(node);
	if (after == 0)
		list->insertAtHead(node);
	else
		list->insertAfter(after, node);
}

/* Give a node's room back and move it to the unloaded list. */
void Cache::unload(CacheNode *node)
{
	CacheList *list = node->list;
	CacheList *unloaded = (CacheList *) lists.tail;

	if (type == EMS_MEMORY)
		list->used -= node->uses;
	free += node->size;
	list->free += node->size;
	node->reset();
	list->unlink(node);
	unloaded->insertAtTail(node);
}

/* The item's bytes, loading them first if they are not in the cache. */
void far *Cache::lock(CacheNode *node)
{
	if (node->address == 0)
		load(node);
	else if (type == EMS_MEMORY)
		Memory.lock(node->list->base, EMS_MEMORY);
	node->addUse();
	return node->address;
}

long Cache::available()
{
	return free;
}

void far *Cache::listEnd(CacheList *list)
{
	long linear = PointerToLinear(memory);

	linear += list->size;
	return LinearToPointer(linear);
}

void Cache::dump()
{
	CacheList *list = (CacheList *) lists.head;
	CacheNode *node;
	int count;
	int n;
	int i;

	printf("Cache size     = %ld\n", size);
	printf("Cache type     = %d ", type);
	if (type == NEAR_MEMORY)
		printf("(Near)\n");
	else if (type == FAR_MEMORY)
		printf("(Far)\n");
	else if (type == EMS_MEMORY)
		printf("(EMS)\n");
	printf("Cache mem free = %ld\n", free);
	printf("Cache mem ptr  = %ld\n", PointerToLinear(memory));
	printf("EMS blocks (if any) = %d\n", blocks);
	for (n = 0; list; list = (CacheList *) list->next, n++) {
		count = (int) list->count();
		node = (CacheNode *) list->head;
		printf("List %d mem free = %ld\n", n, list->free);
		printf("List %d mem ptr  = %x\n", n, list->base);
		printf("List %d block usage = %d\n", n, list->used);
		printf("List node data:\n");
		printf("Element#   Pointer    Size  Used\n");
		for (i = 1; i <= count; i++) {
			printf("%5d %12ld %7ld %5u\n", i, PointerToLinear(node->address), node->size, node->uses);
			node = (CacheNode *) node->next;
		}
		printf("\n");
	}
}

Cache::Cache()
{
	FatalMessage("Error: 0x0705\n");
}

Cache::Cache(long size, unsigned char type)
{
	CacheList *first = new CacheList;
	CacheList *unloaded = new CacheList;
	long count;
	long rest;
	CacheList *list;
	void far *block;
	int i;

	if (type == FAR_MEMORY) {
		this->size = first->size = size;
		free = first->free = size;
		this->type = type;
		blocks = 0;
		memory = first->base = Memory.allocate(size, type, 0, 1, "Not enough memory available for a Far cache\n");
	}
	lists.insertAtHead(first);
	lists.insertAtTail(unloaded);
	if (type == EMS_MEMORY) {
		this->size = EMS_BLOCK_SIZE;
		free = EMS_BLOCK_SIZE;
		this->type = type;
		memory = (void far *) EmsFrame;
		count = size / EMS_BLOCK_SIZE;
		rest = size % EMS_BLOCK_SIZE;
		if (rest)
			blocks = count + 1;
		else
			blocks = count;
		first->base = Memory.allocate(EMS_BLOCK_SIZE, EMS_MEMORY, 0, 1,
			"Not enough memory available for an EMS cache\n");
		first->size = EMS_BLOCK_SIZE;
		first->free = EMS_BLOCK_SIZE;
		for (i = 2; i <= blocks; i++) {
			block = Memory.allocate(EMS_BLOCK_SIZE, EMS_MEMORY, 0, 1,
				"Not enough memory available for an EMS cache\n");
			list = new CacheList(block, EMS_BLOCK_SIZE);
			lists.insertAfter(first, list);
		}
	}
}

/* Free every block but the unloaded list's, which owns none. */
Cache::~Cache()
{
	CacheList *list = (CacheList *) lists.head;
	CacheList *last = (CacheList *) lists.tail;

	while (list != last) {
		Memory.release(&list->base, type);
		list = (CacheList *) list->next;
	}
}
