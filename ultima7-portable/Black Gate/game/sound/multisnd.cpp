/* Black Gate U7.EXE, resident segment 39 (file offsets 0x01c314 to 0x01c400, 236 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
#define SOUND_SLOTS 20

extern "C" void PlaySfx(uint8_t number, int16_t volume, int16_t pan, int16_t flags);

struct SoundSlot {
	int16_t handle;
	int8_t count;
	int8_t sfx;
	void reset();
};

/* The slots and the index of the current one. */
struct SoundSlotTable {
	int16_t current;
	SoundSlot slots[SOUND_SLOTS];
	SoundSlotTable();
	void playCurrent();
	void adjustCount(int8_t, int8_t);
};

SoundSlotTable SoundSlots;

void SoundSlot::reset()
{
	handle = -1;
	count++;
	sfx = -1;
}

SoundSlotTable::SoundSlotTable()
{
	int16_t i;

	current = 0;
	for (i = 0; i < SOUND_SLOTS; i++)
		slots[i].reset();
}

void SoundSlotTable::playCurrent()
{
	PlaySfx(slots[current].sfx, 255, 64, 0);
}

/* Raise or lower the count of every slot holding id. */
void SoundSlotTable::adjustCount(int8_t id, int8_t up)
{
	int16_t i;

	for (i = 0; i < SOUND_SLOTS; i++) {
		if (slots[i].sfx == id) {
			if (up)
				slots[i].count++;
			else
				slots[i].count--;
		}
	}
}
