/* Black Gate U7.EXE, resident segment 68 (file offsets 0x0269f0 to 0x026c52, 610 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include <alloc.h>
#include "cflxbuf.h"
#include "init.h"
#include "sprite.h"
#include "debug.h"
#include "preload.h"
#include "specard.h"
#include "easyfile.h"
#include "usehook.h"
#include "uccomm2.h"
#include "voice.h"

unsigned char SpeechOn = 0;
unsigned char SpeechCardStarted = 0;
unsigned char SpeechCardConfigured = 0;
unsigned char SpeechStarted = 0;
unsigned char SpeechSpriteShown = 0;
int SpeechSprite = 0;
BorrowedSpeechCache SpeechStream;

void Speech::reportError(int code)
{
	FatalError("Voice Error: %04x\nFar Mem=%lu\n", code, farcoreleft());
}

int Speech::isPlaying()
{
	if (SpeechOn && SpeechCardStarted)
		return !SpeechFinished;
	return 0;
}

Speech::Speech(int size, int sampleRate, int basePort, int irqLine, int dataFormat)
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

int Speech::start()
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

void Speech::playFile(char *name, int track)
{
	play(BuildPath(StaticPath, name, 0), track);
}

void Speech::play(char *path, int track)
{
	if (SpeechOn && SpeechCardStarted) {
		stop();
		SpeechCard.resetPlayback();
		SpeechStream.playEntry(path, track, 0);
		SpeechCard.play(&SpeechStream);
		SpeechStarted = 1;
	} else {
		SpeechTrack = track;
		RunUsable(1, 0, 0x614);         /* shows the speech as text instead */
	}
}

int Speech::selectTrack(int track)
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

unsigned char IsSpeechPlaying(void)
{
	return SpeechPlayer.isPlaying();
}
