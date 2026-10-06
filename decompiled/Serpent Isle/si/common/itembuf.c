/* Serpent Isle SI.EXE, resident segment 77 (file offsets 0x030bf4 to 0x030d53, 351 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include <dos.h>
#include "dosio.h"
#include "item.h"
#include "sortitem.h"
#include "oops.h"
#include "itemovr2.h"
#include "loadreg.h"
#include "memapi.h"
#include "mapview.h"

/* the first records are never handed out */
#define RESERVED_ITEMS 7

unsigned ItemBufferBytes = 0;
int ItemBufferCount = 0;
int NpcRecordCount = 0;
int ExtraNpcRecordCount = 0;

void EmptyItemBufferStub(void)
{
}

void AllocItemBuffer(int count, int npcCount, int extraCount)
{
	objref manager;

	EmptyItemBufferStub();
	ItemBufferCount = count;
	NpcRecordCount = npcCount;
	ExtraNpcRecordCount = extraCount;
	ItemBufferBytes = count * sizeof(struct ItemRecord);
	if (ItemBuffer == 0)
		ItemBuffer = (struct ItemRecord far *) AllocateFarHeap(ItemBufferBytes, 2);
	if (ItemBuffer == 0)
		ReportOutOfFarMemory();
	AllocNpcRecords(&manager, NpcRecordCount, ExtraNpcRecordCount);
}

void ClearChunkItemLists(void)
{
	int i, j, k;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 16; j++)
			for (k = 0; k < 16; k++)
				ChunkItemLists[i][j][k] = 0;
}

/* link the records into a free list, each holding the next one's offset */
void BuildItemFreeList(void)
{
	int i;

	ItemBufferSegment = FP_SEG(ItemBuffer);
	FillFarBytes(ItemBuffer, ItemBufferBytes, 0);
	for (i = RESERVED_ITEMS; i < ItemBufferCount - 1; i++)
		ItemBuffer[i].next = (i + 1) * sizeof(struct ItemRecord);
	ItemBuffer[i].next = 0;
}

void ResetItemBuffer(void)
{
	objref manager;

	ItemFreeCount = ItemBufferCount - RESERVED_ITEMS;
	ItemRenderOrder.front = ItemRenderOrder.begin;
	ItemRenderOrder.back = ItemRenderOrder.end;
	BuildItemFreeList();
	ItemFreeList = RESERVED_ITEMS * sizeof(struct ItemRecord);
	DetachedItems = 0;
	ResetNpcRecords(&manager);
	ClearChunkItemLists();
	UnusedItemResetWord = 0;
	ForgetSavedRegions();
}
