/* Serpent Isle SI.EXE, overlay segment 225 (file offsets 0x05eb10 to 0x05f22e, 1822 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 */

/* path: initwp.c */
#include "u7port.h"
#include "dosio.h"
#include "coord.h"
#include "easyfile.h"
#include "flex.h"
#include "debug.h"
#include "equip.h"
#include "itembuf.h"
#include "maps.h"
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
#include "plat.h"
#include "text.h"

char *const TfaFileName = "TFA.DAT";
char *const WgtVolFileName = "WGTVOL.DAT";
char *const WihhFileName = "WIHH.DAT";

void InitWorldPhysics(void)
{
	AddMap(12);
	SetCurrentMap(Coord(0));
	LogMemoryUsage(GetGameText(3, 201));
	LoadTfa((char *)gItemTypeInfo, BuildPath(StaticPath, TfaFileName, 0));
	LogMemoryUsage(GetGameText(3, 202));
	LoadWgtVol(BuildPath(StaticPath, WgtVolFileName, 0));
	LogMemoryUsage(GetGameText(3, 203));
	LoadWihh(BuildPath(StaticPath, WihhFileName, 0));
	LogMemoryUsage(GetGameText(3, 204));
	AllocItemBuffer(6668, 256, 100);
	ResetItemBuffer();
	LogMemoryUsage(GetGameText(3, 205));
	InitEquipment();
	LogMemoryUsage(GetGameText(3, 206));
	ResetMissileTrackers();
	LogMemoryUsage(GetGameText(3, 207));
	AllocateNPCPoses(&ShadowNpcBuffer);
}

uint8_t InitSound(void)
{
	/* music plays only on the MT-32: any configured music device becomes one */
	if (MusicDevice != 0)
		MusicDevice = plat_midi_available() ? MUSIC_DEVICE_MT32 : 0;
	if (MusicDevice != 0) {
		drvr_desc *description;
		int32_t driverSize;
		int16_t i, record;
		Flex file;

		switch (MusicDevice) {
		case MUSIC_DEVICE_ADLIB:
			driverSize = INT32_C(17031);
			record = 0;
			MusicFileName = "adlibmus.dat";
			SfxFileName = "adlibsfx.dat";
			TimbreFileName = "static\\xmidi.ad";
			break;
		case MUSIC_DEVICE_MT32:
			driverSize = INT32_C(10707);
			record = 1;
			MusicFileName = "mt32mus.dat";
			SfxFileName = "mt32sfx.dat";
			TimbreFileName = "static\\xmidi.mt";
			break;
		}
		if (!file.open("static\\snddrvrs.dat"))
			AssertFail(__FILE__, 185);
		SoundDriverImage = (uint16_t *)AllocateFarHeap(driverSize, 2);
		if (!SoundDriverImage)
			AssertFail(__FILE__, 192);
		if (!file.readRecord(record, SoundDriverImage, 0))
			AssertFail(__FILE__, 198);
		MusicDriver = AIL_register_driver(SoundDriverImage);
		if (MusicDriver == -1)
			AssertFail(__FILE__, 206);
		description = AIL_describe_driver(MusicDriver);
		if (description->drvr_type != XMIDI_DRVR)
			AssertFail(__FILE__, 213);
		if (!AIL_detect_device(MusicDriver, description->default_IO,
			description->default_IRQ, description->default_DMA, description->default_DRQ))
			FatalError("\nSerpent Isle could not detect the selected\n"
				"sound card for Music and Sound FX.\n");
		AIL_init_driver(MusicDriver, description->default_IO,
			description->default_IRQ, description->default_DMA, description->default_DRQ);
		MusicStateTableSize = AIL_state_table_size(MusicDriver);
		MusicStateTable = AllocateFarHeap(MusicStateTableSize, 0);
		if (!MusicStateTable)
			AssertFail(__FILE__, 235);
		for (i = 0; i < SFX_CHANNELS; i++) {
			SfxStateTables[i] = AllocateFarHeap(MusicStateTableSize, 0);
			if (!SfxStateTables[i])
				AssertFail(__FILE__, 246);
		}
		SfxControllerTable = (uint8_t *)AllocateFarHeap(INT32_C(140), 0);
		if (!SfxControllerTable)
			AssertFail(__FILE__, 251);
		for (i = 0; i < 12; i++)
			SfxControllerTable[i] = 0;
		for (i = 0; i < SFX_CHANNELS; i++) {
			SfxSequences[i] = -1;
			SfxRelativeVolume[i] = 0;
		}
		TimbreCacheSize = AIL_default_timbre_cache_size(MusicDriver);
		if (TimbreCacheSize != 0) {
			TimbreBank = AllocateFarHeap(TimbreCacheSize, 0);
			if (TimbreBank)
				AIL_define_timbre_cache(MusicDriver, TimbreBank, TimbreCacheSize);
			else
				AssertFail(__FILE__, 278);
		}
		AdlibSfxActive = 1;
		if (!AllocMusicBuffers() || !AllocSfxBuffers())
			return 0;
		ResetContinuousSounds();
		PreloadMusicTimbres();
	}
	return 1;
}

void ShutDownSound(void)
{
	if (MusicDevice != 0)
		MusicDevice = 0;
}

uint8_t AllocMusicBuffers(void)
{
	FlexEntry e;
	int32_t size, maxNormal, maxSpecial;
	int16_t i;
	Flex file;

	maxNormal = 0;
	maxSpecial = 0;
	MusicBuffer = 0;
	if (file.open(BuildPath(StaticPath, MusicFileName, 0))) {
		for (i = 0; i < MUSIC_TRACK_COUNT; i++) {
			file.getEntry(i, &e);
			size = e.size;
			if (MusicTrackModes[i] == 2) {
				if (size > maxSpecial)
					maxSpecial = size;
			} else {
				if (size > maxNormal)
					maxNormal = size;
			}
		}
		file.close();
	}
	MusicBuffer = (uint8_t *)AllocateFarHeap(maxNormal, 0);
	if (MusicBuffer == 0)
		return 0;
	else {
		return 1;
	}
}

uint8_t AllocSfxBuffers(void)
{
	FlexEntry e;
	int32_t maxSize;
	void *buffer;
	int16_t i;
	Flex file;

	if (AdlibSfxActive != 0) {
		if (file.open(BuildPath(StaticPath, SfxFileName, 0))) {
			maxSize = 0;
			for (i = 0; i < SFX_LAST; i++) {
				file.getEntry(i, &e);
				if (e.size > maxSize)
					maxSize = e.size;
			}
			buffer = AllocateFarHeap(maxSize * SFX_CHANNELS, 0);
			if (buffer == 0)
				return 0;
			for (i = 0; i < SFX_CHANNELS; i++) {
				SfxBuffers[i] = (uint8_t *)buffer + i * maxSize;
				SfxNumbers[i] = SFX_NONE;
			}
		}
		return 1;
	}
	return 0;
}
