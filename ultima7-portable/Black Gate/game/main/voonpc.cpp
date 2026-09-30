/* Black Gate U7.EXE, resident segment 69 (file offsets 0x026c52 to 0x026f08, 694 bytes).
 * Borland C++ 2.0 -mm -O -G -d -P rebuilds it byte for byte as C++.
 */

/* path: voonpc.c */
#include "u7port.h"
#include "lowlevel.h"
#include "vooalloc.h"
#include "init.h"
#include "random.h"
#include "memapi.h"
#include "u7npc.h"
#include "voonpc.h"

/* NPC buffers live in voodoo memory; up to 32 at a time are cached in far memory. */
#define CACHE_SLOTS 32

struct NpcBuffer *NpcCacheBuffers;
uint8_t NpcSlotUsed[CACHE_SLOTS];
int16_t NpcInSlot[CACHE_SLOTS];
uint8_t NpcSlotOf[NPC_COUNT];
int16_t NpcItemRefs[NPC_COUNT];
int32_t NpcStore;
int16_t NpcCacheFaultIndex;
int16_t NpcCacheFaultValue;
int16_t LastNpcSlot;

#define RECURSION_ERROR(line)   HaltWithMessage(__FILE__, line, "recursive voodoo")

static int16_t NpcCacheSize = 0;

/* Checks that the NPC table and the slot table agree; on failure NpcCacheFaultIndex and
 * NpcCacheFaultValue name the culprit. */
uint8_t CheckNpcCache(void)
{
	uint8_t ok = 1;
	int16_t i;
	int16_t slot;
	int16_t npc;

	for (i = 0; i < NPC_COUNT; i++) {
		slot = NpcSlotOf[i];
		if (slot != 255 && (slot < 0 || slot >= CACHE_SLOTS || !NpcSlotUsed[slot])) {
			NpcCacheFaultIndex = i;
			NpcCacheFaultValue = slot;
			ok = 0;
			break;
		}
	}
	for (i = 0; i < CACHE_SLOTS; i++) {
		if (NpcSlotUsed[i]) {
			npc = NpcInSlot[i];
			if (npc < 0 || npc >= NPC_COUNT || NpcSlotOf[npc] != i) {
				NpcCacheFaultIndex = -i;
				NpcCacheFaultValue = npc;
				ok = 0;
				break;
			}
			if (NpcSlotUsed[i] != 1) {
				NpcCacheFaultIndex = i + 1000;
				NpcCacheFaultValue = NpcSlotUsed[i];
				ok = 0;
				break;
			}
		}
	}
	return ok;
}

static int8_t LoadNpcSlotBusy = 0;

/* Loads NPC npc from the backing store into slot. */
void LoadNpcSlot(uint16_t npc, int16_t slot)
{
	if (LoadNpcSlotBusy)
		RECURSION_ERROR(124);
	LoadNpcSlotBusy = 1;
	CopyLinearToFar(&NpcCacheBuffers[slot], NpcStore + npc * sizeof(struct NpcBuffer), sizeof(struct NpcBuffer));
	NpcSlotOf[npc] = slot;
	NpcSlotUsed[slot] = 1;
	NpcInSlot[slot] = npc;
	LoadNpcSlotBusy = 0;
}

static int8_t SaveNpcSlotBusy = 0;

/* Writes slot back to its NPC's place in the backing store and frees the slot. */
void SaveNpcSlot(int16_t slot)
{
	uint16_t npc;

	if (SaveNpcSlotBusy)
		RECURSION_ERROR(158);
	SaveNpcSlotBusy = 1;
	npc = NpcInSlot[slot];
	CopyFarToLinear(NpcStore + npc * sizeof(struct NpcBuffer), &NpcCacheBuffers[slot], sizeof(struct NpcBuffer));
	NpcSlotOf[npc] = 255;
	NpcSlotUsed[slot] = 0;
	SaveNpcSlotBusy = 0;
}

static int8_t GetCachedNpcBufferBusy = 0;

/* The cached copy of NPC npc, loading it into a free slot, or evicting a random one, if needed. */
extern "C" struct NpcBuffer *GetCachedNpcBuffer(NpcBufferPool *pool, int16_t npc)
{
	int16_t slot;
	int16_t i;

	if (GetCachedNpcBufferBusy)
		RECURSION_ERROR(203);
	GetCachedNpcBufferBusy = 1;
	slot = NpcSlotOf[npc];
	if (slot == 255) {
		for (i = 0; i < CACHE_SLOTS; i++) {
			if (!NpcSlotUsed[i]) {
				slot = i;
				LastNpcSlot = slot;
				break;
			}
		}
		if (slot == 255) {
			slot = GenerateRandomIntegerInRange(CACHE_SLOTS);
			LastNpcSlot = slot;
			SaveNpcSlot(slot);
		}
		LoadNpcSlot(npc, slot);
	}
	GetCachedNpcBufferBusy = 0;
	return NpcCacheBuffers + slot;
}

/* Sets up the slot cache for count NPCs. */
extern "C" uint8_t InitNpcCache(NpcBufferPool *pool, int16_t count)
{
	uint8_t ok = 0;
	int16_t i;

	if (NpcCacheSize == 0) {
		NpcCacheSize = count;
		NpcCacheBuffers = (struct NpcBuffer *)AllocateFarHeap(CACHE_SLOTS * sizeof(struct NpcBuffer), 0);
		if (NpcCacheBuffers != 0) {
			for (i = 0; i < count; i++) {
				NpcSlotOf[i] = 255;
				NpcItemRefs[i] = 0;
			}
			for (i = 0; i < CACHE_SLOTS; i++)
				NpcSlotUsed[i] = 0;
			NpcStore = AllocateVoodooMemory(&VoodooXmsBlock, count * sizeof(struct NpcBuffer));
			if (NpcStore != 0)
				ok = 1;
		}
	}
	return ok;
}

extern "C" void ResetVoonpcGlobals(void)
{
	NpcCacheBuffers = 0;
	memset(NpcSlotUsed, 0, sizeof(NpcSlotUsed));
	memset(NpcInSlot, 0, sizeof(NpcInSlot));
	memset(NpcSlotOf, 0, sizeof(NpcSlotOf));
	memset(NpcItemRefs, 0, sizeof(NpcItemRefs));
	NpcStore = 0;
	NpcCacheFaultIndex = 0;
	NpcCacheFaultValue = 0;
	LastNpcSlot = 0;
	NpcCacheSize = 0;
	LoadNpcSlotBusy = 0;
	SaveNpcSlotBusy = 0;
	GetCachedNpcBufferBusy = 0;
}
