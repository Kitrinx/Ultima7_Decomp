/* Black Gate INTRO.EXE, resident segment 3 (file offsets 0x009439 to 0x0096f7, 702 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "chkfile.h"
#include "flex.h"
#include "oops.h"
#include "synctrk.h"

/* What next() answers once the track has run out. */
static SyncCue NoCue = { 0, 0 };

SyncTrack::SyncTrack(int length)
{
	cues = new SyncCue[length];
	if (!cues)
		ReportOutOfNearMemory();
	current = 0;
	count = length;
	loaded = 0;
}

SyncTrack::SyncTrack(char *name)
{
	DataFile file(name, FILE_OPEN);
	unsigned long length = file.getLength();

	count = length / sizeof(SyncCue);
	cues = new SyncCue[count];
	if (!cues)
		ReportOutOfNearMemory();
	current = 0;
	loaded = 1;
	file.read(cues, length);
}

SyncTrack::SyncTrack(char *flexName, int entry)
{
	FlexEntry info;
	unsigned long length;
	Flex flex;

	flex.open(flexName);
	flex.getEntry(entry, &info);
	length = info.size;
	count = length / sizeof(SyncCue);
	cues = new SyncCue[count];
	if (!cues)
		ReportOutOfNearMemory();
	current = 0;
	loaded = 1;
	flex.readEntry(&info, cues, 0);
	flex.close();
}

SyncTrack::~SyncTrack()
{
	if (cues)
		delete cues;
}

void SyncTrack::save(char *name)
{
	DataFile file(name, FILE_CREATE);

	for (int i = 0; i < current; i++)
		file.write(&cues[i], sizeof(SyncCue));
}

void SyncTrack::add(unsigned time, unsigned char value)
{
	if (current + 1 <= count) {
		cues[current].time = time;
		cues[current].value = value;
		current++;
	}
}

/* Step past every cue the time has reached; the cue after them, or an empty one at the end. */
SyncCue *SyncTrack::next(unsigned time)
{
	current++;
	while (current <= count && cues[current].time <= time)
		current++;
	if (current <= count)
		return &cues[current];
	return &NoCue;
}
