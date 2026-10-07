/* Serpent Isle MAINMENU.EXE, resident segment 48 (file offsets 0x015822 to 0x0160a9, 2183 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "dosio.h"
#include "init.h"
#include "errors.h"
#include "oops.h"
#include "memapi.h"
#include "specard.h"
#include "cflxbuf.h"
#include "plat.h"
#include <new>

namespace Shared {

uint16_t SpeechRate = 0;
uint16_t DspTimeConstant = 0;
int16_t CardPort = 0;
uint16_t CardIrq = 7;
uint16_t DmaHalfSize = 0;               /* bytes in each half of the buffer */
char *DmaBuffer = 0;
uint8_t DmaNextHalf = 0;          /* which half is refilled next */
uint16_t DmaFirstCount = 0;
uint16_t DmaSecondCount = 0;

int32_t QueuedBlock = 0;
uint16_t QueuedBlockSize = 0;
uint8_t BlockConsumed = 0;
uint8_t SpeechFinished = 1;
uint16_t DmaBlockCount = 0;
uint8_t SpeechStreaming = 1;
SpeechCache *CurrentSpeech = 0;

uint16_t DmaChannel = 1;
const char CardErrorFormat[] = "%s line#%d";

SoundBlaster SpeechCard;

/* The halves go to the host's output queue instead of a DMA channel. Draining: the last half has
 * gone out, and the speech ends once the host has played it. Samples the queue had no room for
 * wait in the spill area after the two halves. */
static uint8_t SpeechRunning = 0;
static uint8_t SpeechDraining = 0;
static uint8_t *Spill = 0;
static int32_t SpillStart = 0;
static int32_t SpillCount = 0;

static void QueueSamples(const uint8_t *samples, int32_t count)
{
	int32_t taken = plat_pcm_queue(samples, count);

	SpillStart = 0;
	SpillCount = count - taken;
	if (SpillCount > 0)
		memmove(Spill, samples + taken, SpillCount);
}

/* The rate the card played at; it kept only the low byte of the time constant. */
static int32_t CardRate(void)
{
	return INT32_C(1000000) / (256 - (uint8_t)DspTimeConstant);
}

static void FeedSpeech(void)
{
	SpeechCard.feed();
}

void SoundBlaster::init(uint16_t size, uint16_t rate, int16_t port, uint16_t irq, uint16_t dma)
{
	if (!ready) {
		DmaHalfSize = size;
		SpeechRate = rate;
		CardPort = port;
		CardIrq = irq;
		DmaChannel = dma;
		DspTimeConstant = 256 - INT32_C(1000000) / SpeechRate;
		allocateDmaBuffer();
		plat_game_timer_add(FeedSpeech);
		ready = 1;
	}
}

void SoundBlaster::resetPlayback()
{
	DmaNextHalf = 0;
	SpeechFinished = 0;
	DmaBlockCount = 0;
	BlockConsumed = 0;
}

SoundBlaster::~SoundBlaster()
{
	shutdown();
	if (DmaBuffer)
		FreeFarHeap(DmaBuffer);
	ready = 0;
}

void SoundBlaster::setRate(uint16_t rate)
{
	if (rate != 0)
		DspTimeConstant = 256 - INT32_C(1000000) / rate;
}

void SoundBlaster::setTimeConstant(int16_t constant)
{
	if (constant != 0)
		DspTimeConstant = constant;
}

/* Both halves, then the spill area. */
void SoundBlaster::allocateDmaBuffer()
{
	DmaBuffer = (char *)AllocateFarHeap(DmaHalfSize * 3, 0);
	if (!DmaBuffer)
		ReportOutOfFarMemory();
	Spill = (uint8_t *)DmaBuffer + DmaHalfSize * 2;
}

void SoundBlaster::shutdown()
{
	if (ready != 0) {
		stop();
		plat_game_timer_remove(FeedSpeech);
	}
}

void SoundBlaster::stop()
{
	SpeechRunning = 0;
	plat_pcm_stop();
}

/* Start playing a sound from the first half of the buffer. */
void SoundBlaster::play(SpeechCache *sound)
{
	if (!sound->isPrimed())
		return;
	setRate(sound->getRate());
	SpeechStreaming = sound->isStreaming();
	CurrentSpeech = sound;
	DmaBlockCount = 0;
	SpeechFinished = 0;
	sound->fillDoubleBuffer(DmaBuffer, &DmaFirstCount, &DmaSecondCount);
	plat_pcm_start(CardRate());
	SpeechDraining = 0;
	QueueSamples((uint8_t *)DmaBuffer, DmaFirstCount);
	SpeechRunning = 1;
	feed();
}

/* Keeps about two seconds queued ahead; the card's interrupt did this at the end of each half. */
void SoundBlaster::feed()
{
	int32_t taken;

	if (!SpeechRunning)
		return;
	if (SpillCount > 0) {
		taken = plat_pcm_queue(Spill + SpillStart, SpillCount);
		SpillStart += taken;
		SpillCount -= taken;
	}
	/* A streamed sound waits for its player to queue the next block before another half goes. */
	while (SpillCount == 0 && !SpeechDraining && !(SpeechStreaming == 1 && BlockConsumed)
		&& plat_pcm_pending() < 2 * CardRate())
		OnDmaDone();
	if (SpeechDraining && SpillCount == 0 && plat_pcm_pending() == 0) {
		SpeechRunning = 0;
		CurrentSpeech->stop();
		SpeechFinished = 1;
	}
}

/* A half has played: send the other, already filled, and refill this one. */
void OnDmaDone(void)
{
	uint16_t count;
	uint16_t length, fill;
	char *half;

	if (QueuedBlockSize == 0) {
		SpeechDraining = 1;
		return;
	}
	if (DmaNextHalf == 0) {
		if (DmaFirstCount < DmaHalfSize) {
			BlockConsumed = 0;
			SpeechDraining = 1;
			return;
		}
		half = DmaBuffer + DmaHalfSize;
		count = DmaSecondCount;
		fill = 0;
	} else {
		if (DmaSecondCount < DmaHalfSize) {
			BlockConsumed = 0;
			SpeechDraining = 1;
			return;
		}
		half = DmaBuffer;
		count = DmaFirstCount;
		fill = DmaHalfSize;
	}
	QueueSamples((uint8_t *)half, count);
	if (SpeechStreaming == 1) {
		MoveLinear(PointerToLinear(DmaBuffer), QueuedBlock, (uint32_t) QueuedBlockSize, 0x111);
		if (QueuedBlockSize < DmaHalfSize)
			--QueuedBlockSize;
		if (DmaNextHalf == 0) {
			DmaFirstCount = QueuedBlockSize;
			DmaNextHalf = 1;
		} else {
			DmaSecondCount = QueuedBlockSize;
			DmaNextHalf = 0;
		}
		BlockConsumed = 1;
	} else {
		length = CurrentSpeech->readBytes(DmaBuffer + fill, DmaHalfSize);
		if (length != 0 && length < DmaHalfSize)
			--length;
		if (DmaNextHalf == 0) {
			DmaFirstCount = length;
			DmaNextHalf = 1;
		} else {
			DmaSecondCount = length;
			DmaNextHalf = 0;
		}
	}
}

void SoundBlaster::queueBlock(int32_t source, uint16_t count)
{
	QueuedBlock = source;
	QueuedBlockSize = count;
}

void SoundBlaster::fail(char *message)
{
	shutdown();
	FatalError(message);
}

}

extern "C" void ResetSpecardGlobals(void)
{
	using namespace Shared;

	SpeechRate = 0;
	DspTimeConstant = 0;
	CardPort = 0;
	CardIrq = 7;
	DmaHalfSize = 0;
	DmaBuffer = 0;
	DmaNextHalf = 0;
	DmaFirstCount = 0;
	DmaSecondCount = 0;
	QueuedBlock = 0;
	QueuedBlockSize = 0;
	BlockConsumed = 0;
	SpeechFinished = 1;
	DmaBlockCount = 0;
	SpeechStreaming = 1;
	CurrentSpeech = 0;
	DmaChannel = 1;
	SpeechRunning = 0;
	SpeechDraining = 0;
	Spill = 0;
	SpillStart = 0;
	SpillCount = 0;
}

extern "C" void ConstructSpecardGlobals(void)
{
	using namespace Shared;

	new (&SpeechCard) SoundBlaster();
}
