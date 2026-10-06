/* Serpent Isle MAINMENU.EXE, resident segment 45 (file offsets 0x014aa3 to 0x0150ba, 1559 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include <dir.h>
#include <dos.h>
#include "dosio.h"
#include "oops.h"
#include "errors.h"
#include "memapi.h"
#include "flex.h"
#include "music.h"

#define MUSIC_DEVICE_THIRD  3       /* a driver U7 no longer offers */

char MusicEnabled = 0;
char SfxEnabled = 0;

/* A patch count, three-byte address and four-byte patches; zero ends the sequence. */
unsigned char RolandPatchSetup[77] = {
	1, 3, 1, 32, 0, 90, 7, 0, 1, 3, 1, 52, 6, 100, 7, 1,
	1, 3, 2, 88, 1, 90, 5, 0, 12, 3, 2, 96, 1, 90, 6, 0,
	1, 90, 7, 0, 2, 100, 7, 1, 1, 90, 8, 0, 5, 90, 7, 1,
	1, 90, 9, 0, 3, 95, 7, 1, 4, 100, 4, 1, 4, 100, 5, 1,
	4, 100, 6, 1, 4, 100, 7, 1, 4, 100, 8, 1, 0
};
char MusicErrorFormat[] = "%s line#%d\n";      /* never referenced */

extern void (far pascal *DriverSysexEntry)(long, int, void far *);

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

void MusicSystem::start(unsigned char device, char *driverFile, char *timbreFile)
{
	struct ffblk found;

	if (findfirst(timbreFile, &found, 0) == -1)
		ReportFileNotFound(timbreFile);
	if (findfirst(driverFile, &found, 0) == -1)
		ReportFileNotFound(driverFile);
	if (device) {
		switch (device) {
		case MUSIC_DEVICE_MT32:
			MusicDevice = MUSIC_DEVICE_MT32;
			break;
		case MUSIC_DEVICE_ADLIB:
			MusicDevice = MUSIC_DEVICE_ADLIB;
			break;
		case MUSIC_DEVICE_THIRD:
			MusicDevice = MUSIC_DEVICE_THIRD;
			break;
		}
		StartSoundDriver(timbreFile, driverFile);
		if (MusicDevice == 0L)
			FatalError("Unable to initialize sound driver.\n"
				"Refer to your manual regarding\n"
				"sound board configuration.\n");
		if (MusicDevice == MUSIC_DEVICE_MT32)
			UploadRolandPatches();
	} else
		MusicDevice = 0;
}

void MusicSystem::stop()
{
	if (ready()) {
		FadeOutSong(0);
		StopSoundDriver();
		disable();
		UnhookInterrupt(8);
		UnhookInterrupt(8);
		enable();
		MusicDevice = 0;
	}
}

Song::Song(MusicSystem *music)
{
	data = 0;
	if (MusicEnabled == 0 || !music->ready())
		return;
}

Song::Song(MusicSystem *music, char *name)
{
	data = 0;
	if (MusicEnabled == 0 || !music->ready())
		return;
	load(name);
}

Song::Song(MusicSystem *music, char *flexName, int entry)
{
	data = 0;
	if (MusicEnabled == 0 || !music->ready())
		return;
	load(flexName, entry);
}

Song::~Song()
{
	if (MusicEnabled)
		FadeOutSong(0);
	if (data != 0)
		FreeFarHeap(data);
}

void Song::load(char *name)
{
	long size;

	if (MusicEnabled == 0)
		return;
	if (data != 0)
		FreeFarHeap(data);
	data = 0;
	if (!file.open(name, FILE_OPEN))
		ReportFileNotFound(name);
	size = file.getLength();
	data = (unsigned char far *)AllocateFarHeap(size, 0);
	if (data == 0)
		ReportOutOfFarMemory();
	file.read(data, size);
}

void Song::load(char *flexName, int entry)
{
	long size;

	if (MusicEnabled == 0)
		return;
	if (data != 0)
		FreeFarHeap(data);
	data = 0;
	Flex flex;
	if (!flex.open(flexName))
		ReportFileNotFound(flexName);
	FlexEntry where;
	flex.getEntry(entry, &where);
	size = where.size;
	data = (unsigned char far *)AllocateFarHeap(size, 0);
	if (data == 0)
		ReportOutOfFarMemory();
	flex.readEntry(&where, data, 0);
	flex.close();
}

void Song::play()
{
	if (data != 0 && MusicEnabled)
		PlaySong(data);
}

void Song::fadeOut(unsigned ticks)
{
	if (MusicEnabled)
		FadeOutSong(ticks);
}

unsigned char Song::finished()
{
	if (MusicEnabled)
		return MusicFlags & MUSIC_ENDED;
	return 1;
}

void Song::unload()
{
	if (data != 0) {
		FreeFarHeap(data);
		data = 0;
	}
}

void EnableMusic()
{
	MusicEnabled = 1;
}

void DisableMusic()
{
	MusicEnabled = 0;
}

SoundEffect::SoundEffect(SfxNote &n)
{
	note.flags = n.flags;
	note.patch = n.patch;
	note.pitch = n.pitch;
	note.velocity = n.velocity;
	note.duration = n.duration;
	note.slideTo = n.slideTo;
	note.chain = n.chain;
	setup(0, SFX_FULL_VOLUME, SFX_PAN_CENTER, 0, 1);
}

SoundEffect::SoundEffect(unsigned char how, unsigned char program, unsigned char key, unsigned char loudness,
	int ticks, unsigned char slide, unsigned char skip)
{
	note.flags = how;
	note.patch = program;
	note.pitch = key;
	note.velocity = loudness;
	note.duration = ticks;
	note.slideTo = slide;
	note.chain = skip;
	setup(0, SFX_FULL_VOLUME, SFX_PAN_CENTER, 0, 1);
}

void SoundEffect::setup(int f, int v, int p, int n, int level)
{
	flags = f;
	volume = v;
	pan = p;
	id = n;
	priority = level;
}

void SoundEffect::play(int n)
{
	if (n > 0)
		id = n;
	if (SfxEnabled)
		StartMidiSfx((unsigned char far *)&note, flags, volume, pan, id, priority);
}

void SoundEffect::stop()
{
	if (SfxEnabled)
		StopMidiSfx(0, id);
}

void EnableSfx()
{
	SfxEnabled = 1;
}

void DisableSfx()
{
	SfxEnabled = 0;
}
