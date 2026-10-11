/* Black Gate ENDGAME.EXE: xmidi.c, and the driver and timbre loading of sounddrv.c. */

#include "u7port.h"
#include "plat.h"
#include "memapi.h"
#include "ail.h"
#include "xmidi.h"

namespace Endgame {

static uint16_t ReadLittleWord(const uint8_t *p) { return (uint16_t) (p[0] | p[1] << 8); }

static uint32_t ReadBigLong(const uint8_t *p)
{
	return (uint32_t) p[0] << 24 | (uint32_t) p[1] << 16 | (uint32_t) p[2] << 8 | p[3];
}

/* Loads the driver and starts it at its default settings. Without its synth (DOS stopped with
 * "Sound hardware not found") the ending goes on silent. */
SoundDriver::SoundDriver(const char *driver, const char *timbres)
{
	int16_t file = plat_file_open(driver, PLAT_FILE_READ);
	drvr_desc *descriptor;
	uint16_t size;

	handle = -1;
	image = 0;
	timbreCache = 0;
	timbreFile = -1;
	if (file < 0)
		return;
	size = (uint16_t) plat_file_length(file);
	image = AllocateFarHeap(size, 2);
	plat_file_read(file, image, size);
	plat_file_close(file);
	handle = AIL_register_driver(image);
	if (handle == -1)
		return;
	descriptor = AIL_describe_driver(handle);
	if (!AIL_detect_device(handle, descriptor->default_IO, descriptor->default_IRQ,
			descriptor->default_DMA, descriptor->default_DRQ)) {
		AIL_release_driver_handle(handle);
		handle = -1;
		return;
	}
	AIL_init_driver(handle, descriptor->default_IO, descriptor->default_IRQ,
		descriptor->default_DMA, descriptor->default_DRQ);
	size = AIL_default_timbre_cache_size(handle);
	if (size) {
		timbreCache = AllocateFarHeap(size, 0);
		AIL_define_timbre_cache(handle, timbreCache, size);
	}
	timbreFile = plat_file_open(timbres, PLAT_FILE_READ);
}

SoundDriver::~SoundDriver()
{
	if (handle != -1) {
		AIL_shutdown_driver(handle, "SCSCSCFY!");
		AIL_release_driver_handle(handle);
	}
	if (timbreFile >= 0)
		plat_file_close(timbreFile);
	if (timbreCache)
		FreeFarHeap(timbreCache);
	if (image)
		FreeFarHeap(image);
}

/* Reads a timbre from the timbre file, its size in its first word; 0 if absent. Entries are a
 * patch, a bank and where the timbre starts; a bank of 0xff ends them. */
uint8_t *SoundDriver::loadTimbre(uint8_t bank, uint8_t patch)
{
	uint8_t entry[6];
	uint8_t size[2];
	uint8_t *data;
	int32_t at = 0;

	if (timbreFile < 0)
		return 0;
	do {
		plat_file_seek(timbreFile, at, PLAT_SEEK_SET);
		if (plat_file_read(timbreFile, entry, 6) < 2 || entry[1] == 0xff)
			return 0;
		at += 6;
	} while (entry[1] != bank || entry[0] != patch);
	plat_file_seek(timbreFile, (int32_t) (entry[2] | entry[3] << 8 | entry[4] << 16 | (uint32_t) entry[5] << 24),
		PLAT_SEEK_SET);
	plat_file_read(timbreFile, size, 2);
	data = new uint8_t[ReadLittleWord(size)];
	memcpy(data, size, 2);
	plat_file_read(timbreFile, data + 2, ReadLittleWord(size) - 2);
	return data;
}

/* Gives the driver a timbre it does not have yet. */
void SoundDriver::installTimbre(uint8_t bank, uint8_t patch)
{
	uint8_t *data;

	if (!AIL_timbre_status(handle, bank, patch)) {
		data = loadTimbre(bank, patch);
		AIL_install_timbre(handle, bank, patch, data);
		delete[] data;
	}
}

/* Reads an XMIDI file whole; its XDIR form says how many sequences it holds. */
XmidiPlayer::XmidiPlayer(SoundDriver *d, const char *name)
{
	int16_t file;

	driver = d;
	data = 0;
	size = 0;
	count = 0;
	handles = 0;
	file = plat_file_open(name, PLAT_FILE_READ);
	if (file < 0)
		return;
	size = plat_file_length(file);
	data = new uint8_t[size];
	plat_file_read(file, data, size);
	plat_file_close(file);
	count = 1;
	if (size >= 12 && memcmp(data, "FORM", 4) == 0 && memcmp(data + 8, "XDIR", 4) == 0) {
		const uint8_t *chunk = data + 12;
		const uint8_t *end = data + 8 + ReadBigLong(data + 4);

		for (; chunk + 8 <= end; chunk += 8 + ((ReadBigLong(chunk + 4) + 1) & ~1u)) {
			if (memcmp(chunk, "INFO", 4) == 0) {
				count = ReadLittleWord(chunk + 8);
				break;
			}
		}
	}
	handles = new int16_t[count];
	states = new void *[count];
	for (uint16_t i = 0; i < count; i++) {
		handles[i] = -1;
		states[i] = 0;
	}
}

XmidiPlayer::~XmidiPlayer()
{
	for (uint16_t i = 0; i < count; i++) {
		if (handles[i] != -1) {
			if (isPlaying(i))
				stop(i);
			AIL_release_sequence_handle(driver->handle, handles[i]);
		}
		if (states[i])
			FreeFarHeap(states[i]);
	}
	delete[] states;
	delete[] handles;
	delete[] data;
}

/* Registers sequence n the first time, with the timbres it asks for, and starts it. */
void XmidiPlayer::play(int16_t n)
{
	uint16_t request;

	if (driver == 0 || n >= count)
		return;
	if (handles[n] == -1) {
		states[n] = AllocateFarHeap(AIL_state_table_size(driver->handle), 0);
		handles[n] = AIL_register_sequence(driver->handle, data, n, states[n], 0);
		if (handles[n] == -1) {
			FreeFarHeap(states[n]);
			states[n] = 0;
			return;
		}
		while (((request = AIL_timbre_request(driver->handle, handles[n])) >> 8) != 0xff)
			driver->installTimbre((uint8_t) (request >> 8), (uint8_t) request);
	}
	AIL_start_sequence(driver->handle, handles[n]);
}

void XmidiPlayer::stop(int16_t n)
{
	if (driver && n < count)
		AIL_stop_sequence(driver->handle, handles[n]);
}

uint8_t XmidiPlayer::isDone(int16_t n)
{
	if (driver == 0 || n >= count)
		return 1;
	return AIL_sequence_status(driver->handle, handles[n]) == SEQ_DONE;
}

uint8_t XmidiPlayer::isPlaying(int16_t n)
{
	if (driver == 0 || n >= count)
		return 0;
	return AIL_sequence_status(driver->handle, handles[n]) == SEQ_PLAYING;
}

}
