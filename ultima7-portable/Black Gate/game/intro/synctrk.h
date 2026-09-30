#ifndef INTRO_SYNCTRK_H
#define INTRO_SYNCTRK_H

namespace Intro {

/* A value that takes effect at a time; three bytes a cue in the file. */
struct SyncCue {
	uint16_t time;
	uint8_t value;
};

/* Cues in time order, recorded one by one or read whole from a file or a Flex entry, then
 * played back by stepping to the cue a time has reached. */
struct SyncTrack {
	SyncCue *cues;
	int16_t current;
	int16_t count;
	uint8_t loaded;             /* read from a file */
	SyncTrack(int16_t length);
	SyncTrack(char *name);
	SyncTrack(char *flexName, int16_t entry);
	~SyncTrack();
	void save(char *name);
	void add(uint16_t time, uint8_t value);
	SyncCue *next(uint16_t time);
};

}

#endif
