/* Serpent Isle SI.EXE, resident segment 77 (file offsets 0x030bf4 to 0x030d53, 351 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
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

uint16_t ItemBufferBytes = 0;
int16_t ItemBufferCount = 0;
int16_t NpcRecordCount = 0;
int16_t ExtraNpcRecordCount = 0;

void EmptyItemBufferStub(void)
{
}

void AllocItemBuffer(int16_t count, int16_t npcCount, int16_t extraCount)
{
	objref manager;

	EmptyItemBufferStub();
	ItemBufferCount = count;
	NpcRecordCount = npcCount;
	ExtraNpcRecordCount = extraCount;
	ItemBufferBytes = count * sizeof(struct ItemRecord);
	if (ItemBuffer == 0)
		ItemBuffer = (struct ItemRecord *) AllocateFarHeap(ItemBufferBytes, 2);
	if (ItemBuffer == 0)
		ReportOutOfFarMemory();
	AllocNpcRecords(&manager, NpcRecordCount, ExtraNpcRecordCount);
}

void ClearChunkItemLists(void)
{
	int16_t i, j, k;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 16; j++)
			for (k = 0; k < 16; k++)
				ChunkItemLists[i][j][k] = 0;
}

/* link the records into a free list, each holding the next one's offset */
void BuildItemFreeList(void)
{
	int16_t i;

	ItemBufferBase = (uint8_t *)ItemBuffer;
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

extern "C" void ResetItembufGlobals(void)
{
	ItemBufferBytes = 0;
	ItemBufferCount = 0;
	NpcRecordCount = 0;
	ExtraNpcRecordCount = 0;
}
