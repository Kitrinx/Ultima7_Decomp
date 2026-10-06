#ifndef SPECARD_H
#define SPECARD_H

struct SpeechCache;

extern unsigned char DmaNextHalf, BlockConsumed, SpeechFinished;

/* The speech card. */
class SoundBlaster {
	unsigned char ready;
public:
	SoundBlaster() { ready = 0; DmaNextHalf = 0; SpeechFinished = 1; BlockConsumed = 0; }
	~SoundBlaster();
	void init(unsigned size, unsigned rate, int port, unsigned irq, unsigned dma);
	void setDmaChannel(unsigned dma);
	void resetPlayback();
	void setRate(unsigned rate);
	void setTimeConstant(int constant);
	void resetDsp();
	void allocateDmaBuffer();
	void shutdown();
	void stop();
	void play(SpeechCache *sound);
	void queueBlock(long source, unsigned count);
	void fail(char *message);
};

extern SoundBlaster SpeechCard;

extern unsigned DmaHalfSize;
extern char far *DmaBuffer;
extern unsigned char DmaPage;
extern unsigned DmaFirstOffset;
extern unsigned DmaSecondOffset;
extern unsigned DmaFirstCount;
extern unsigned DmaSecondCount;
extern long QueuedBlock;
extern unsigned QueuedBlockSize;
extern unsigned DmaBlockCount;
extern unsigned char SpeechStreaming;
extern SpeechCache *CurrentSpeech;
extern unsigned DmaBasePort;
extern unsigned DmaAddressPort;
extern unsigned DmaCountPort;
extern unsigned DmaChannel;
extern int DmaPagePort;
extern int DmaPagePorts[8];
extern char CardErrorFormat[];
void MaskCardIrq(void);
void UnmaskCardIrq(void);
void AcknowledgeCardIrq(void);
void OnDmaDone(void);

#ifdef __cplusplus
extern "C" {
#endif
void far SoundIrqHandler(void);
#ifdef __cplusplus
}
#endif

extern unsigned SpeechRate;
extern unsigned DspTimeConstant;
extern int CardPort;
extern unsigned CardIrq;

#endif
