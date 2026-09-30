/* Black Gate U7.EXE, resident segment 39 (file offsets 0x01c314 to 0x01c400, 236 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#define SOUND_SLOTS 20

extern "C" void far PlaySfx(unsigned char number, int volume, int pan, int flags);

struct SoundSlot {
	int handle;
	char count;
	char sfx;
	void reset();
};

/* The slots and the index of the current one. */
struct SoundSlotTable {
	int current;
	SoundSlot slots[SOUND_SLOTS];
	SoundSlotTable();
	void playCurrent();
	void adjustCount(char, char);
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
	int i;

	current = 0;
	for (i = 0; i < SOUND_SLOTS; i++)
		slots[i].reset();
}

void SoundSlotTable::playCurrent()
{
	PlaySfx(slots[current].sfx, 255, 64, 0);
}

/* Raise or lower the count of every slot holding id. */
void SoundSlotTable::adjustCount(char id, char up)
{
	int i;

	for (i = 0; i < SOUND_SLOTS; i++) {
		if (slots[i].sfx == id) {
			if (up)
				slots[i].count++;
			else
				slots[i].count--;
		}
	}
}
