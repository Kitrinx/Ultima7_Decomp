#ifndef VOICE_H
#define VOICE_H

struct BorrowedSpeechCache;

/* Digitized speech, played through the sound card. */
struct Speech {
	int bufferSize, rate, port, irq, dma, format;

	Speech(int, int, int, int, int);
	~Speech();
	void reportError(int);
	int isPlaying();
	int start();
	void pause();
	void stop();
	void playFile(char *, int);
	void play(char *, int);
	int selectTrack(int);
	void continuePlaying();
};

void ContinuePlayingSpeech(void);

extern unsigned char SpeechOn;
extern unsigned char SpeechCardStarted;
extern unsigned char SpeechCardConfigured;
extern unsigned char SpeechStarted;
extern unsigned char SpeechSpriteShown;
extern int SpeechSprite;
extern BorrowedSpeechCache SpeechStream;
unsigned char IsSpeechPlaying(void);

#endif
