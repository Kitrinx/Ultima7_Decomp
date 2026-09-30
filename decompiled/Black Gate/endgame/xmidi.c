/* Black Gate ENDGAME.EXE, resident segment 8 (file offsets 0x009eb1 to 0x00a40c, 1371 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include "xmidi.h"
#include "iff.h"
#include "fileio.h"
#include "shutdown.h"

char *NotXmidiDriver = "Sound driver not an XMIDI player.\n";
char *NoSequenceMemory = "No memory for XMIDI sequence.\n";
char *NoSequenceDriver = "No sound driver for sequence.\n";

void XmidiSequence::release()
{
	if (driver) {
		if (isPlaying())
			stop();
		AIL_release_sequence_handle(driver->getHandle(), handle);
		state.release(0);
		handle = -1;
	}
}

void XmidiSequence::setDriver(SoundDriver *d)
{
	if (d && d->type() != XMIDI_DRVR)
		FatalMessage(NotXmidiDriver);
	driver = d;
}

/* Registers sequence n of an XMIDI file in memory and installs the timbres it asks for. */
void XmidiSequence::registerSequence(void far *xmidi, int n)
{
	Timbre timbre;
	unsigned size;

	if (driver && handle == -1) {
		size = AIL_state_table_size(driver->getHandle());
		state.allocate(size, FAR_MEMORY, 0, 1, "No mem for XMIDI state table.");
		handle = AIL_register_sequence(driver->getHandle(), xmidi, n, state.pointer(), 0);
		if (handle == -1) {
			state.release(0);
			return;
		}
		for (;;) {
			*(unsigned *)&timbre = AIL_timbre_request(driver->getHandle(), handle);
			if (timbre.bank != -1)
				driver->installTimbre(timbre);
			else
				break;
		}
	}
}

void XmidiSong::load(char *name, unsigned char timbres)
{
	if (driver) {
		if (timbres)
			driver->installTimbres(name);
		DiskFile file;
		if (file.open(name, FILE_READ)) {
			data.allocate(file.getLength(), FAR_MEMORY, 0, 1, NoSequenceMemory);
			ReadAll(&file, data.pointer());
		}
	}
}

void XmidiSong::setAndLoad(SoundDriver *d, char *name, unsigned char timbres)
{
	setDriver(d);
	load(name, timbres);
}

void XmidiPlayer::setDriver(SoundDriver *d)
{
	if (d && d->type() != XMIDI_DRVR)
		FatalMessage(NotXmidiDriver);
	driver = d;
}

/* Reads an XMIDI file whole and makes a sequence for each one its XDIR form counts. */
void XmidiPlayer::load(char *name, unsigned char timbres)
{
	IffFile file;
	unsigned i;

	if (driver) {
		if (timbres)
			driver->installTimbres(name);
		if (file.open(name, FILE_READ)) {
			count = 1;
			if (file.findForm(XdirId)) {
				if (file.findChunk("INFO"))
					count = file.readWord();
				file.leaveForm();
			}
			sequences = new XmidiSequence[count];
			for (i = 0; i < count; i++)
				sequences[i].setDriver(driver);
			data.allocate(file.getLength(), FAR_MEMORY, 0, 1, NoSequenceMemory);
			file.File::read(data.pointer(), file.getLength(), 0L);
		}
	}
}

void XmidiPlayer::setAndLoad(SoundDriver *d, char *name, unsigned char timbres)
{
	setDriver(d);
	load(name, timbres);
}

void XmidiPlayer::release()
{
	unsigned i;

	if (driver) {
		for (i = 0; i < count; i++)
			sequences[i].release();
		delete sequences;
		sequences = 0;
		count = 0;
		data.release(0);
	}
}

void XmidiPlayer::play(int n)
{
	if (driver) {
		prepare(n);
		sequences[n].start();
	}
}

void XmidiPlayer::release(int n)
{
	if (driver)
		sequences[n].release();
}

void XmidiPlayer::stop(int n)
{
	if (driver)
		sequences[n].stop();
}
