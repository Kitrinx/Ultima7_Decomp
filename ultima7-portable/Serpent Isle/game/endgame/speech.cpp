/* Serpent Isle ENDGAME.EXE, resident segment 9 (file offsets 0x00a09d to 0x00a739, 1692 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "speech.h"

namespace Endgame {

/* Creative Voice file header and block types. */
#define VOC_FIRST_BLOCK     0x14
#define VOC_CHECK           0x18
#define VOC_CHECK_VALUE     0x1129
#define VOC_END             0
#define VOC_SOUND           1
#define VOC_MORE_SOUND      2
#define VOC_SILENCE         3
#define VOC_MARKER          4
#define VOC_TEXT            5
#define VOC_REPEAT          6
#define VOC_END_REPEAT      7

static int16_t useSecond = 0;

void StreamSound::init(SoundDriver *d, const uint8_t *from, int32_t size, uint16_t buffer)
{
	memory = from;
	second = 0;
	setDriver(d);
	if (driver) {
		source = position = 0;
		length = size;
		bufferSize = buffer;
		sample = FarAllocate(bufferSize, 0, NoStreamMemory);
		second = (uint8_t *) FarAllocate(bufferSize, 0, NoStreamMemory);
	}
}

StreamSound::~StreamSound()
{
	if (second)
		FreeFarHeap(second);
}

void StreamSound::readLinear(void *to, int32_t size)
{
	CopyLinearToFar(to, position, size);
	position += size;
}

/* Refills whichever buffer AIL has finished with and keeps playback going. */
void StreamSound::service()
{
	if (active && driver) {
		for (int16_t i = 0; i < 2; i++) {
			if (AIL_sound_buffer_status(driver->getHandle(), i) == DAC_DONE && sound.len) {
				buffer.len = bufferSize < sound.len ? bufferSize : sound.len;
				sound.len -= buffer.len;
				if ((useSecond ^= 1) == 0) {
					readLinear(sample, buffer.len);
					buffer.data = sample;
				} else {
					readLinear(second, buffer.len);
					buffer.data = second;
				}
				AIL_register_sound_buffer(driver->getHandle(), i, &buffer);
			}
		}
		AIL_start_digital_playback(driver->getHandle());
		if (isDone()) {
			AIL_stop_digital_playback(driver->getHandle());
			active = 0;
		}
	} else
		return;
}

uint8_t StreamSound::isDone()
{
	if (driver) {
		if (sound.len == 0 && AIL_sound_buffer_status(driver->getHandle(), 0) == DAC_DONE &&
				AIL_sound_buffer_status(driver->getHandle(), 1) == DAC_DONE)
			return 1;
		return 0;
	}
	return 1;
}

void StreamSound::start(uint8_t packType, uint8_t rate)
{
	if (driver) {
		sound.sample_rate = rate;
		sound.pack_type = packType;
		sound.len = length;
		buffer = sound;
		active = 1;
		service();
	}
}

void SpeechStream::readLinear(void *to, int32_t size)
{
	CopyLinearToFar(to, source + offset, size);
	offset += size;
}

/* Reads a block's three-byte length. */
void SpeechStream::readHeader()
{
	int32_t high;

	blockSize = PeekWord(source + offset);
	offset += 2;
	high = PeekByte(source + offset++);
	blockSize += high << 16;
	blockEnd = offset + blockSize;
}

/* Moves on to the next block of sound, repeating as the file asks. */
uint8_t SpeechStream::nextBlock()
{
	offset = blockEnd;
	if (--repeats != 0) {
		sound.len = blockSize;
		source = offset;
		return 1;
	}
	repeats = 1;
	while ((type = PeekByte(source + offset++)) != VOC_END) {
		readHeader();
		switch (type) {
		case VOC_SOUND:
			sound.sample_rate = PeekByte(source + offset++);
			sound.pack_type = PeekByte(source + offset++);
			blockSize = blockSize - 2;
			sound.len = blockSize;
			return 1;
		case VOC_MORE_SOUND:
			sound.len = blockSize;
			return 1;
		case VOC_SILENCE:
			break;
		case VOC_MARKER:
			return 0;
		case VOC_TEXT:
			break;
		case VOC_REPEAT:
			repeats = PeekWord(source + offset);
			break;
		case VOC_END_REPEAT:
			break;
		}
		offset = blockEnd;
	}
	return 0;
}

uint8_t SpeechStream::isDone()
{
	if (driver) {
		if (active) {
			if (sound.len == 0 && type && !nextBlock())
				type = 0;
			if (sound.len == 0 && AIL_sound_buffer_status(driver->getHandle(), 0) == DAC_DONE &&
					AIL_sound_buffer_status(driver->getHandle(), 1) == DAC_DONE)
				return 1;
			return 0;
		} else
			return 1;
	}
	return 1;
}

/* Finds the first sound block at or after marker and starts playing from there. */
void SpeechStream::play(int16_t marker)
{
	if (driver == 0)
		return;
	if (PeekWord(source + VOC_CHECK) != VOC_CHECK_VALUE)
		return;
	offset = PeekWord(source + VOC_FIRST_BLOCK);
	position += offset;
	int16_t current = -1;
	repeats = 1;
	while ((type = PeekByte(source + offset++)) != VOC_END) {
		readHeader();
		switch (type) {
		case VOC_SOUND:
			sound.sample_rate = PeekByte(source + offset++);
			sound.pack_type = PeekByte(source + offset++);
			sound.len = blockSize - 2;
			break;
		case VOC_MORE_SOUND:
			break;
		case VOC_SILENCE:
			break;
		case VOC_MARKER:
			current = PeekWord(source + offset);
			break;
		case VOC_TEXT:
			break;
		case VOC_REPEAT:
			repeats = PeekWord(source + offset);
			break;
		case VOC_END_REPEAT:
			break;
		}
		if (marker <= current)
			break;
		else
			offset = blockEnd;
	}
	buffer = sound;
	active = 1;
	service();
}

}

extern "C" void ResetEndgameSpeechGlobals(void)
{
	Endgame::useSecond = 0;
}
