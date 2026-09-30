/* Black Gate ENDGAME.EXE: xmidi.c, and the timbre loading of sounddrv.c. */

#include "u7port.h"
#include "plat.h"
#include "ailmt32.h"
#include "xmidi.h"

namespace Endgame {

static uint16_t ReadLittleWord(const uint8_t *p) { return (uint16_t) (p[0] | p[1] << 8); }

static uint32_t ReadBigLong(const uint8_t *p)
{
	return (uint32_t) p[0] << 24 | (uint32_t) p[1] << 16 | (uint32_t) p[2] << 8 | p[3];
}

SoundDriver::SoundDriver(const char *timbres)
{
	timbreFile = plat_file_open(timbres, PLAT_FILE_READ);
}

SoundDriver::~SoundDriver()
{
	Mt32Close("SCSCSCFY!");
	if (timbreFile >= 0)
		plat_file_close(timbreFile);
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

	if (!Mt32TimbreStatus(bank, patch)) {
		data = loadTimbre(bank, patch);
		Mt32InstallTimbre(bank, patch, data);
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
	for (uint16_t i = 0; i < count; i++)
		handles[i] = -1;
}

XmidiPlayer::~XmidiPlayer()
{
	delete[] handles;
	delete[] data;
}

/* Registers sequence n the first time, with the timbres it asks for, and starts it. */
void XmidiPlayer::play(int16_t n)
{
	int16_t request;

	if (driver == 0 || n >= count)
		return;
	if (handles[n] == -1) {
		handles[n] = Mt32RegisterSequence(data, size, n);
		if (handles[n] == -1)
			return;
		while ((request = Mt32TimbreRequest(handles[n])) != -1)
			driver->installTimbre((uint8_t) (request >> 8), (uint8_t) request);
	}
	Mt32StartSequence(handles[n]);
}

void XmidiPlayer::stop(int16_t n)
{
	if (driver && n < count)
		Mt32StopSequence(handles[n]);
}

uint8_t XmidiPlayer::isDone(int16_t n)
{
	if (driver == 0 || n >= count)
		return 1;
	return Mt32SequenceStatus(handles[n]) == SEQ_DONE;
}

uint8_t XmidiPlayer::isPlaying(int16_t n)
{
	if (driver == 0 || n >= count)
		return 0;
	return Mt32SequenceStatus(handles[n]) == SEQ_PLAYING;
}

}
