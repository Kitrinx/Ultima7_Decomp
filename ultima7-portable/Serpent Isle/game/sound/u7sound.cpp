/* Serpent Isle SI.EXE, resident segment 109 (file offsets 0x03a663 to 0x03b38a, 3367 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -b- rebuilds it byte for byte as C++.
 */

/* path: u7sound.c */
#include "u7port.h"
#include "u7sound.h"
#include "easyfile.h"
#include "flex.h"
#include "dosio.h"
#include "memapi.h"
#include "debug.h"
#include "random.h"
#include "init.h"
#include "plat.h"
#include <new>

const uint8_t Mt32MusicVolumes[MUSIC_TRACK_COUNT] = {
	70, 65, 70, 70, 70, 70, 70, 65, 100, 65, 70, 80, 80, 90, 80, 55,
	85, 90, 90, 90, 90, 80, 75, 70, 100, 70, 75, 90, 90, 85, 70, 90,
	85, 75, 85, 85, 85, 85, 85, 80, 80, 80, 70, 90, 80, 70, 70, 75,
	80, 80, 60, 80, 65, 60, 90, 85, 85, 70, 70, 100, 100, 100, 100, 70,
	70, 65, 70, 65, 75, 60, 0
};
const uint8_t AdlibMusicVolumes[MUSIC_TRACK_COUNT] = {
	185, 160, 180, 180, 180, 180, 190, 165, 165, 160, 175, 165, 165, 170, 180, 180,
	175, 155, 165, 165, 165, 163, 165, 170, 170, 180, 155, 150, 163, 160, 160, 165,
	165, 165, 165, 165, 165, 165, 165, 180, 180, 180, 160, 175, 180, 155, 155, 160,
	160, 180, 180, 180, 180, 180, 180, 160, 170, 165, 170, 180, 180, 180, 180, 155,
	180, 180, 170, 165, 250, 250, 0
};
/* Effect 135 has no entry; the extra one holds what DOS read past each table. */
const uint8_t Mt32SfxVolumes[SFX_COUNT] = {
	60, 90, 90, 105, 100, 75, 100, 105, 110, 120, 90, 120, 120, 100, 80, 80,
	95, 90, 90, 100, 110, 80, 80, 100, 127, 110, 127, 95, 80, 80, 90, 90,
	90, 100, 70, 70, 127, 127, 115, 127, 100, 127, 127, 100, 100, 80, 80, 80,
	90, 100, 127, 80, 100, 100, 127, 127, 100, 80, 100, 110, 110, 100, 120, 100,
	80, 127, 127, 90, 90, 40, 80, 127, 90, 90, 80, 110, 100, 100, 90, 80,
	127, 100, 80, 80, 90, 90, 127, 105, 80, 80, 110, 80, 80, 100, 90, 100,
	127, 110, 100, 100, 100, 127, 95, 90, 100, 100, 100, 120, 80, 80, 120, 120,
	120, 120, 110, 120, 127, 80, 80, 80, 100, 90, 120, 90, 100, 110, 90, 127,
	100, 80, 60, 80, 80, 120, 100, 127
};
const uint8_t AdlibSfxVolumes[SFX_COUNT] = {
	127, 150, 150, 110, 140, 170, 110, 160, 127, 150, 100, 120, 150, 140, 140, 140,
	150, 127, 127, 150, 150, 127, 127, 140, 140, 140, 160, 115, 110, 150, 150, 150,
	150, 140, 127, 127, 127, 140, 127, 140, 150, 150, 160, 200, 127, 150, 150, 150,
	150, 140, 180, 160, 127, 140, 127, 127, 127, 140, 140, 127, 127, 160, 127, 180,
	150, 150, 140, 140, 150, 150, 140, 140, 140, 150, 150, 150, 170, 170, 140, 140,
	140, 140, 127, 127, 140, 127, 127, 140, 140, 140, 150, 150, 140, 110, 140, 140,
	170, 110, 110, 110, 140, 140, 150, 110, 127, 110, 110, 160, 160, 127, 110, 110,
	110, 110, 110, 140, 140, 160, 160, 160, 127, 140, 115, 150, 140, 140, 150, 140,
	160, 160, 140, 160, 150, 190, 150, 0x88
};

int16_t AdlibPort = 0x388;
int16_t RolandArgument = 2;
uint8_t MusicResume = 0, BackgroundMusicDue = 1;
uint8_t SfxEnabled = 0, MusicEnabled = 0;
uint8_t MusicChangeDue = 0, RequestedMusic = 0;
uint16_t SfxAgeCounter = 0;
uint8_t SpecialMusicPlaying = 0;
int16_t MusicDevice = 0;
char *MusicFileName = 0, *SfxFileName = 0, *TimbreFileName = 0;
HSEQUENCE MusicSequence = -1;
Timer SfxInterval;
uint8_t CurrentMusic = MUSIC_NONE, DeferredMusic = MUSIC_NONE;
uint8_t AdlibSfxActive = 0;
const uint8_t MusicTrackModes[MUSIC_TRACK_COUNT] = {
	3, 3, 3, 3, 1, 1, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
	1, 3, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 3, 0, 0, 0, 0, 0
};

void *MusicBuffer;
void *SpecialMusicBuffer;            /* never referenced */
void *SfxBuffers[SFX_CHANNELS];
uint8_t SfxNumbers[SFX_CHANNELS];
uint16_t SfxAge[SFX_CHANNELS];
HDRIVER MusicDriver;
uint16_t *SoundDriverImage;
uint16_t TimbreCacheSize;
void *TimbreBank;
void *MusicStateTable;
int32_t MusicStateTableSize;
HSEQUENCE SfxSequences[SFX_CHANNELS];
void *SfxStateTables[SFX_CHANNELS];
void *unused_global_8[SFX_CHANNELS];   /* never referenced */
uint8_t *SfxControllerTable;
int16_t SfxRelativeVolume[SFX_CHANNELS];
uint16_t TimbreSize;
TimbreRecord TimbreEntry;

extern uint8_t SaveLoadActive;
uint8_t IsAvatarDead();
int16_t GetDominantHostileSide();

void PlayPainSfx()
{
	PlaySfx(GenerateRandomIntegerInRange(5) + 110, 127, 64);
}

void EnableMusic(int16_t enabled)
{
	if (!MusicDevice) {
		MusicEnabled = 0;
		return;
	}
	if (enabled) {
		MusicEnabled = 1;
		return;
	}
	if (MusicEnabled && !SaveLoadActive)
		StopMusicSequence();
	MusicEnabled = 0;
}

void * ReadTimbre(int16_t bank, int16_t patch)
{
	void *data;
	int32_t count = sizeof(TimbreEntry);
	int16_t file = DosOpen(TimbreFileName);

	if (file == INT32_C(0))
		AssertFail(__FILE__, 280);
	int32_t position = 0;
	do {
		ReadFileBlock(file, position, count, &TimbreEntry);
		position = position + count;
		if (TimbreEntry.bank == -1)
			return 0;
	} while (TimbreEntry.bank != bank || TimbreEntry.patch != patch);
	ReadFileBlock(file, TimbreEntry.offset, 2, &TimbreSize);
	data = AllocateFarHeap(TimbreSize + 10, 0);
	if (!data)
		AssertFail(__FILE__, 301);
	*(uint16_t *)data = TimbreSize;
	ReadFileBlock(file, TimbreEntry.offset + 2, TimbreSize - 2,
		(uint8_t *)data + 2);
	DosClose(file);
	return data;
}

void LoadSequenceTimbres(HDRIVER driver, HSEQUENCE sequence, int16_t record)
{
	void *data;
	uint16_t request;
	int16_t bank, patch;

	while ((request = AIL_timbre_request(driver, sequence)) != 0xffff) {
		bank = request >> 8;
		patch = request & 0xff;
		if (!AIL_timbre_status(driver, bank, patch)) {
			data = ReadTimbre(bank, patch);
			if (data) {
				AIL_install_timbre(driver, bank, patch, data);
				FreeFarHeap(data);
			} else
				AssertFail(__FILE__, 354);
		}
	}
}

uint8_t ReadMusicRecord(int16_t record, void *destination)
{
	Flex file;

	if (file.open(BuildPath(StaticPath, MusicFileName, 0))) {
		file.readRecord(record, destination, 0);
		file.close();
		if (!destination)
			return 0;
		else
			return 1;
	} else
		return 0;
}

void PreloadMusicTimbres()
{
	void *data;
	int16_t i;

	if (MusicDevice == MUSIC_DEVICE_MT32 && ReadMusicRecord(70, MusicBuffer)) {
		if ((MusicSequence = AIL_register_sequence(MusicDriver, MusicBuffer, 0,
			MusicStateTable, 0)) == -1)
			AssertFail(__FILE__, 419);
		LoadSequenceTimbres(MusicDriver, MusicSequence, 70);
		AIL_start_sequence(MusicDriver, MusicSequence);
		while (AIL_sequence_status(MusicDriver, MusicSequence) != SEQ_DONE)
			plat_yield();
		int16_t bank[10] = {65,65,65,65,65,65,65,65,65,66};
		int16_t patch[10] = {0,1,2,3,4,5,6,21,22,16};
		for (i = 0; i < 10; i++) {
			data = ReadTimbre(bank[i], patch[i]);
			if (data) {
				AIL_install_timbre(MusicDriver, bank[i], patch[i], data);
				AIL_protect_timbre(MusicDriver, bank[i], patch[i]);
				FreeFarHeap(data);
			} else
				AssertFail(__FILE__, 455);
		}
	}
}

void StopMusicSequence()
{
	if (MusicSequence != -1) {
		AIL_stop_sequence(MusicDriver, MusicSequence);
		AIL_release_sequence_handle(MusicDriver, MusicSequence);
		MusicSequence = -1;
	}
}

void PlayMusic(uint8_t track)
{
	int16_t mode, status, beat;

	if (SaveLoadActive || !MusicDevice || !MusicEnabled)
		return;
	if (IsAvatarDead() && track != 8)
		return;
	if (track == 49) {
	}
	MusicResume = 0;
	if (track >= MUSIC_TRACK_COUNT && track != MUSIC_NONE)
		return;
	if (MusicSequence != -1)
		status = AIL_sequence_status(MusicDriver, MusicSequence);
	else
		status = 0;
	if (status == SEQ_STOPPED || status == SEQ_DONE) {
		CurrentMusic = DeferredMusic = MUSIC_NONE;
		if (SpecialMusicPlaying)
			SpecialMusicPlaying = 0;
		StopMusicSequence();
	} else {
		beat = AIL_beat_count(MusicDriver, MusicSequence);
		if (beat > 0) {
			MusicChangeDue = 1;
			RequestedMusic = track;
			return;
		}
		MusicChangeDue = 0;
	}
	if (track == MUSIC_NONE)
		mode = 4;
	else
		mode = MusicTrackModes[track];
	if (SpecialMusicPlaying && (mode == 1 || track == MUSIC_NONE))
		DeferredMusic = track;
	else {
		if (mode == 0 && (SpecialMusicPlaying || status == SEQ_PLAYING)) {
			/* DOS read zeroed data past the table for no track */
			if ((DeferredMusic < MUSIC_TRACK_COUNT ? MusicTrackModes[DeferredMusic] : 0) != 0)
				DeferredMusic = track;
		} else if (track != CurrentMusic) {
			if (track == MUSIC_NONE || track == 49)
				StopMusicSequence();
			else if (mode == 2)
				track = CurrentMusic;
			else {
				if (CurrentMusic != MUSIC_NONE)
					StopMusicSequence();
				if (ReadMusicRecord(track, MusicBuffer)) {
					if ((MusicSequence = AIL_register_sequence(MusicDriver, MusicBuffer, 0,
						MusicStateTable, 0)) == -1)
						AssertFail(__FILE__, 608);
					LoadSequenceTimbres(MusicDriver, MusicSequence, track);
					if (MusicDevice == MUSIC_DEVICE_MT32) {
						AIL_set_controller_value(MusicDriver, MusicSequence, 1, 61, 2);
						AIL_set_controller_value(MusicDriver, MusicSequence, 1, 62, 4);
						AIL_set_controller_value(MusicDriver, MusicSequence, 1, 63, 5);
					}
					AIL_start_sequence(MusicDriver, MusicSequence);
					if (MusicDevice == MUSIC_DEVICE_MT32)
						AIL_set_relative_volume(MusicDriver, MusicSequence, Mt32MusicVolumes[track], 0);
					else
						AIL_set_relative_volume(MusicDriver, MusicSequence, AdlibMusicVolumes[track], 0);
					BackgroundMusicDue = 0;
				} else {
					track = MUSIC_NONE;
					mode = 1;
				}
			}
			CurrentMusic = DeferredMusic = track;
			if (mode == 3 || mode == 2)
				SpecialMusicPlaying = 1;
			else if (mode == 1 || mode == 4)
				SpecialMusicPlaying = 0;
		}
	}
}

void StopMusic()
{
	MusicResume = 0;
	StopMusicSequence();
	CurrentMusic = MUSIC_NONE;
}

void PlayCombatMusic()
{
	if (!SpecialMusicPlaying || CurrentMusic == 1) {
		switch (GetDominantHostileSide()) {
		case 1: PlayMusic(3); break;
		case 2: PlayMusic(2); break;
		}
	}
}

void NoteCombatAlignment(int8_t) {}

uint8_t ReadSfxRecord(int16_t record, void *destination)
{
	Flex file;

	if (file.open(BuildPath(StaticPath, SfxFileName, 0))) {
		file.readRecord(record, destination, 0);
		file.close();
		if (!destination)
			return 0;
		else
			return 1;
	} else
		return 0;
}

void ReleaseSfxChannels(int16_t slot, int8_t percussion)
{
	uint16_t channel1 = 0, channel2 = 0;

	channel1 = AIL_true_sequence_channel(MusicDriver, SfxSequences[slot], 11);
	channel2 = AIL_true_sequence_channel(MusicDriver, SfxSequences[slot], 12);
	if (channel1 != 11)
		AIL_release_channel(MusicDriver, channel1);
	if (channel2 != 12)
		AIL_release_channel(MusicDriver, channel2);
	if (percussion)
		AIL_release_channel(MusicDriver, 16);
}

extern "C" void PlaySfx(uint8_t number, uint16_t volume, int16_t pan)
{
	uint8_t found = 0, unused = 0;

	if (!SfxEnabled || number == SFX_NONE)
		return;
	if (number > SFX_LAST)
		AssertFail(__FILE__, 877);
	int16_t i = 0;
	if (SpecialMusicPlaying) {
		uint32_t remaining = Timer_getRemaining(&SfxInterval);
		if (remaining > 0 && remaining < 30)
			return;
		Timer_set(&SfxInterval, 30);
	}
	for (i = 0; i < SFX_CHANNELS; i++)
		if (SfxNumbers[i] == number) {
			found = 1;
			break;
		}
	if (volume == 255)
		if (MusicDevice == MUSIC_DEVICE_MT32)
			volume = Mt32SfxVolumes[number];
		else
			volume = AdlibSfxVolumes[number];
	if (found) {
		int16_t status = AIL_sequence_status(MusicDriver, SfxSequences[i]);
		if (status != SEQ_PLAYING) {
			LoadSequenceTimbres(MusicDriver, SfxSequences[i], number);
			uint8_t *control = SfxControllerTable+i*2;
			*control = pan;
			control++;
			*control = volume;
			AIL_start_sequence(MusicDriver, SfxSequences[i]);
		}
		SfxAge[i] = SfxAgeCounter;
		SfxAgeCounter++;
	} else {
		int16_t selected = 0;
		for (i = 0; i < SFX_CHANNELS; i++)
			if (SfxNumbers[i] == SFX_NONE)
				break;
		if (i == SFX_CHANNELS) {
			for (i = 0; i < SFX_CHANNELS; i++)
				if (SfxAge[i] < SfxAge[selected])
					selected = i;
		} else
			selected = i;
		if (SfxSequences[selected] != -1) {
			ReleaseSfxChannels(selected, 0);
			AIL_stop_sequence(MusicDriver, SfxSequences[selected]);
			AIL_release_sequence_handle(MusicDriver, SfxSequences[selected]);
			SfxSequences[selected] = -1;
		}
		if (ReadSfxRecord(number, SfxBuffers[selected])) {
			if ((SfxSequences[selected] = AIL_register_sequence(MusicDriver, SfxBuffers[selected], 0,
				SfxStateTables[selected], SfxControllerTable+selected*2)) == -1)
				AssertFail(__FILE__, 990);
			LoadSequenceTimbres(MusicDriver, SfxSequences[selected], number);
			uint8_t *control = SfxControllerTable+selected*2;
			*control = pan;
			control++;
			*control = volume;
			AIL_start_sequence(MusicDriver, SfxSequences[selected]);
			SfxNumbers[selected] = number;
			SfxAge[selected] = SfxAgeCounter;
			SfxAgeCounter++;
		} else
			goto done;
	}
done:
	if (SfxAgeCounter > 65530) {
		uint16_t minIndex = 0;
		i = 0;
		while (i < SFX_CHANNELS) {
			if (SfxAge[i] < SfxAge[minIndex])
				minIndex = i;
			int16_t oldest = SfxAge[minIndex];
			for (i = 0; i < SFX_CHANNELS; i++)
				SfxAge[i] = SfxAge[i] - oldest;
			i++;
		}
	}
}

void StopSfx(uint8_t number, int16_t slot, int8_t percussion)
{
	int16_t first, end;

	if (number <= SFX_LAST) {
		if (slot >= 0 && slot < SFX_CHANNELS && SfxSequences[slot] != -1) {
			first = slot;
			end = slot+1;
		} else {
			first = 0;
			end = SFX_CHANNELS;
		}
		for (int16_t i = first; i < end; i++)
			if (SfxNumbers[i] == number && SfxSequences[i] != -1)
				if (SfxSequences[i] != -1) {
					ReleaseSfxChannels(i, percussion);
					AIL_stop_sequence(MusicDriver, SfxSequences[i]);
					SfxRelativeVolume[i] = 0;
				}
	}
}

void SetSfxVolume(uint8_t number, int16_t volume, int16_t previous)
{
	int16_t base, change, delta, result;

	if (!SfxEnabled)
		return;
	for (int16_t i = 0; i < SFX_CHANNELS; i++) {
		if (SfxNumbers[i] == number) {
			if (MusicDevice == MUSIC_DEVICE_MT32)
				base = Mt32SfxVolumes[number];
			else
				base = AdlibSfxVolumes[number];
			change = previous - volume;
			change *= 100;
			delta = -(change / base);
			result = SfxRelativeVolume[i] + delta;
			if (delta == 0 || result < 10 || result > 100)
				return;
			AIL_set_relative_volume(MusicDriver, SfxSequences[i], result, 0);
			SfxRelativeVolume[i] = result;
		}
	}
}

void EnableSfx(int8_t enabled)
{
	int16_t i;

	if (!MusicDevice) {
		SfxEnabled = 0;
		return;
	}
	if (enabled) {
		SfxEnabled = 1;
		return;
	}
	if (SfxEnabled && !SaveLoadActive)
		for (i = 0; i < SFX_CHANNELS; i++)
			if (SfxNumbers[i] != SFX_NONE)
				StopSfx(SfxNumbers[i], 255, 0);
	SfxEnabled = 0;
}

extern "C" void ResetU7soundGlobals(void)
{
	AdlibPort = 0x388;
	RolandArgument = 2;
	MusicResume = 0;
	BackgroundMusicDue = 1;
	SfxEnabled = 0;
	MusicEnabled = 0;
	MusicChangeDue = 0;
	RequestedMusic = 0;
	SfxAgeCounter = 0;
	SpecialMusicPlaying = 0;
	MusicDevice = 0;
	MusicFileName = 0;
	SfxFileName = 0;
	TimbreFileName = 0;
	MusicSequence = -1;
	memset(&SfxInterval, 0, sizeof SfxInterval);
	CurrentMusic = MUSIC_NONE;
	DeferredMusic = MUSIC_NONE;
	AdlibSfxActive = 0;
	MusicBuffer = 0;
	SpecialMusicBuffer = 0;
	memset(SfxBuffers, 0, sizeof SfxBuffers);
	memset(SfxNumbers, 0, sizeof SfxNumbers);
	memset(SfxAge, 0, sizeof SfxAge);
	MusicDriver = 0;
	SoundDriverImage = 0;
	TimbreCacheSize = 0;
	TimbreBank = 0;
	MusicStateTable = 0;
	MusicStateTableSize = 0;
	memset(SfxSequences, 0, sizeof SfxSequences);
	memset(SfxStateTables, 0, sizeof SfxStateTables);
	memset(unused_global_8, 0, sizeof unused_global_8);
	SfxControllerTable = 0;
	memset(SfxRelativeVolume, 0, sizeof SfxRelativeVolume);
	TimbreSize = 0;
	memset(&TimbreEntry, 0, sizeof TimbreEntry);
}

extern "C" void ConstructU7soundGlobals(void)
{
	new (&SfxInterval) Timer();
}
