/* Serpent Isle SI.EXE, overlay segment 341 (file offsets 0x09f020 to 0x09f82b, 2059 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -Y rebuilds it byte for byte as C++.
 */

/* path: voice.c */
#include "objref.h"
#include "itemrec.h"
#include "iteminfo.h"
#include "init.h"
#include "debug.h"
#include "easyfile.h"
#include "memapi.h"
#include "flex.h"
#include "ail.h"
#include "cflxbuf.h"
#include "wihh.h"
#include "sprite.h"
#include "camera.h"
#include "worldpal.h"
#include "usehook.h"
#include "preload.h"
#include "voice.h"

extern objref AvatarRef;
extern int SpeechTrack;

inline unsigned char Differs(const unsigned &a, const unsigned &b) { return a != b; }

unsigned char SpeechOn = 0;
unsigned char SpeechCardStarted = 0;
unsigned char SpeechCardConfigured = 0;
unsigned char AlternateSpeechDriver = 0;
unsigned char SpeechStarted = 0;
unsigned char SpeechSpriteShown = 0;
int SpeechSprite = 0;
BorrowedSpeechCache SpeechStream;
static int SecondBuffer = 0;

int Speech::isPlaying()
{
	if (SpeechOn && SpeechCardStarted) {
		if (next.len == 0 && AIL_sound_buffer_status(driver, 0) == DAC_DONE &&
				AIL_sound_buffer_status(driver, 1) == DAC_DONE)
			return 0;
		return 1;
	}
	return 0;
}

int Speech::isFinished()
{
	if (SpeechCardStarted) {
		if (next.len == 0 && AIL_sound_buffer_status(driver, 0) == DAC_DONE &&
				AIL_sound_buffer_status(driver, 1) == DAC_DONE)
			return 1;
		return 0;
	}
	return 1;
}

Speech::Speech(int size, int sampleRate, int basePort, int irqLine, int dataFormat)
{
	bufferSize = size;
	rate = sampleRate;
	port = basePort;
	irq = irqLine;
	format = dataFormat;
	buffers[0] = 0;
	buffers[1] = 0;
}

Speech::~Speech()
{
	if (SpeechCardStarted) {
		stop();
		pause();
	}
}

int Speech::start()
{
	long size;
	int record;

	if (SpeechCardConfigured) {
		if (!drivers.open("static\\snddrvrs.dat"))
			AssertFail(__FILE__, 229);
		size = 0;
		record = 0;
		if (AlternateSpeechDriver) {
			size = 4936;
			record = 3;
		} else {
			size = 4657;
			record = 2;
		}
		driverImage = AllocateFarHeap(size, 2);
		if (driverImage == 0)
			AssertFail(__FILE__, 257);
		if (!drivers.readRecord(record, driverImage, 0))
			AssertFail(__FILE__, 262);
		driver = AIL_register_driver(driverImage);
		if (driver == -1)
			AssertFail(__FILE__, 269);
		description = AIL_describe_driver(driver);
		if (description->drvr_type != DSP_DRVR)
			AssertFail(__FILE__, 275);
		if (!AIL_detect_device(driver, port, irq, dma, dma))
			FatalError("\nSerpent Isle could not detect a sound\ncard at the IO/IRQ/DMA values selected.\n");
		AIL_init_driver(driver, port, irq, dma, dma);
		SpeechStream.playEntry(0, 0, format);
		buffers[0] = AllocateFarHeap(512L, 0);
		if (buffers[0] == 0)
			AssertFail(__FILE__, 295);
		buffers[1] = AllocateFarHeap(512L, 0);
		if (buffers[1] == 0)
			AssertFail(__FILE__, 301);
		SpeechCardStarted = 1;
	}
	return 0;
}

void Speech::pause()
{
	if (SpeechCardStarted) {
		AIL_shutdown_driver(driver, 0);
		AIL_release_driver_handle(driver);
		FreeFarHeap(driverImage);
	}
}

void Speech::stop()
{
	if (SpeechCardStarted) {
		AIL_stop_digital_playback(driver);
		SpeechStarted = 0;
	}
	if (SpeechSpriteShown) {
		SpriteManager_stopSprite(&gSpriteManager, SpeechSprite);
		SpeechSpriteShown = 0;
	}
}

void Speech::playFile(char *name, int track)
{
	if (SpeechTrack < 21 && SpeechTrack != 3) {
		unsigned held = objref(GetItemInSlot(AvatarRef, 9)).type();
		if (Differs(635, held))
			return;
	}
	play(BuildPath(StaticPath, name, 0), track);
}

void Speech::play(char *path, int track)
{
	int face = 295;

	if (SpeechOn && SpeechCardStarted) {
		stop();
		if (SpeechTrack < 21) {
			face = 300;
			int hand = objref(GetItemInSlot(AvatarRef, 8)).type();
			int other = objref(GetItemInSlot(AvatarRef, 7)).type();
			if (hand == 887) {
				int frame = objref(GetItemInSlot(AvatarRef, 8)).frame();
				if (frame == 1)
					face = 295;
			}
			if (other == 887) {
				int frame = objref(GetItemInSlot(AvatarRef, 7)).frame();
				if (frame == 1)
					face = 295;
			}
		} else {
			switch (SpeechTrack) {
			case 21:
			case 22:
				face = 296;
				break;
			case 23:
			case 24:
				face = 256;
				break;
			case 25:
				face = 293;
				break;
			case 26:
				face = 294;
				break;
			}
		}
		int dx = -(AvatarRef.z() * 4);
		int dy = dx;
		SpeechSprite = SpriteManager_playSpriteForItem(&gSpriteManager, AvatarRef, dx + 160, dy + 100, 0, 0,
			face + 1098, 0, 32767, 5);
		SpeechSpriteShown = 1;
		DrawWorld(&gCamera);
		SpeechStream.playEntry(path, track, 0);
		SpeechStream.load();
		next.sample_rate = 0x8e;
		next.pack_type = 0;
		next.len = SpeechStream.remaining;
		block = next;
		SpeechStarted = 1;
		GameScreen.updateLight();
		CopyFrameBuffer();
		while (SpeechStarted)
			continuePlaying();
	} else {
		GameScreen.updateLight();
		CopyFrameBuffer();
		SpeechTrack = track;
		RunUsable(1, 0, 0x614);
	}
	GameScreen.clearLight();
}

void Speech::continuePlaying()
{
	long got;
	int i;

	if (SpeechOn && SpeechCardStarted && SpeechStarted) {
		for (i = 0; i < 2; i++) {
			if (AIL_sound_buffer_status(driver, i) == DAC_DONE && next.len != 0) {
				block.len = next.len > 512 ? 512 : next.len;
				next.len -= block.len;
				if ((SecondBuffer ^= 1) == 0) {
					got = SpeechStream.read(buffers[0], block.len);
					block.data = buffers[0];
				} else {
					got = SpeechStream.read(buffers[1], block.len);
					block.data = buffers[1];
				}
				AIL_register_sound_buffer(driver, i, &block);
			}
		}
		AIL_start_digital_playback(driver);
		if (isFinished()) {
			SpeechStream.close();
			SpeechStream.freeBuffer();
			stop();
		}
	}
}

int Speech::selectTrack(int track)
{
	SpeechTrack = track;
	return SpeechOn && SpeechCardStarted;
}

void ContinuePlayingSpeech(void)
{
	SpeechPlayer.continuePlaying();
}

unsigned char IsSpeechPlaying(void)
{
	return SpeechPlayer.isPlaying();
}
