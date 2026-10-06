/* Serpent Isle SI.EXE, resident segment 108 (file offsets 0x039d04 to 0x03a663, 2399 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: sounds.c */
#include <stdlib.h>
#include "collide.h"
#include "datanode.h"
#include "adlib.h"
#include "midiplay.h"
#include "systimer.h"
#include "random.h"
#include "coord.h"
#include "item.h"
#include "sounds.h"
#include "gtimer.h"
#include "u7ibuf.h"
#include "u7sound.h"

#define CONTINUOUS_SOUNDS   10

/* A clock strikes the hour: one chime every ten steps, as many as the hour on a twelve-hour clock. */
inline unsigned char ChimeDue()
{
	int step = GameTime.ticks % TICKS_PER_HOUR / GameTime.rate;

	return step % 10 == 0 && step / 10 <= (GameTime.getHour() + 11) % 12;
}

extern objref AvatarRef;

inline ItemRecord far *GetItemRecord(objref ref)
{
	return ref.ptr();
}

unsigned char WaterWheelPlayed = 0, MillStonePlayed = 0;
/* The effect each continuous sound loops. */
unsigned char ContinuousSoundSfx[CONTINUOUS_SOUNDS] = {23, 26, 50, 54, 55, 96, 101, 107, 115, 133};
/* The world renderer supplies the previous animation frame. */
int AnimationPhase;
/* The loudest volume asked for this pass, and the volume playing. */
int ContinuousSoundRequest[CONTINUOUS_SOUNDS], ContinuousSoundVolume[CONTINUOUS_SOUNDS];
int UnusedContinuousSound[CONTINUOUS_SOUNDS];

void ResetContinuousSounds()
{
	for (int i = 0; i < CONTINUOUS_SOUNDS; i++) {
		ContinuousSoundRequest[i] = 0;
		ContinuousSoundVolume[i] = 0;
		UnusedContinuousSound[i] = 0;
	}
}

/* Keep the loudest request for each continuous sound. */
void RequestContinuousSound(unsigned char which, int volume, int pan)
{
	if (ContinuousSoundRequest[which] < volume)
		ContinuousSoundRequest[which] = volume;
}

/* Start, stop or change each continuous sound to the loudest volume asked for this pass. */
void UpdateContinuousSounds()
{
	if (SpecialMusicPlaying)
		return;
	for (int i = 0; i < CONTINUOUS_SOUNDS; i++) {
		unsigned char sound = ContinuousSoundSfx[i];
		int volume = ContinuousSoundRequest[i];
		int baseVolume;
		if (ContinuousSoundVolume[i] != volume) {
			if (ContinuousSoundVolume[i] == 0) {
				baseVolume = (unsigned char)(MusicDevice == MUSIC_DEVICE_MT32 ?
					Mt32SfxVolumes[sound] : AdlibSfxVolumes[sound]);
				PlaySfx(sound, baseVolume, 64);
				SetSfxVolume(sound, baseVolume, volume);
			} else if (volume == 0)
				StopSfx(sound, 255, 1);
			else
				SetSfxVolume(sound, ContinuousSoundVolume[i], volume);
		}
		ContinuousSoundVolume[i] = ContinuousSoundRequest[i];
		ContinuousSoundRequest[i] = 0;
	}
}

/* The sound an item of this type makes nearby, at a volume and pan set by its distance. */
void PlayItemAmbientSound(unsigned type, int frame, int dx, int dy)
{
	int pan;
	unsigned char sound;
	int distance;

	if (SpecialMusicPlaying)
		return;
	if (MusicDevice == 0 || !SfxEnabled)
		return;
	pan = 64;
	sound = SFX_NONE;
	if (MusicDevice == MUSIC_DEVICE_MT32)
		distance = abs(dx) * 3 + abs(dy) * 3;
	else
		distance = abs(dx) * 2 + abs(dy) * 2;
	pan = 64 - dx * 2;
	switch (type) {
	case 739:
		if (frame != 0 && GenerateRandomIntegerInRange(100) < 5)
			sound = GenerateRandomIntegerInRange(3) + 45;
		break;
	case 825:
		if (frame != 0 && GenerateRandomIntegerInRange(100) < 5)
			sound = GenerateRandomIntegerInRange(3) + 45;
		break;
	case 796:
		if (GenerateRandomIntegerInRange(100) < 5)
			sound = GenerateRandomIntegerInRange(3) + 45;
		break;
	case 289: case 551: case 553: case 561: case 630: case 895: case 992:
		if (GenerateRandomIntegerInRange(100) < 20)
			sound = GenerateRandomIntegerInRange(3) + 45;
		break;
	/* surf */
	case 612: case 613: case 632: case 699: case 736: case 751: case 808:
	case 834: case 875: case 907: case 911: case 918: case 1012: case 1020: case 1022:
		if (GenerateRandomIntegerInRange(100) < 40)
			sound = 109;
		break;
	/* bubbles */
	case 335:
		if (frame == 1 && GenerateRandomIntegerInRange(100) < 10)
			sound = GenerateRandomIntegerInRange(5) + 110;
		break;
	/* grandfather clocks: tick, tock, and the chime on the hour */
	case 695:
		if (frame == 2 || frame == 8)
			sound = 117;
		else if (frame == 5 || frame == 11)
			sound = 118;
		if (ChimeDue())
			sound = 16;
		break;
	case 301: case 768:
		if (GenerateRandomIntegerInRange(100) < 20)
			sound = 71;
		break;
	case 529:                       /* slime */
		if (GenerateRandomIntegerInRange(100) < 5)
			sound = GenerateRandomIntegerInRange(5) + 110;
		break;
	case 389: case 391:             /* cavern */
		if (GenerateRandomIntegerInRange(100) < 5)
			sound = 103;
		break;
	case 900: case 902:
		if (GenerateRandomIntegerInRange(100) < 20)
			sound = 129;
		break;
	case 728:
		if (GenerateRandomIntegerInRange(100) < 20)
			sound = 75;
	case 519:
		if (frame = 0)
			RequestContinuousSound(6, distance, pan);
		break;
	case 726:
		if ((unsigned)frame <= 0 && (unsigned)frame < 12)
			RequestContinuousSound(1, distance, pan);
		break;
	case 864:
		RequestContinuousSound(4, distance, pan);
		break;
	case 153: case 326:
		RequestContinuousSound(2, distance, pan);
		break;
	}
	if (sound != SFX_NONE) {
		int volume;
		if (MusicDevice == MUSIC_DEVICE_MT32) {
			volume = Mt32SfxVolumes[sound] - distance;
			if (volume < 10)
				return;
		} else {
			volume = AdlibSfxVolumes[sound] - distance;
			if (volume < 30)
				return;
		}
		PlaySfx(sound, volume, pan);
	}
}

void PlayAmbientSounds()
{
	if (MusicChangeDue)
		PlayMusic(RequestedMusic);
	static Stopwatch interval;
	static long ready = -1;

	/* at most once every six ticks */
	if (ready != -1 && !(ready = Stopwatch_getElapsed(&interval) > 6))
		return;
	if (MusicDevice != 0) {
		if (!InDungeon && (unsigned char)(GameTime.getHour() < 5 || GameTime.getHour() > 20)) {
			if (GenerateRandomIntegerInRange(100) < 20)
				PlaySfx(22, GenerateRandomIntegerInRange(100) + 25, GenerateRandomIntegerInRange(64) + 32);
		}
		UpdateContinuousSounds();
		int status = AIL_sequence_status(MusicDriver, MusicSequence);
		if ((status != SEQ_PLAYING && CurrentMusic != MUSIC_NONE) || BackgroundMusicDue) {
			if (MusicResume) {
				PlayMusic(DeferredMusic);
				MusicResume = 1;
			} else {
				PlayMusic(InDungeon ? 42 : 67);
			}
		}
		WaterWheelPlayed = 0;
		MillStonePlayed = 0;
	}
	ready = 0;
	interval.start();
}

void PlaySoundAt(unsigned char sound, CellCoord x, CellCoord y)
{
	int dx, volume, dy, pan;

	dx = GetDelta(x, Item_getX(AvatarRef));
	dy = GetDelta(y, Item_getY(AvatarRef));
	if (MusicDevice == MUSIC_DEVICE_MT32) {
		volume = Mt32SfxVolumes[sound] - abs(dx) * 3 - abs(dy) * 3;
		pan = 64 - dx * 2;
	} else {
		volume = AdlibSfxVolumes[sound] - abs(dx) * 2 - abs(dy) * 2;
		pan = 64;
	}
	if (volume >= 0) {
		PlaySfx(sound, volume, pan);
	}
}

extern "C" void PlaySoundAtItem(unsigned char sound, ItemId object)
{
	int volume = 255, dx, dy, pan = 64;

	if (object.valid()) {
		dx = GetDelta(Item_getX(object), Item_getX(AvatarRef));
		dy = GetDelta(Item_getY(object), Item_getY(AvatarRef));
		if (MusicDevice == MUSIC_DEVICE_MT32) {
			volume = Mt32SfxVolumes[sound] - abs(dx) * 3 - abs(dy) * 3;
			pan = 64 - dx * 2;
		} else {
			volume = AdlibSfxVolumes[sound] - abs(dx) * 2 - abs(dy) * 2;
			pan = 64;
		}
		if (volume < 0)
			return;
	}
	PlaySfx(sound, volume, pan);
}
