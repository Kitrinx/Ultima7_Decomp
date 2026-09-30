#ifndef MIDIPLAY_H
#define MIDIPLAY_H

/* What the driver's describe entry returns. */
struct SoundDriver {
	uint8_t keepTimbres;        /* the driver still needs the timbre bank after init */
	int16_t first;              /* the first timbre slot free after the bank */
};

struct Voice;

/* What MusicDevice holds; 0 is no music. */
#define MUSIC_DEVICE_MT32   1
#define MUSIC_DEVICE_ADLIB  2

extern int16_t MusicDevice;
extern uint8_t *CurrentSong;
extern int16_t SongStopped;
extern int16_t MusicFlags;
#define MUSIC_ENDED         1       /* the song stopped, ended or jumped back */
#define MUSIC_STARTED       2
#define MUSIC_RETURN        4       /* go back to the interrupted song after this one */
int16_t PlaySong(uint8_t *song);
Voice *StartMidiSfx(uint8_t *data, int16_t flags, int16_t volume, int16_t pan, int16_t id, int16_t priority);
void SetMidiSfxControl(Voice *v, int16_t control, int16_t value, int16_t id);
void StopMidiSfxVoice(Voice *v, int16_t id);
void DropAllMidiSfx(void);
void FadeOutSong(uint16_t ticks);
void QueueSong(uint8_t *song, int16_t flags);
void SetMusicVolume(int16_t volume);

extern SoundDriver *SoundDriverInfo;
extern void *TimbreBank;
extern char unused_global_7[20];
extern int16_t SongFormat;
extern int16_t TrackCount;
extern int16_t TicksPerBeat;
extern int16_t MidiSfxReady;
extern int16_t SongMarker;
extern int16_t SongBranchValue;
extern int16_t SongFromStart;
extern int16_t MusicOn;
extern int16_t SongVolume;
extern int16_t MusicVolume;
extern int16_t FadeStep;
extern int16_t FadeLevel;
extern int16_t FadeKeepsSong;
extern int16_t SongExitPending;
extern uint8_t *QueuedSong;
extern uint8_t ChannelProgram[11];
extern uint8_t ChannelVolume[11];
extern uint8_t ChannelPan[11];
extern uint8_t SongProgram[11];
extern uint32_t TickLength;
extern int32_t SongClock;
extern uint8_t PercussionChannel[88];
extern uint8_t PercussionNote[88];
void FreeMidiChannel(int16_t n);
int16_t AllocateMidiChannel(int16_t priority);
void StartSfxNote(int16_t n);
void TickMidiSfx(void);
void ReturnChannelToSong(int16_t channel);
void StopMidiSfx(Voice *v, int16_t id);
int32_t ReadVarLength(int16_t track);
int16_t MatchChunkId(uint8_t *p, char *id);
int16_t GetChunkLength(uint8_t *p);
void HandleMetaEvent(int16_t track, int16_t type, int16_t len);
void AddTrackDelta(int16_t track, int32_t delta);
int16_t NextDueTrack(void);
int16_t PlayDueEvents(void);
void StopSong(void);
void ApplySongVolume(int16_t volume);
void MusicTimerHandler(void);
extern void (*SoundTickFirst)(void);
void RewindSong(int16_t only);
void * ReadFlexRecord(char *name, int16_t n, int16_t flags);
int16_t LoadSoundDriver(char *timbres, char *driver);
void StartSoundDriver(char *timbres, char *driver);
void StopSoundDriver(void);
void SilenceAllSound(void);
int16_t OpenSong(uint8_t *song);
void ChangeSong(uint8_t *song, uint16_t fade, int16_t flags);
void WaitForMusicFlags(uint16_t mask);
void EnableMusic(int16_t on);

#endif
