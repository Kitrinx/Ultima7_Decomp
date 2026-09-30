#ifndef VOICE_H
#define VOICE_H

struct BorrowedSpeechCache;

/* Digitized speech, played through the sound card. */
struct Speech {
	int16_t bufferSize, rate, port, irq, dma, format;

	Speech(int16_t, int16_t, int16_t, int16_t, int16_t);
	~Speech();
	void reportError(int16_t);
	int16_t isPlaying();
	int16_t start();
	void pause();
	void stop();
	void playFile(char *, int16_t);
	void play(char *, int16_t);
	int16_t selectTrack(int16_t);
	void continuePlaying();
};

void ContinuePlayingSpeech(void);

extern uint8_t SpeechOn;
extern uint8_t SpeechCardStarted;
extern uint8_t SpeechCardConfigured;
extern uint8_t SpeechStarted;
extern uint8_t SpeechSpriteShown;
extern int16_t SpeechSprite;
extern BorrowedSpeechCache SpeechStream;
uint8_t IsSpeechPlaying(void);

#endif
