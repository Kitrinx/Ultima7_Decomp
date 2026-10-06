#ifndef VOICE_H
#define VOICE_H

#include "flex.h"
#include "ail.h"

struct BorrowedSpeechCache;

/* Digitized speech, played through an AIL digital driver. */
struct Speech {
	HDRIVER driver;
	long unused;
	void far *driverImage;
	drvr_desc far *description;
	sound_buff next;            /* what is left of the entry */
	sound_buff block;           /* the block handed to the driver */
	void far *buffers[2];
	Flex drivers;
	int bufferSize, rate, port, irq, dma, format;

	Speech(int, int, int, int, int);
	~Speech();
	int isPlaying();
	int isFinished();
	int start();
	void pause();
	void stop();
	void playFile(char *, int);
	void play(char *, int);
	void continuePlaying();
	int selectTrack(int);
};

void ContinuePlayingSpeech(void);
unsigned char IsSpeechPlaying(void);

extern unsigned char SpeechOn;
extern unsigned char SpeechCardStarted;
extern unsigned char SpeechCardConfigured;
extern unsigned char AlternateSpeechDriver;
extern unsigned char SpeechStarted;
extern unsigned char SpeechSpriteShown;
extern int SpeechSprite;
extern BorrowedSpeechCache SpeechStream;

#endif
