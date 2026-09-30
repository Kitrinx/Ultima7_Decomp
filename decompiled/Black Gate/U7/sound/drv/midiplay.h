#ifndef MIDIPLAY_H
#define MIDIPLAY_H

struct SoundDriver;

struct Voice;

/* What MusicDevice holds; 0 is no music. */
#define MUSIC_DEVICE_MT32   1
#define MUSIC_DEVICE_ADLIB  2

extern int MusicDevice;
extern unsigned char far *CurrentSong;
extern int SongStopped;
extern int MusicFlags;
#define MUSIC_ENDED         1       /* the song stopped, ended or jumped back */
#define MUSIC_STARTED       2
#define MUSIC_RETURN        4       /* go back to the interrupted song after this one */
int PlaySong(unsigned char far *song);
Voice far *StartMidiSfx(unsigned char far *data, int flags, int volume, int pan, int id, int priority);
void SetMidiSfxControl(Voice far *v, int control, int value, int id);
void StopMidiSfxVoice(Voice far *v, int id);
void DropAllMidiSfx(void);
void FadeOutSong(unsigned ticks);
void QueueSong(unsigned char far *song, int flags);
void far SetMusicVolume(int volume);

extern unsigned far *SoundDriverImage;
extern SoundDriver far *SoundDriverInfo;
extern void far *TimbreBank;
extern char unused_global_7[20];
extern int SongFormat;
extern int TrackCount;
extern int TicksPerBeat;
extern int MidiSfxReady;
extern int SongMarker;
extern int SongBranchValue;
extern int SongFromStart;
extern int MusicOn;
extern int SongVolume;
extern int MusicVolume;
extern int FadeStep;
extern int FadeLevel;
extern int FadeKeepsSong;
extern int SongExitPending;
extern unsigned char far *QueuedSong;
extern unsigned char ChannelProgram[11];
extern unsigned char ChannelVolume[11];
extern unsigned char ChannelPan[11];
extern unsigned char SongProgram[11];
extern unsigned long TickLength;
extern long SongClock;
extern unsigned char PercussionChannel[88];
extern unsigned char PercussionNote[88];
void far FreeMidiChannel(int n);
int far AllocateMidiChannel(int priority);
void StartSfxNote(int n);
void TickMidiSfx(void);
void ReturnChannelToSong(int channel);
void StopMidiSfx(Voice far *v, int id);
long far ReadVarLength(int track);
int MatchChunkId(unsigned char far *p, char *id);
int GetChunkLength(unsigned char far *p);
void HandleMetaEvent(int track, int type, int len);
void AddTrackDelta(int track, long delta);
int far NextDueTrack(void);
int PlayDueEvents(void);
void StopSong(void);
void ApplySongVolume(int volume);
void interrupt MusicTimerHandler(...);
void RewindSong(int only);
void far *far ReadFlexRecord(char *name, int n, int flags);
int LoadSoundDriver(char *timbres, char *driver);
void StartSoundDriver(char *timbres, char *driver);
void StopSoundDriver(void);
void SilenceAllSound(void);
int far OpenSong(unsigned char far *song);
void ChangeSong(unsigned char far *song, unsigned fade, int flags);
void WaitForMusicFlags(unsigned mask);
void EnableMusic(int on);

#endif
