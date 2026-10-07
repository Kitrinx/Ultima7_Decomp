#ifndef VOICE_H
#define VOICE_H

#include "flex.h"
#include "ail.h"

struct BorrowedSpeechCache;

/* Digitized speech, played through an AIL digital driver. */
struct Speech {
	HDRIVER driver;
	int32_t unused;
	void *driverImage;
	drvr_desc *description;
	sound_buff next;            /* what is left of the entry */
	sound_buff block;           /* the block handed to the driver */
	void *buffers[2];
	Flex drivers;
	int16_t bufferSize, rate, port, irq, dma, format;

	Speech(int16_t, int16_t, int16_t, int16_t, int16_t);
	~Speech();
	int16_t isPlaying();
	int16_t isFinished();
	int16_t start();
	void pause();
	void stop();
	void playFile(char *, int16_t);
	void play(char *, int16_t);
	void continuePlaying();
	int16_t selectTrack(int16_t);
};

void ContinuePlayingSpeech(void);
uint8_t IsSpeechPlaying(void);

extern uint8_t SpeechOn;
extern uint8_t SpeechCardStarted;
extern uint8_t SpeechCardConfigured;
extern uint8_t AlternateSpeechDriver;
extern uint8_t SpeechStarted;
extern uint8_t SpeechSpriteShown;
extern int16_t SpeechSprite;
extern BorrowedSpeechCache SpeechStream;

#endif
