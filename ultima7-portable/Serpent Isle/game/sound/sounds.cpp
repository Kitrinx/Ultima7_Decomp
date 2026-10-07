/* Serpent Isle SI.EXE, resident segment 108 (file offsets 0x039d04 to 0x03a663, 2399 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: sounds.c */
#include "u7port.h"
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
#include <new>

#define CONTINUOUS_SOUNDS   10

/* A clock strikes the hour: one chime every ten steps, as many as the hour on a twelve-hour clock. */
inline uint8_t ChimeDue()
{
	int16_t step = GameTime.ticks % TICKS_PER_HOUR / GameTime.rate;

	return step % 10 == 0 && (uint16_t)(step / 10) <= (GameTime.getHour() + 11) % 12;
}

extern objref AvatarRef;

inline ItemRecord *GetItemRecord(objref ref)
{
	return ref.ptr();
}

uint8_t WaterWheelPlayed = 0, MillStonePlayed = 0;
extern "C" uint8_t QuietWeapons = 0;
/* The effect each continuous sound loops. */
const uint8_t ContinuousSoundSfx[CONTINUOUS_SOUNDS] = {23, 26, 50, 54, 55, 96, 101, 107, 115, 133};
/* The world renderer supplies the previous animation frame. */
int16_t AnimationPhase;
/* The loudest volume asked for this pass, and the volume playing. */
int16_t ContinuousSoundRequest[CONTINUOUS_SOUNDS], ContinuousSoundVolume[CONTINUOUS_SOUNDS];
int16_t UnusedContinuousSound[CONTINUOUS_SOUNDS];

void ResetContinuousSounds()
{
	for (int16_t i = 0; i < CONTINUOUS_SOUNDS; i++) {
		ContinuousSoundRequest[i] = 0;
		ContinuousSoundVolume[i] = 0;
		UnusedContinuousSound[i] = 0;
	}
}

/* Keep the loudest request for each continuous sound. */
void RequestContinuousSound(uint8_t which, int16_t volume, int16_t pan)
{
	if (ContinuousSoundRequest[which] < volume)
		ContinuousSoundRequest[which] = volume;
}

/* Start, stop or change each continuous sound to the loudest volume asked for this pass. */
void UpdateContinuousSounds()
{
	if (SpecialMusicPlaying)
		return;
	for (int16_t i = 0; i < CONTINUOUS_SOUNDS; i++) {
		uint8_t sound = ContinuousSoundSfx[i];
		int16_t volume = ContinuousSoundRequest[i];
		int16_t baseVolume;
		if (ContinuousSoundVolume[i] != volume) {
			if (ContinuousSoundVolume[i] == 0) {
				baseVolume = (uint8_t)(MusicDevice == MUSIC_DEVICE_MT32 ?
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
void PlayItemAmbientSound(uint16_t type, int16_t frame, int16_t dx, int16_t dy)
{
	int16_t pan;
	uint8_t sound;
	int16_t distance;

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
		if ((type == 551 || type == 553) && QuietWeapons)
			break;
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
		if ((uint16_t)frame <= 0 && (uint16_t)frame < 12)
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
		int16_t volume;
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

static Stopwatch interval;
static int32_t ready = -1;

void PlayAmbientSounds()
{
	if (MusicChangeDue)
		PlayMusic(RequestedMusic);

	/* at most once every six ticks */
	if (ready != -1 && !(ready = Stopwatch_getElapsed(&interval) > 6))
		return;
	if (MusicDevice != 0) {
		if (!InDungeon && (uint8_t)(GameTime.getHour() < 5 || GameTime.getHour() > 20)) {
			if (GenerateRandomIntegerInRange(100) < 20)
				PlaySfx(22, GenerateRandomIntegerInRange(100) + 25, GenerateRandomIntegerInRange(64) + 32);
		}
		UpdateContinuousSounds();
		int16_t status = AIL_sequence_status(MusicDriver, MusicSequence);
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

void PlaySoundAt(uint8_t sound, CellCoord x, CellCoord y)
{
	int16_t dx, volume, dy, pan;

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

extern "C" void PlaySoundAtItem(uint8_t sound, ItemId object)
{
	int16_t volume = 255, dx, dy, pan = 64;

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

extern "C" void ResetSoundsGlobals(void)
{
	WaterWheelPlayed = 0;
	MillStonePlayed = 0;
	QuietWeapons = 0;
	AnimationPhase = 0;
	memset(ContinuousSoundRequest, 0, sizeof ContinuousSoundRequest);
	memset(ContinuousSoundVolume, 0, sizeof ContinuousSoundVolume);
	memset(UnusedContinuousSound, 0, sizeof UnusedContinuousSound);
	memset(&interval, 0, sizeof interval);
	ready = -1;
}

extern "C" void ConstructSoundsGlobals(void)
{
	new (&interval) Stopwatch();
}
