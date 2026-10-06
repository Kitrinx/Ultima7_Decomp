/* Serpent Isle SI.EXE, resident segment 109 (file offsets 0x03a663 to 0x03b38a, 3367 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -b- rebuilds it byte for byte as C++.
 */

/* path: u7sound.c */
#include "u7sound.h"
#include "easyfile.h"
#include "flex.h"
#include "dosio.h"
#include "memapi.h"
#include "debug.h"
#include "random.h"
#include "init.h"

unsigned char Mt32MusicVolumes[MUSIC_TRACK_COUNT] = {
	70, 65, 70, 70, 70, 70, 70, 65, 100, 65, 70, 80, 80, 90, 80, 55,
	85, 90, 90, 90, 90, 80, 75, 70, 100, 70, 75, 90, 90, 85, 70, 90,
	85, 75, 85, 85, 85, 85, 85, 80, 80, 80, 70, 90, 80, 70, 70, 75,
	80, 80, 60, 80, 65, 60, 90, 85, 85, 70, 70, 100, 100, 100, 100, 70,
	70, 65, 70, 65, 75, 60, 0
};
unsigned char AdlibMusicVolumes[MUSIC_TRACK_COUNT] = {
	185, 160, 180, 180, 180, 180, 190, 165, 165, 160, 175, 165, 165, 170, 180, 180,
	175, 155, 165, 165, 165, 163, 165, 170, 170, 180, 155, 150, 163, 160, 160, 165,
	165, 165, 165, 165, 165, 165, 165, 180, 180, 180, 160, 175, 180, 155, 155, 160,
	160, 180, 180, 180, 180, 180, 180, 160, 170, 165, 170, 180, 180, 180, 180, 155,
	180, 180, 170, 165, 250, 250, 0
};
unsigned char Mt32SfxVolumes[SFX_VOLUME_COUNT] = {
	60, 90, 90, 105, 100, 75, 100, 105, 110, 120, 90, 120, 120, 100, 80, 80,
	95, 90, 90, 100, 110, 80, 80, 100, 127, 110, 127, 95, 80, 80, 90, 90,
	90, 100, 70, 70, 127, 127, 115, 127, 100, 127, 127, 100, 100, 80, 80, 80,
	90, 100, 127, 80, 100, 100, 127, 127, 100, 80, 100, 110, 110, 100, 120, 100,
	80, 127, 127, 90, 90, 40, 80, 127, 90, 90, 80, 110, 100, 100, 90, 80,
	127, 100, 80, 80, 90, 90, 127, 105, 80, 80, 110, 80, 80, 100, 90, 100,
	127, 110, 100, 100, 100, 127, 95, 90, 100, 100, 100, 120, 80, 80, 120, 120,
	120, 120, 110, 120, 127, 80, 80, 80, 100, 90, 120, 90, 100, 110, 90, 127,
	100, 80, 60, 80, 80, 120, 100
};
unsigned char AdlibSfxVolumes[SFX_VOLUME_COUNT] = {
	127, 150, 150, 110, 140, 170, 110, 160, 127, 150, 100, 120, 150, 140, 140, 140,
	150, 127, 127, 150, 150, 127, 127, 140, 140, 140, 160, 115, 110, 150, 150, 150,
	150, 140, 127, 127, 127, 140, 127, 140, 150, 150, 160, 200, 127, 150, 150, 150,
	150, 140, 180, 160, 127, 140, 127, 127, 127, 140, 140, 127, 127, 160, 127, 180,
	150, 150, 140, 140, 150, 150, 140, 140, 140, 150, 150, 150, 170, 170, 140, 140,
	140, 140, 127, 127, 140, 127, 127, 140, 140, 140, 150, 150, 140, 110, 140, 140,
	170, 110, 110, 110, 140, 140, 150, 110, 127, 110, 110, 160, 160, 127, 110, 110,
	110, 110, 110, 140, 140, 160, 160, 160, 127, 140, 115, 150, 140, 140, 150, 140,
	160, 160, 140, 160, 150, 190, 150
};

int AdlibPort = 0x388;
int RolandArgument = 2;
unsigned char MusicResume = 0, BackgroundMusicDue = 1;
unsigned char SfxEnabled = 0, MusicEnabled = 0;
unsigned char MusicChangeDue = 0, RequestedMusic = 0;
unsigned SfxAgeCounter = 0;
unsigned char SpecialMusicPlaying = 0;
int MusicDevice = 0;
char *MusicFileName = 0, *SfxFileName = 0, *TimbreFileName = 0;
HSEQUENCE MusicSequence = -1;
Timer SfxInterval;
unsigned char CurrentMusic = MUSIC_NONE, DeferredMusic = MUSIC_NONE;
unsigned char AdlibSfxActive = 0;
unsigned char MusicTrackModes[MUSIC_TRACK_COUNT] = {
	3, 3, 3, 3, 1, 1, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
	1, 3, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 3, 0, 0, 0, 0, 0
};

void far *MusicBuffer;
void far *SpecialMusicBuffer;            /* never referenced */
void far *SfxBuffers[SFX_CHANNELS];
unsigned char SfxNumbers[SFX_CHANNELS];
unsigned SfxAge[SFX_CHANNELS];
HDRIVER MusicDriver;
unsigned far *SoundDriverImage;
unsigned TimbreCacheSize;
void far *TimbreBank;
void far *MusicStateTable;
long MusicStateTableSize;
HSEQUENCE SfxSequences[SFX_CHANNELS];
void far *SfxStateTables[SFX_CHANNELS];
void far *unused_global_8[SFX_CHANNELS];   /* never referenced */
unsigned char far *SfxControllerTable;
int SfxRelativeVolume[SFX_CHANNELS];
unsigned TimbreSize;
TimbreRecord TimbreEntry;

extern unsigned char SaveLoadActive;
unsigned char far IsAvatarDead();
int far GetDominantHostileSide();

void far PlayPainSfx()
{
	PlaySfx(GenerateRandomIntegerInRange(5) + 110, 127, 64);
}

void far EnableMusic(int enabled)
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

void far *far ReadTimbre(int bank, int patch)
{
	void far *data;
	long count = sizeof(TimbreEntry);
	int file = DosOpen(TimbreFileName);

	if (file == 0L)
		AssertFail(__FILE__, 280);
	long position = 0;
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
	*(unsigned far *)data = TimbreSize;
	ReadFileBlock(file, TimbreEntry.offset + 2, TimbreSize - 2,
		(unsigned char far *)data + 2);
	DosClose(file);
	return data;
}

void far LoadSequenceTimbres(HDRIVER driver, HSEQUENCE sequence, int record)
{
	void far *data;
	unsigned request;
	int bank, patch;

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

unsigned char far ReadMusicRecord(int record, void far *destination)
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

void far PreloadMusicTimbres()
{
	void far *data;
	int i;

	if (MusicDevice == MUSIC_DEVICE_MT32 && ReadMusicRecord(70, MusicBuffer)) {
		if ((MusicSequence = AIL_register_sequence(MusicDriver, MusicBuffer, 0,
			MusicStateTable, 0)) == -1)
			AssertFail(__FILE__, 419);
		LoadSequenceTimbres(MusicDriver, MusicSequence, 70);
		AIL_start_sequence(MusicDriver, MusicSequence);
		while (AIL_sequence_status(MusicDriver, MusicSequence) != SEQ_DONE) {}
		int bank[10] = {65,65,65,65,65,65,65,65,65,66};
		int patch[10] = {0,1,2,3,4,5,6,21,22,16};
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

void far StopMusicSequence()
{
	if (MusicSequence != -1) {
		AIL_stop_sequence(MusicDriver, MusicSequence);
		AIL_release_sequence_handle(MusicDriver, MusicSequence);
		MusicSequence = -1;
	}
}

void far PlayMusic(unsigned char track)
{
	int mode, status, beat;

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
			if (MusicTrackModes[DeferredMusic] != 0)
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

void far StopMusic()
{
	MusicResume = 0;
	StopMusicSequence();
	CurrentMusic = MUSIC_NONE;
}

void far PlayCombatMusic()
{
	if (!SpecialMusicPlaying || CurrentMusic == 1) {
		switch (GetDominantHostileSide()) {
		case 1: PlayMusic(3); break;
		case 2: PlayMusic(2); break;
		}
	}
}

void far NoteCombatAlignment(char) {}

unsigned char far ReadSfxRecord(int record, void far *destination)
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

void far ReleaseSfxChannels(int slot, char percussion)
{
	unsigned channel1 = 0, channel2 = 0;

	channel1 = AIL_true_sequence_channel(MusicDriver, SfxSequences[slot], 11);
	channel2 = AIL_true_sequence_channel(MusicDriver, SfxSequences[slot], 12);
	if (channel1 != 11)
		AIL_release_channel(MusicDriver, channel1);
	if (channel2 != 12)
		AIL_release_channel(MusicDriver, channel2);
	if (percussion)
		AIL_release_channel(MusicDriver, 16);
}

extern "C" void far PlaySfx(unsigned char number, unsigned volume, int pan)
{
	unsigned char found = 0, unused = 0;

	if (!SfxEnabled || number == SFX_NONE)
		return;
	if (number > SFX_LAST)
		AssertFail(__FILE__, 877);
	int i = 0;
	if (SpecialMusicPlaying) {
		unsigned long remaining = Timer_getRemaining(&SfxInterval);
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
		int status = AIL_sequence_status(MusicDriver, SfxSequences[i]);
		if (status != SEQ_PLAYING) {
			LoadSequenceTimbres(MusicDriver, SfxSequences[i], number);
			unsigned char far *control = SfxControllerTable+i*2;
			*control = pan;
			control++;
			*control = volume;
			AIL_start_sequence(MusicDriver, SfxSequences[i]);
		}
		SfxAge[i] = SfxAgeCounter;
		SfxAgeCounter++;
	} else {
		int selected = 0;
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
			unsigned char far *control = SfxControllerTable+selected*2;
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
		unsigned minIndex = 0;
		i = 0;
		while (i < SFX_CHANNELS) {
			if (SfxAge[i] < SfxAge[minIndex])
				minIndex = i;
			int oldest = SfxAge[minIndex];
			for (i = 0; i < SFX_CHANNELS; i++)
				SfxAge[i] = SfxAge[i] - oldest;
			i++;
		}
	}
}

void far StopSfx(unsigned char number, int slot, char percussion)
{
	int first, end;

	if (number <= SFX_LAST) {
		if (slot >= 0 && slot < SFX_CHANNELS && SfxSequences[slot] != -1) {
			first = slot;
			end = slot+1;
		} else {
			first = 0;
			end = SFX_CHANNELS;
		}
		for (int i = first; i < end; i++)
			if (SfxNumbers[i] == number && SfxSequences[i] != -1)
				if (SfxSequences[i] != -1) {
					ReleaseSfxChannels(i, percussion);
					AIL_stop_sequence(MusicDriver, SfxSequences[i]);
					SfxRelativeVolume[i] = 0;
				}
	}
}

void far SetSfxVolume(unsigned char number, int volume, int previous)
{
	int base, change, delta, result;

	if (!SfxEnabled)
		return;
	for (int i = 0; i < SFX_CHANNELS; i++) {
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

void far EnableSfx(char enabled)
{
	int i;

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
