/* Black Gate U7.EXE, resident segment 69 (file offsets 0x026c52 to 0x026f08, 694 bytes).
 * Borland C++ 2.0 -mm -O -G -d -P rebuilds it byte for byte as C++.
 */

/* path: voonpc.c */
#include "lowlevel.h"
#include "vooalloc.h"
#include "init.h"
#include "random.h"
#include "memapi.h"
#include "u7npc.h"
#include "voonpc.h"

/* NPC buffers live in voodoo memory; up to 32 at a time are cached in far memory. */
#define CACHE_SLOTS 32

struct NpcBuffer far *NpcCacheBuffers;
unsigned char NpcSlotUsed[CACHE_SLOTS];
int NpcInSlot[CACHE_SLOTS];
unsigned char NpcSlotOf[NPC_COUNT];
int NpcItemRefs[NPC_COUNT];
long NpcStore;
int NpcCacheFaultIndex;
int NpcCacheFaultValue;
int LastNpcSlot;

#define RECURSION_ERROR(line)   HaltWithMessage(__FILE__, line, "recursive voodoo")

static int NpcCacheSize = 0;

/* Checks that the NPC table and the slot table agree; on failure NpcCacheFaultIndex and
 * NpcCacheFaultValue name the culprit. */
unsigned char CheckNpcCache(void)
{
	unsigned char ok = 1;
	int i;
	int slot;
	int npc;

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

/* Loads NPC npc from the backing store into slot. */
void LoadNpcSlot(unsigned npc, int slot)
{
	static char busy = 0;

	if (busy)
		RECURSION_ERROR(124);
	busy = 1;
	CopyLinearToFar(&NpcCacheBuffers[slot], NpcStore + npc * sizeof(struct NpcBuffer), sizeof(struct NpcBuffer));
	NpcSlotOf[npc] = slot;
	NpcSlotUsed[slot] = 1;
	NpcInSlot[slot] = npc;
	busy = 0;
}

/* Writes slot back to its NPC's place in the backing store and frees the slot. */
void SaveNpcSlot(int slot)
{
	static char busy = 0;
	unsigned npc;

	if (busy)
		RECURSION_ERROR(158);
	busy = 1;
	npc = NpcInSlot[slot];
	CopyFarToLinear(NpcStore + npc * sizeof(struct NpcBuffer), &NpcCacheBuffers[slot], sizeof(struct NpcBuffer));
	NpcSlotOf[npc] = 255;
	NpcSlotUsed[slot] = 0;
	busy = 0;
}

/* The cached copy of NPC npc, loading it into a free slot, or evicting a random one, if needed. */
extern "C" struct NpcBuffer far *GetCachedNpcBuffer(NpcBufferPool *pool, int npc)
{
	static char busy = 0;
	int slot;
	int i;

	if (busy)
		RECURSION_ERROR(203);
	busy = 1;
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
	busy = 0;
	return NpcCacheBuffers + slot;
}

/* Sets up the slot cache for count NPCs. */
extern "C" unsigned char InitNpcCache(NpcBufferPool *pool, int count)
{
	unsigned char ok = 0;
	int i;

	if (NpcCacheSize == 0) {
		NpcCacheSize = count;
		NpcCacheBuffers = (struct NpcBuffer far *)AllocateFarHeap(CACHE_SLOTS * sizeof(struct NpcBuffer), 0);
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
