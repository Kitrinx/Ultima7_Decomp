/* Black Gate U7.EXE, resident segment 68 (file offsets 0x0269f0 to 0x026c52, 610 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "cflxbuf.h"
#include "init.h"
#include "sprite.h"
#include "debug.h"
#include "drv/specard.h"
#include "easyfile.h"
#include "usehook.h"
#include "voice.h"
#include "memapi.h"
#include <new>

extern int16_t SpeechTrack;

namespace BgSound {

Speech SpeechPlayer(3072, 10667, 0x220, 7, 3);
uint8_t SpeechCardStarted = 0;
uint8_t SpeechStarted = 0;
uint8_t SpeechSpriteShown = 0;
int16_t SpeechSprite = 0;
BorrowedSpeechCache SpeechStream;

void Speech::reportError(int16_t code)
{
	FatalError("Voice Error: %04x\nFar Mem=%" PRIu32 "\n", code, (uint32_t)GetFarHeapFree(0));
}

int16_t Speech::isPlaying()
{
	if (SpeechOn && SpeechCardStarted) {
		SpeechCard.feed();
		return !SpeechFinished;
	}
	return 0;
}

Speech::Speech(int16_t size, int16_t sampleRate, int16_t basePort, int16_t irqLine, int16_t dataFormat)
{
	bufferSize = size;
	rate = sampleRate;
	port = basePort;
	irq = irqLine;
	format = dataFormat;
}

Speech::~Speech()
{
	if (SpeechCardStarted)
		stop();
}

int16_t Speech::start()
{
	if (SpeechCardConfigured) {
		SpeechCard.init(bufferSize, rate, port, irq, dma);
		SpeechCardStarted = 1;
	}
	return 0;
}

void Speech::pause()
{
	if (SpeechCardStarted)
		SpeechCard.SoundBlaster::~SoundBlaster();
}

void Speech::stop()
{
	if (SpeechCardStarted) {
		SpeechCard.stop();
		SpeechStarted = 0;
		SpeechStream.releaseBuffer();
	}
	if (SpeechSpriteShown) {
		SpriteManager_stopSprite(&gSpriteManager, SpeechSprite);
		SpeechSpriteShown = 0;
	}
}

void Speech::playFile(char *name, int16_t track)
{
	play(BuildPath(StaticPath, name, 0), track);
}

void Speech::play(char *path, int16_t track)
{
	if (SpeechOn && SpeechCardStarted) {
		stop();
		if (!SpeechCard.playHq(track)) {
			SpeechCard.resetPlayback();
			SpeechStream.playEntry(path, track, 0);
			SpeechCard.play(&SpeechStream);
		}
		SpeechStarted = 1;
	} else {
		SpeechTrack = track;
		RunUsable(1, 0, 0x614);         /* shows the speech as text instead */
	}
}

int16_t Speech::selectTrack(int16_t track)
{
	SpeechTrack = track;
	if (SpeechOn && SpeechCardStarted)
		return 1;
	return 0;
}

void Speech::continuePlaying()
{
	if (SpeechOn && SpeechCardStarted && SpeechStarted) {
		if (!isPlaying())
			stop();
	}
}

void ContinuePlayingSpeech(void)
{
	SpeechPlayer.continuePlaying();
}

uint8_t IsSpeechPlaying(void)
{
	return SpeechPlayer.isPlaying();
}

/* Starts the card with the port, IRQ and DMA the engine's configuration gave its own player. */
int16_t StartSpeech(int16_t port, int16_t irq, int16_t dma)
{
	SpeechPlayer.port = port;
	SpeechPlayer.irq = irq;
	SpeechPlayer.dma = dma;
	return SpeechPlayer.start();
}

extern "C" void ResetBgSoundVoiceGlobals(void)
{
	memset((void *)&SpeechPlayer, 0, sizeof(SpeechPlayer));
	SpeechCardStarted = 0;
	SpeechStarted = 0;
	SpeechSpriteShown = 0;
	SpeechSprite = 0;
	memset((void *)&SpeechStream, 0, sizeof SpeechStream);
}

extern "C" void ConstructBgSoundVoiceGlobals(void)
{
	new (&SpeechPlayer) Speech(3072, 10667, 0x220, 7, 3);
	new (&SpeechStream) BorrowedSpeechCache();
}

}
