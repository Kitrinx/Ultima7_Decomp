#include "u7port.h"
#include "arena.h"

extern "C" int32_t PackShapeFrames(int32_t shape, int32_t keep);

/* Packs a shape in linear memory down to the frames whose bits are set in the dword at keep:
 * dropped frames' bytes are closed up and their table entries zeroed. The shape starts with
 * its size, then a table of frame offsets. Returns the new size. */
int32_t PackShapeFrames(int32_t shape, int32_t keep)
{
	uint32_t bits = LinearGet32(keep);
	uint32_t newSize = LinearGet32(shape);
	uint32_t after = newSize - LinearGet32(shape + 4);
	int32_t frames = (int32_t) (LinearGet32(shape + 4) >> 2) - 1;
	int32_t entry = shape + 4;
	uint32_t from, to, size;
	int32_t i;

	if (frames <= 1)
		return (int32_t) newSize;
	for (; frames != 1; frames--, entry += 4, bits >>= 1) {
		to = shape + LinearGet32(entry);
		from = shape + LinearGet32(entry + 4);
		size = from - to;
		after -= size;
		if (bits & 1)
			continue;
		newSize -= size;
		memmove(LINEAR(to), LINEAR(from), after);
		LinearPut32(entry, 0);
		for (i = 1; i < frames; i++)
			LinearPut32(entry + i * 4, LinearGet32(entry + i * 4) - size);
		LinearPut32(shape, newSize);
	}
	size = LinearGet32(shape) - LinearGet32(entry);
	if ((bits & 1) == 0) {
		newSize -= size;
		LinearPut32(entry, 0);
	}
	LinearPut32(shape, newSize);
	return (int32_t) newSize;
}
