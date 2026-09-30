/* Black Gate ENDGAME.EXE: speech.c and digital.c. The sound card and its driver are the platform's
 * speech output; a clock tick keeps it fed, as the card's interrupt did.
 */

#include "u7port.h"
#include "plat.h"
#include "speech.h"

namespace Endgame {

/* Creative Voice file header and block types. */
#define VOC_FIRST_BLOCK     0x14
#define VOC_CHECK           0x18
#define VOC_CHECK_VALUE     0x1129
#define VOC_END             0
#define VOC_SOUND           1
#define VOC_MORE_SOUND      2
#define VOC_MARKER          4

static SpeechStream *Playing = 0;

static uint16_t PeekWord(const uint8_t *p) { return (uint16_t) (p[0] | p[1] << 8); }

static void FeedSpeech(void)
{
	if (Playing)
		Playing->service();
}

SpeechStream::SpeechStream(uint8_t *voc, int32_t size)
{
	source = voc;
	length = size;
	active = 0;
	type = 0;
	left = 0;
}

SpeechStream::~SpeechStream()
{
	stopPlayback();
}

/* Reads a block's three-byte length. */
void SpeechStream::readHeader()
{
	blockSize = PeekWord(source + offset) | (int32_t) source[offset + 2] << 16;
	offset += 3;
	blockEnd = offset + blockSize;
}

/* Moves on to the next block of sound; a marker or the end stops it. */
uint8_t SpeechStream::nextBlock()
{
	offset = blockEnd;
	while (offset < length && (type = source[offset++]) != VOC_END) {
		readHeader();
		switch (type) {
		case VOC_SOUND:
			offset += 2;
			left = blockSize - 2;
			return 1;
		case VOC_MORE_SOUND:
			left = blockSize;
			return 1;
		case VOC_MARKER:
			return 0;
		}
		offset = blockEnd;
	}
	return 0;
}

/* Finds the first sound block at or after marker and starts playing from there. */
void SpeechStream::play(int16_t marker)
{
	int16_t current = -1;
	uint8_t timeConstant = 0;

	if (PeekWord(source + VOC_CHECK) != VOC_CHECK_VALUE)
		return;
	offset = PeekWord(source + VOC_FIRST_BLOCK);
	while ((type = source[offset++]) != VOC_END) {
		readHeader();
		switch (type) {
		case VOC_SOUND:
			timeConstant = source[offset];
			offset += 2;
			left = blockSize - 2;
			break;
		case VOC_MARKER:
			current = (int16_t) PeekWord(source + offset);
			break;
		}
		if (marker <= current)
			break;
		offset = blockEnd;
	}
	/* The card played 1000000 / (256 - time constant) samples a second. */
	plat_pcm_start(INT32_C(1000000) / (256 - timeConstant));
	active = 1;
	Playing = this;
	plat_game_timer_add(FeedSpeech);
	service();
}

/* Queues what the output has room for, block after block. */
void SpeechStream::service()
{
	int32_t taken;

	if (!active)
		return;
	for (;;) {
		if (left == 0 && (type == VOC_END || !nextBlock())) {
			type = VOC_END;
			left = 0;
			return;
		}
		taken = plat_pcm_queue(source + offset, left);
		offset += taken;
		left -= taken;
		if (left > 0)
			return;
	}
}

uint8_t SpeechStream::isDone()
{
	if (!active)
		return 1;
	service();
	return type == VOC_END && left == 0 && plat_pcm_pending() == 0;
}

void SpeechStream::stopPlayback()
{
	if (Playing == this) {
		plat_game_timer_remove(FeedSpeech);
		Playing = 0;
		plat_pcm_stop();
	}
	active = 0;
}

}

extern "C" void ResetEndgameSpeechGlobals(void)
{
	Endgame::Playing = 0;
}
