/* Serpent Isle ENDGAME.EXE, resident segment 9 (file offsets 0x009c3a to 0x00a09d, 1123 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include "digital.h"
#include "shutdown.h"
#include "fileio.h"

char *NoSampleMemory = "No memory for digitized sample.\n";
char *NoStreamMemory = "No memory for speech double buffer.\n";
char *NoSampleDriver = "No sound driver for sample.\n";
char *NotDspDriver = "Sound driver not an DSP driver.\n";

void DigitalSound::setDriver(SoundDriver *d)
{
	if (d && d->type() != DSP_DRVR)
		FatalMessage(NotDspDriver);
	driver = d;
}

/* Reads a raw sample file whole into far memory. */
void DigitalSound::load(char *name, int packType, int rate)
{
	if (driver) {
		DiskFile file;

		if (file.open(name, FILE_READ)) {
			sound.len = file.getLength();
			sample.allocate(sound.len, FAR_MEMORY, 0, 1, NoSampleMemory);
			ReadAll(&file, sample.pointer());
			sound.data = sample.pointer();
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
		sample.release(0);
	}
}

void DigitalSound::setAndLoad(SoundDriver *d, char *name, int packType, int rate)
{
	if (driver) {
		setDriver(d);
		load(name, packType, rate);
	}
}

void DigitalSound::play()
{
	if (driver && sample.isAllocated()) {
		AIL_register_sound_buffer(driver->getHandle(), 0, &sound);
		AIL_start_digital_playback(driver->getHandle());
	}
}

unsigned char DigitalSound::isDone()
{
	if (driver)
		return AIL_sound_buffer_status(driver->getHandle(), 0) == DAC_DONE;
	else
		return 1;
}

unsigned char DigitalSound::isPlaying()
{
	if (driver)
		return AIL_sound_buffer_status(driver->getHandle(), 0) == DAC_PLAYING;
	else
		return 0;
}

unsigned char DigitalSound::isPaused()
{
	if (driver)
		return AIL_sound_buffer_status(driver->getHandle(), 0) == DAC_PAUSED;
	else
		return 1;
}

unsigned char DigitalSound::isStopped()
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
		DiskFile file;

		if (file.open(name, FILE_READ)) {
			sample.allocate(file.getLength(), FAR_MEMORY, 0, 1, NoSampleMemory);
			ReadAll(&file, sample.pointer());
		}
	}
}

void DigitalSound::setData(void far *data)
{
	if (driver)
		sample.set(data, FAR_MEMORY, 1);
}

void VocSound::play(int block)
{
	if (driver && sample.isAllocated()) {
		AIL_play_VOC_file(driver->getHandle(), sample.pointer(), block);
		AIL_start_digital_playback(driver->getHandle());
	}
}

unsigned char VocSound::isDone()
{
	if (driver)
		return AIL_VOC_playback_status(driver->getHandle()) == DAC_DONE;
	else
		return 1;
}

unsigned char VocSound::isPlaying()
{
	if (driver)
		return AIL_VOC_playback_status(driver->getHandle()) == DAC_PLAYING;
	else
		return 0;
}

unsigned char VocSound::isPaused()
{
	if (driver)
		return AIL_VOC_playback_status(driver->getHandle()) == DAC_PAUSED;
	else
		return 1;
}

unsigned char VocSound::isStopped()
{
	if (driver)
		return AIL_VOC_playback_status(driver->getHandle()) == DAC_STOPPED;
	else
		return 1;
}
