/* Black Gate INTRO.EXE, synctrk.c: the cues that move the Guardian's face with his speech. */

#include "u7port.h"
#include "chkfile.h"
#include "flex.h"
#include "oops.h"
#include "synctrk.h"

namespace Intro {

/* What next() answers once the track has run out. */
static SyncCue NoCue = { 0, 0 };

/* The cues are followed by one of NoCue's: next() reads the entry just past the last. */
static SyncCue *NewCues(int16_t count)
{
	SyncCue *cues = new SyncCue[count + 1];

	cues[count] = NoCue;
	return cues;
}

SyncTrack::SyncTrack(int16_t length)
{
	cues = NewCues(length);
	current = 0;
	count = length;
	loaded = 0;
}

SyncTrack::SyncTrack(char *name)
{
	DataFile file(name, FILE_OPEN);
	uint32_t length = file.getLength();

	count = (int16_t) (length / sizeof(SyncCue));
	cues = NewCues(count);
	current = 0;
	loaded = 1;
	file.read(cues, length);
}

SyncTrack::SyncTrack(char *flexName, int16_t entry)
{
	FlexEntry info;
	uint32_t length;
	Flex flex;

	flex.open(flexName);
	flex.getEntry(entry, &info);
	length = info.size;
	count = (int16_t) (length / sizeof(SyncCue));
	cues = NewCues(count);
	current = 0;
	loaded = 1;
	flex.readEntry(&info, cues, 0);
	flex.close();
}

SyncTrack::~SyncTrack()
{
	if (cues)
		delete[] cues;
}

void SyncTrack::save(char *name)
{
	DataFile file(name, FILE_CREATE);

	for (int16_t i = 0; i < current; i++)
		file.write(&cues[i], sizeof(SyncCue));
}

void SyncTrack::add(uint16_t time, uint8_t value)
{
	if (current + 1 <= count) {
		cues[current].time = time;
		cues[current].value = value;
		current++;
	}
}

/* Step past every cue the time has reached; the cue after them, or an empty one at the end. */
SyncCue *SyncTrack::next(uint16_t time)
{
	current++;
	while (current <= count && cues[current].time <= time)
		current++;
	if (current <= count)
		return &cues[current];
	return &NoCue;
}

}

extern "C" void ResetIntroSynctrkGlobals(void)
{
	memset(&Intro::NoCue, 0, sizeof Intro::NoCue);
}
