/* Serpent Isle ENDGAME.EXE, resident segment 8 (file offsets 0x00973f to 0x009c3a, 1275 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "xmidi.h"

namespace Endgame {

char *NotXmidiDriver = "Sound driver not an XMIDI player.\n";
char *NoSequenceMemory = "No memory for XMIDI sequence.\n";
char *NoSequenceDriver = "No sound driver for sequence.\n";

void XmidiSequence::release()
{
	if (driver) {
		if (isPlaying())
			stop();
		AIL_release_sequence_handle(driver->getHandle(), handle);
		if (state)
			FreeFarHeap(state);
		state = 0;
		handle = -1;
	}
}

void XmidiSequence::setDriver(SoundDriver *d)
{
	if (d && d->type() != XMIDI_DRVR)
		plat_fatal(NotXmidiDriver);
	driver = d;
}

/* Registers sequence n of an XMIDI file in memory and installs the timbres it asks for. */
void XmidiSequence::registerSequence(void *xmidi, int16_t n)
{
	Timbre timbre;
	uint16_t size;

	if (driver && handle == -1) {
		size = AIL_state_table_size(driver->getHandle());
		state = FarAllocate(size, 0, "No mem for XMIDI state table.");
		handle = AIL_register_sequence(driver->getHandle(), xmidi, n, state, 0);
		if (handle == -1) {
			FreeFarHeap(state);
			state = 0;
			return;
		}
		for (;;) {
			*(uint16_t *)&timbre = AIL_timbre_request(driver->getHandle(), handle);
			if (timbre.bank != -1)
				driver->installTimbre(timbre);
			else
				break;
		}
	}
}

void XmidiSong::load(char *name, uint8_t timbres)
{
	if (driver) {
		/* installTimbres is not linked: the scenes pass 0. */
		int16_t file = plat_file_open(name, PLAT_FILE_READ);
		if (file >= 0) {
			data = FarAllocate(plat_file_length(file), 0, NoSequenceMemory);
			plat_file_read(file, data, plat_file_length(file));
			plat_file_close(file);
		}
	}
}

void XmidiSong::setAndLoad(SoundDriver *d, char *name, uint8_t timbres)
{
	setDriver(d);
	load(name, timbres);
}

}
