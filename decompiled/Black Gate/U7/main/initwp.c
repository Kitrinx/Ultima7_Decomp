/* Black Gate U7.EXE, overlay segment 237 (file offsets 0x06d240 to 0x06d893, 1619 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include <dos.h>
#include "dosio.h"
#include "coord.h"
#include "easyfile.h"
#include "flex.h"
#include "adlib.h"
#include "debug.h"
#include "equip.h"
#include "itembuf.h"
#include "maps.h"
#include "midiplay.h"
#include "misstrac.h"
#include "sounds.h"
#include "sysusage.h"
#include "type.h"
#include "weight.h"
#include "wihh.h"
#include "u7sound.h"
#include "target.h"
#include "memapi.h"
#include "init.h"
#include "initwp.h"

/* the start of the driver's description block */
struct SoundDriver {
	char unusedField1[18];
	int first;              /* first voice channel */
};

extern void (far pascal *DriverSysexEntry)(long, int, void far *);
extern void (far pascal *DriverPortEntry)(int);
extern char PrevSoundTimer[];

char *TfaFileName = "TFA.DAT";
char *WgtVolFileName = "WGTVOL.DAT";
char *WihhFileName = "WIHH.DAT";

void InitWorldPhysics(void)
{
	AddMap(12);
	SetCurrentMap(Coord(0));
	LogMemoryUsage("TFA");
	LoadTfa((char *)gItemTypeInfo, BuildPath(StaticPath, TfaFileName, 0));
	LogMemoryUsage("WV");
	LoadWgtVol(BuildPath(StaticPath, WgtVolFileName, 0));
	LogMemoryUsage("HOT");
	LoadWihh(BuildPath(StaticPath, WihhFileName, 0));
	LogMemoryUsage("Item Buffer");
	AllocItemBuffer(6668, 256, 100);
	ResetItemBuffer();
	LogMemoryUsage("Equipment");
	InitEquipment();
	LogMemoryUsage("Missile Tracker");
	ResetMissileTrackers();
	LogMemoryUsage("Shadow NPC Buffer");
	AllocateNPCPoses(&ShadowNpcBuffer);
}

void InitRolandVoices(void)
{
	int i, first;

	FirstVoiceChannel = SoundDriverInfo->first;
	first = SoundDriverInfo->first;
	for (i = 0; i < 32; i++) {
		VoiceSlots[i].number = first++;
		VoiceSlots[i].voice = RolandVoiceNumbers[i] - 1;
		VoiceSlots[i].sound = -1;
		VoiceSlots[i].next = &VoiceSlots[i + 1];
		VoiceSlots[i].previous = &VoiceSlots[i - 1];
	}
	VoiceSlots[0].previous = &VoiceSlots[31];
	VoiceSlots[31].next = &VoiceSlots[0];
	VoiceSlotHead = VoiceSlots;
}

void UploadRolandPatches(void)
{
	unsigned char *p;
	int count;
	long address;

	if (MusicDevice == MUSIC_DEVICE_MT32) {
		p = RolandPatchSetup;
		count = *p++;
		while (count != 0) {
			address = *p++;
			address <<= 8;
			address += *p++;
			address <<= 8;
			address += *p++;
			DriverSysexEntry(address, count * 4, p);
			p += count * 4;
			count = *p++;
		}
	}
}

unsigned char InitSound(void)
{
	if (MusicDevice != 0) {
		MusicFileName = MusicDevice == MUSIC_DEVICE_ADLIB ? AdlibMusicFile : Mt32MusicFile;
		StartSoundDriver(BuildPath(StaticPath, VoiceFlexName, 0), BuildPath(StaticPath, DriverFileName, 0));
		if (SoundDriverImage == 0 || MusicDevice == 0)
			return 0;
		if (MusicDevice == MUSIC_DEVICE_ADLIB) {
			if (AdlibPort != 0x388)
				DriverPortEntry(AdlibPort);
			ResetAdlib();
			HookInterruptVector(8, SoundTimerHandler, 0, (long far *)PrevSoundTimer);
			AdlibSfxActive = 1;
		}
		if (MusicDevice == MUSIC_DEVICE_MT32) {
			UploadRolandPatches();
			InitRolandVoices();
		}
		if (!AllocMusicBuffers() || !AllocSfxBuffers())
			return 0;
		ResetContinuousSounds();
	}
	return 1;
}

void ShutDownSound(void)
{
	if (MusicDevice != 0) {
		FadeOutSong(0);
		if (AdlibSfxActive != 0) {
			ReclaimAdlibChannels();
			ResetAdlib();
			disable();
			UnhookInterrupt(8);
			enable();
		}
		StopSoundDriver();
		disable();
		UnhookInterrupt(8);
		UnhookInterrupt(8);
		enable();
		MusicDevice = 0;
	}
}

unsigned char AllocMusicBuffers(void)
{
	FlexEntry e;
	long size, maxNormal, maxSpecial;
	int i;
	Flex file;

	maxNormal = 0;
	maxSpecial = 0;
	MusicBuffer = 0;
	if (file.open(BuildPath(StaticPath, MusicFileName, 0))) {
		for (i = 0; i < 60; i++) {
			file.getEntry(i, &e);
			size = e.size;
			if (MusicTrackModes[i] == 1) {
				if (size > maxSpecial)
					maxSpecial = size;
			} else {
				if (size > maxNormal)
					maxNormal = size;
			}
		}
		file.close();
	}
	MusicBuffer = (unsigned char far *)AllocateFarHeap(maxNormal, 0);
	/* keep the braces: they change how these returns destroy the file */
	if (MusicBuffer == 0) {
		return 0;
	}
	SpecialMusicBuffer = (unsigned char far *)AllocateFarHeap(maxSpecial, 0);
	if (SpecialMusicBuffer == 0) {
		return 0;
	}
	return 1;
}

unsigned char AllocSfxBuffers(void)
{
	FlexEntry e;
	long maxSize;
	void far *buffer;
	int i;
	Flex file;

	if (MusicDevice == MUSIC_DEVICE_MT32) {
		for (i = 0; i < SFX_COUNT; i++) {
			SfxVoices[i] = 0;
			SfxAlternate[i] = 0;
		}
	}
	if (AdlibSfxActive != 0) {
		if (file.open(BuildPath(StaticPath, AdlibSfxFile, 0))) {
			maxSize = 0;
			for (i = 0; i < SFX_COUNT; i++) {
				file.getEntry(i, &e);
				if (e.size > maxSize)
					maxSize = e.size;
			}
			file.close();
			maxSize = (maxSize + 15) & ~15L;
			buffer = AllocateFarHeap(maxSize << 2, 2);
			if (buffer == 0)
				return 0;
			/* each channel gets a quarter of the buffer, as a segment */
			for (i = 0; i < 4; i++)
				SfxChannels[i].buffer = *((unsigned far *)&buffer + 1) + i * (maxSize >> 4);
		} else
			return 0;
	}
	return 1;
}
