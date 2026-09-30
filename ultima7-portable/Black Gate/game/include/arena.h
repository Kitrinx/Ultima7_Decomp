/* Linear memory: one arena, addressed by 32-bit offsets.
 * The game calls these offsets linear addresses; offset 0 is never handed out.
 * Bytes in the arena keep the little-endian order the game's files use.
 */
#ifndef ARENA_H
#define ARENA_H

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t *LinearBase;
extern uint32_t LinearSize;

/* Allocates the arena; the far heap, extended memory and the screen all come from it.
 * Later calls do nothing. */
void InitLinearMemory(uint32_t size);

#define LINEAR_MEMORY_SIZE (UINT32_C(32) << 20)

/* The 320x200 screen buffer, inside the arena. */
uint8_t *ScreenPixels(void);

/* Where InitLinearMemory put the far heap and extended (Voodoo) memory. */
extern uint32_t FarHeapArea, FarHeapAreaSize;
extern uint32_t ExtendedArea, ExtendedAreaSize;

#ifdef __cplusplus
}
#endif

#define LINEAR(address) (LinearBase + (uint32_t)(address))

static inline uint8_t LinearGet8(int32_t a) { return LINEAR(a)[0]; }
static inline uint16_t LinearGet16(int32_t a) { return (uint16_t)(LINEAR(a)[0] | LINEAR(a)[1] << 8); }
static inline uint32_t LinearGet32(int32_t a)
{
	return (uint32_t)LinearGet16(a) | (uint32_t)LinearGet16(a + 2) << 16;
}
static inline void LinearPut8(int32_t a, uint8_t v) { LINEAR(a)[0] = v; }
static inline void LinearPut16(int32_t a, uint16_t v) { LINEAR(a)[0] = (uint8_t)v; LINEAR(a)[1] = (uint8_t)(v >> 8); }
static inline void LinearPut32(int32_t a, uint32_t v) { LinearPut16(a, (uint16_t)v); LinearPut16(a + 2, (uint16_t)(v >> 16)); }

#endif
