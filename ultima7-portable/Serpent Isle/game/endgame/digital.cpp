/* Serpent Isle ENDGAME.EXE, resident segment 9 (file offsets 0x009c3a to 0x00a09d, 1123 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "plat.h"
#include "digital.h"

namespace Endgame {

char *NoSampleMemory = "No memory for digitized sample.\n";
char *NoStreamMemory = "No memory for speech double buffer.\n";
char *NoSampleDriver = "No sound driver for sample.\n";
char *NotDspDriver = "Sound driver not an DSP driver.\n";

void DigitalSound::setDriver(SoundDriver *d)
{
	if (d && d->type() != DSP_DRVR)
		plat_fatal(NotDspDriver);
	driver = d;
}

/* Reads a raw sample file whole into far memory. */
void DigitalSound::load(char *name, int16_t packType, int16_t rate)
{
	if (driver) {
		int16_t file = plat_file_open(name, PLAT_FILE_READ);

		if (file >= 0) {
			sound.len = plat_file_length(file);
			sample = FarAllocate(sound.len, 0, NoSampleMemory);
			plat_file_read(file, sample, sound.len);
			plat_file_close(file);
			sound.data = sample;
			sound.pack_type = packType;
			sound.sample_rate = rate;
		}
	}
}

void DigitalSound::stop()
{
	if (driver) {
		if (isPlaying())
			stopPlayback();
		if (sample)
			FreeFarHeap(sample);
		sample = 0;
	}
}

void DigitalSound::setAndLoad(SoundDriver *d, char *name, int16_t packType, int16_t rate)
{
	if (driver) {
		setDriver(d);
		load(name, packType, rate);
	}
}

void DigitalSound::play()
{
	if (driver && sample) {
		AIL_register_sound_buffer(driver->getHandle(), 0, &sound);
		AIL_start_digital_playback(driver->getHandle());
	}
}

uint8_t DigitalSound::isDone()
{
	if (driver)
		return AIL_sound_buffer_status(driver->getHandle(), 0) == DAC_DONE;
	else
		return 1;
}

uint8_t DigitalSound::isPlaying()
{
	if (driver)
		return AIL_sound_buffer_status(driver->getHandle(), 0) == DAC_PLAYING;
	else
		return 0;
}

uint8_t DigitalSound::isPaused()
{
	if (driver)
		return AIL_sound_buffer_status(driver->getHandle(), 0) == DAC_PAUSED;
	else
		return 1;
}

uint8_t DigitalSound::isStopped()
{
	if (driver)
		return AIL_sound_buffer_status(driver->getHandle(), 0) == DAC_STOPPED;
	else
		return 1;
}

/* Reads a sound file whole into far memory, leaving the sound record alone. */
void DigitalSound::loadFile(char *name)
{
	if (driver) {
		int16_t file = plat_file_open(name, PLAT_FILE_READ);

		if (file >= 0) {
			sample = FarAllocate(plat_file_length(file), 0, NoSampleMemory);
			plat_file_read(file, sample, plat_file_length(file));
			plat_file_close(file);
		}
	}
}

void DigitalSound::setData(void *data)
{
	if (driver)
		sample = data;
}

void VocSound::play(int16_t block)
{
	if (driver && sample) {
		AIL_play_VOC_file(driver->getHandle(), sample, block);
		AIL_start_digital_playback(driver->getHandle());
	}
}

uint8_t VocSound::isDone()
{
	if (driver)
		return AIL_VOC_playback_status(driver->getHandle()) == DAC_DONE;
	else
		return 1;
}

uint8_t VocSound::isPlaying()
{
	if (driver)
		return AIL_VOC_playback_status(driver->getHandle()) == DAC_PLAYING;
	else
		return 0;
}

uint8_t VocSound::isPaused()
{
	if (driver)
		return AIL_VOC_playback_status(driver->getHandle()) == DAC_PAUSED;
	else
		return 1;
}

uint8_t VocSound::isStopped()
{
	if (driver)
		return AIL_VOC_playback_status(driver->getHandle()) == DAC_STOPPED;
	else
		return 1;
}

}
