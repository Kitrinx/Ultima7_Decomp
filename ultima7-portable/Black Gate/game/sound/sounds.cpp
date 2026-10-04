/* Black Gate U7.EXE, resident segment 47 (file offsets 0x01f504 to 0x01ff6b, 2663 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

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

#define CONTINUOUS_SOUNDS   12

struct Stopwatch {
	int8_t running;
	uint32_t start, total;
	Stopwatch() { total = start = 0; }
	void restart() { total = 0; start = TickCount; running = 1; }
};

/* A clock strikes the hour: one chime every ten steps, as many as the hour on a twelve-hour clock. */
inline uint8_t ChimeDue()
{
	int16_t step = GameTime.ticks % TICKS_PER_HOUR / GameTime.rate;

	return step % 10 == 0 && (uint16_t)(step / 10) <= (GameTime.getHour() + 11) % 12;
}

extern objref AvatarRef;
extern "C" void PlaySfx(uint8_t number, int16_t volume, int16_t pan, int16_t flags);

inline ItemRecord *GetItemRecord(objref ref)
{
	return ref.ptr();
}

uint8_t WaterWheelPlayed = 0, MillStonePlayed = 0;
extern "C" uint8_t QuietWeapons = 0;
/* The effect each continuous sound loops. */
const uint8_t ContinuousSoundSfx[CONTINUOUS_SOUNDS] = {48, 50, 77, 78, 82, 80, 81, 114, 25, 52, 79, 113};
/* The world renderer supplies the previous animation frame. */
int16_t AnimationPhase;
/* The loudest volume asked for this pass, and the volume playing. */
int16_t ContinuousSoundRequest[CONTINUOUS_SOUNDS], ContinuousSoundVolume[CONTINUOUS_SOUNDS];

void ResetContinuousSounds()
{
	for (int16_t i = 0; i < CONTINUOUS_SOUNDS; i++) {
		ContinuousSoundRequest[i] = 0;
		ContinuousSoundVolume[i] = 0;
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
	for (int16_t i = 0; i < CONTINUOUS_SOUNDS; i++) {
		uint8_t sound = ContinuousSoundSfx[i];
		int16_t volume = ContinuousSoundRequest[i];
		SoundChannel *playing;
		/* an Adlib effect that lost its channel or stopped holding its note starts again */
		if (ContinuousSoundVolume[i] != 0 && AdlibSfxActive != 0) {
			playing = FindSfxChannel(sound);
			if (playing == 0) {
				StopSfx(sound);
				ContinuousSoundVolume[i] = 0;
			}
			if (playing != 0 && playing->active() && GetAdlibVoiceState(playing->playback) != 2) {
				StopSfx(sound);
				ContinuousSoundVolume[i] = 0;
			}
		}
		if (volume > 127)
			volume = 127;
		if (ContinuousSoundVolume[i] != volume) {
			if (ContinuousSoundVolume[i] == 0)
				PlaySfx(sound, volume, 64, 0);
			else if (volume == 0)
				StopSfx(sound);
			else
				SetSfxVolume(sound, volume);
		}
		ContinuousSoundVolume[i] = volume;
		ContinuousSoundRequest[i] = 0;
	}
}

/* The sound an item of this type makes nearby, at a volume and pan set by its distance. */
void PlayItemAmbientSound(uint16_t type, int16_t frame, int16_t dx, int16_t dy)
{
	int16_t volume;
	int16_t pan = 64;

	if (MusicDevice == 0)
		return;
	if (MusicDevice == MUSIC_DEVICE_MT32)
		volume = 127 - (abs(dy) * 3 + abs(dx) * 3);
	else
		volume = 127 - (abs(dy) * 2 + abs(dx) * 2);
	if (volume < 0)
		return;
	pan = 64 - dx * 2;
	switch (type) {
	/* fire: burning weapons, oil and fields, firepits and campfires */
	case 551: case 553: case 630: case 739: case 782: case 825: case 895:
		if (type == 739 && frame == 0)
			break;
		if (type == 825 && frame == 8)
			break;
		if ((type == 551 || type == 553) && QuietWeapons)
			break;
		if (GenerateRandomIntegerInRange(100) < 60)
			PlaySfx(GenerateRandomIntegerInRange(3) + 20, volume, pan, 2);
		break;
	/* surf */
	case 612: case 613: case 632: case 699: case 736: case 751: case 808:
	case 834: case 875: case 907: case 911: case 918: case 1012: case 1020: case 1022:
		if (GenerateRandomIntegerInRange(100) < 1)
			PlaySfx(49, volume, pan, 0);
		break;
	/* bubbles */
	case 334: case 335:
		if (frame == 1)
			PlaySfx(GenerateRandomIntegerInRange(5) + 54, volume, pan, 0);
		break;
	/* grandfather clocks: tick, tock, and the chime on the hour */
	case 695: case 252:
		if (frame == 0 && AnimationPhase == 0)
			PlaySfx(17, volume, pan, 0);
		else if (frame == 1 && AnimationPhase == 2)
			PlaySfx(18, volume, pan, 0);
		if (ChimeDue())
			PlaySfx(19, volume, pan, 0);
		break;
	case 893:                       /* pool of water */
		RequestContinuousSound(0, volume, pan);
		break;
	case 794:                       /* water */
		RequestContinuousSound(1, volume, pan);
		break;
	case 157: case 776: case 777: case 779:     /* moongates */
		RequestContinuousSound(2, volume, pan);
		break;
	case 305:                       /* the Guardian's portal */
		RequestContinuousSound(3, volume, pan);
		break;
	/* the sphere, cube and tetrahedron generators */
	case 234: case 235: case 236: case 237:
		if (frame != 7)
			RequestContinuousSound(5, volume, pan);
		break;
	case 238: case 239: case 240: case 241:
		if (frame != 7)
			RequestContinuousSound(4, volume, pan);
		break;
	case 242: case 243: case 244: case 245:
		if (frame != 7)
			RequestContinuousSound(6, volume, pan);
		break;
	case 711:                       /* mill stone; its flag is never set */
		if (frame == 0 && !MillStonePlayed)
			PlaySfx(26, volume, pan, 1);
		break;
	case 934: case 941:             /* water wheels */
		if (frame == 0 && !WaterWheelPlayed) {
			PlaySfx(59, volume, pan, 1);
			WaterWheelPlayed = 1;
		}
		break;
	case 755: case 767: case 770:   /* orreries */
		RequestContinuousSound(7, volume, pan);
		break;
	/* a ghost, the Well of Souls, smoke and poison and sleep fields */
	case 337: case 748: case 769: case 900: case 902:
		RequestContinuousSound(9, volume, pan);
		break;
	case 768: case 1010:            /* energy field, prism */
		if (GenerateRandomIntegerInRange(100) < 20)
			PlaySfx(11, volume, pan, 0);
		break;
	case 547: case 548: case 559: case 562:     /* magic weapons */
		if (QuietWeapons)
			break;
		if (GenerateRandomIntegerInRange(100) < 10)
			PlaySfx(104, volume, pan, 0);
		break;
	case 168: case 230: case 786:   /* beam of light, ethereal monster, vortex cube */
		RequestContinuousSound(10, volume, pan);
		break;
	case 529:                       /* slime */
		if (GenerateRandomIntegerInRange(100) < 5)
			PlaySfx(GenerateRandomIntegerInRange(5) + 54, volume, pan, 0);
		break;
	case 494:                       /* bee */
		RequestContinuousSound(11, volume, pan);
		break;
	case 389: case 391:             /* cavern */
		if (GenerateRandomIntegerInRange(100) < 5)
			PlaySfx(103, volume, pan, 0);
		break;
	}
}

static Stopwatch interval;
static int32_t ready = -1;

void PlayAmbientSounds()
{
	/* at most once every six ticks */
	if (ready != -1 && !(ready = Stopwatch_getElapsed(&interval) > 6))
		return;
	if (MusicDevice != 0) {
		if (!InDungeon && (uint8_t)(GameTime.getHour() < 5 || GameTime.getHour() > 20)) {
			if (GenerateRandomIntegerInRange(100) < 20)
				PlaySfx(61, GenerateRandomIntegerInRange(100) + 25, GenerateRandomIntegerInRange(64) + 32, 0);
		}
		UpdateContinuousSounds();
		if ((SongStopped != 0 && CurrentMusic != MUSIC_NONE) || BackgroundMusicDue) {
			if (MusicResume) {
				PlayMusic(DeferredMusic);
				MusicResume = 1;
			} else {
				/* at sea when aboard a ship with sails, else the dungeon or overland music */
				PlayMusic(CurrentVehicle != 0 &&
					(GetItemRecord(CurrentVehicle)->typeFrame & 0x3ff) == 251 ? 8 : InDungeon ? 52 : 6);
			}
		}
		WaterWheelPlayed = 0;
		MillStonePlayed = 0;
	}
	ready = 0;
	interval.restart();
}

void PlaySoundAt(uint8_t sound, CellCoord x, CellCoord y)
{
	int16_t dx, volume, dy, pan;

	dx = GetDelta(x, Item_getX(AvatarRef));
	dy = GetDelta(y, Item_getY(AvatarRef));
	if (MusicDevice == MUSIC_DEVICE_MT32)
		volume = 127 - (abs(dy) * 3 + abs(dx) * 3);
	else
		volume = 127 - (abs(dy) * 2 + abs(dx) * 2);
	if (volume >= 0) {
		pan = 64 - dx * 2;
		PlaySfx(sound, volume, pan, 0);
	}
}

extern "C" void PlaySoundAtItem(uint8_t sound, ItemId object)
{
	int16_t volume = 255, dx, dy, pan = 64;

	if (object.valid()) {
		dx = GetDelta(Item_getX(object), Item_getX(AvatarRef));
		dy = GetDelta(Item_getY(object), Item_getY(AvatarRef));
		if (MusicDevice == MUSIC_DEVICE_MT32)
			volume = 127 - (abs(dy) * 3 + abs(dx) * 3);
		else
			volume = 127 - (abs(dy) * 2 + abs(dx) * 2);
		if (volume < 0)
			return;
		pan = 64 - dx * 2;
	}
	PlaySfx(sound, volume, pan, 0);
}

extern "C" void ResetSoundsGlobals(void)
{
	WaterWheelPlayed = 0;
	MillStonePlayed = 0;
	QuietWeapons = 0;
	AnimationPhase = 0;
	memset(ContinuousSoundRequest, 0, sizeof ContinuousSoundRequest);
	memset(ContinuousSoundVolume, 0, sizeof ContinuousSoundVolume);
	memset(&interval, 0, sizeof interval);
	ready = -1;
}

extern "C" void ConstructSoundsGlobals(void)
{
	new (&interval) Stopwatch();
}
