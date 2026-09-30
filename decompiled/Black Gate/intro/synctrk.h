#ifndef SYNCTRK_H
#define SYNCTRK_H

/* A value that takes effect at a time. */
struct SyncCue {
	unsigned time;
	unsigned char value;
};

/* Cues in time order, recorded one by one or read whole from a file or a Flex entry, then
 * played back by stepping to the cue a time has reached. */
struct SyncTrack {
	SyncCue *cues;
	int current;
	int count;
	unsigned char loaded;       /* read from a file */
	SyncTrack(int length);
	SyncTrack(char *name);
	SyncTrack(char *flexName, int entry);
	~SyncTrack();
	void save(char *name);
	void add(unsigned time, unsigned char value);
	SyncCue *next(unsigned time);
};

#endif
