#ifndef SPECARD_H
#define SPECARD_H

struct SpeechCache;

extern uint8_t DmaNextHalf, BlockConsumed, SpeechFinished;

/* The speech card. */
class SoundBlaster {
	uint8_t ready;
public:
	SoundBlaster() { ready = 0; DmaNextHalf = 0; SpeechFinished = 1; BlockConsumed = 0; }
	~SoundBlaster();
	void init(uint16_t size, uint16_t rate, int16_t port, uint16_t irq, uint16_t dma);
	void resetPlayback();
	void setRate(uint16_t rate);
	void setTimeConstant(int16_t constant);
	void allocateDmaBuffer();
	void shutdown();
	void stop();
	void play(SpeechCache *sound);
	void feed();
	void queueBlock(int32_t source, uint16_t count);
	void fail(char *message);
};

extern SoundBlaster SpeechCard;

extern uint16_t DmaHalfSize;
extern char *DmaBuffer;
extern uint16_t DmaFirstCount;
extern uint16_t DmaSecondCount;
extern int32_t QueuedBlock;
extern uint16_t QueuedBlockSize;
extern uint16_t DmaBlockCount;
extern uint8_t SpeechStreaming;
extern SpeechCache *CurrentSpeech;
extern uint16_t DmaChannel;
extern char CardErrorFormat[];
void OnDmaDone(void);

extern uint16_t SpeechRate;
extern uint16_t DspTimeConstant;
extern int16_t CardPort;
extern uint16_t CardIrq;

#endif
