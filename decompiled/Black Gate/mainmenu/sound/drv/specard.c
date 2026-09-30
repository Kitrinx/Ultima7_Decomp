/* Black Gate MAINMENU.EXE, resident segment 49 (file offsets 0x01540e to 0x015c3e, 2096 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <dos.h>
#include "lowlevel.h"
#include "dosio.h"
#include "init.h"
#include "oops.h"
#include "systimer.h"
#include "memapi.h"
#include "specard.h"
#include "cflxbuf.h"

/* Sound Blaster DSP ports, from the card's base port */
#define DSP_RESET             6
#define DSP_READ              10
#define DSP_WRITE             12        /* also its status: bit 7 set while busy */
#define DSP_READ_STATUS       14        /* bit 7 set when a byte can be read; reading acknowledges */
#define DSP_READY             0xaa      /* what a reset DSP answers */

/* DSP commands */
#define DSP_PLAY_8BIT         0x14
#define DSP_SET_TIME_CONSTANT 0x40
#define DSP_HALT_DMA          0xd0
#define DSP_SPEAKER_ON        0xd1
#define DSP_SPEAKER_OFF       0xd3

/* DMA controller registers, from its base port */
#define DMA_MASK              10
#define DMA_MODE              11
#define DMA_CLEAR_FLIPFLOP    12
#define DMA_MASK_ON           4
#define DMA_PLAY_MODE         0x48      /* single transfer, memory to card */

unsigned SpeechRate = 0;
unsigned DspTimeConstant = 0;
int CardPort = 0;
unsigned CardIrq = 7;
unsigned DmaHalfSize = 0;               /* bytes in each half of the DMA buffer */
char far *DmaBuffer = 0;
unsigned char DmaPage = 0;              /* the buffer's 64K page */
unsigned DmaFirstOffset = 0;            /* and its offset in that page */
unsigned DmaSecondOffset = 0;           /* the offset of its second half */
unsigned char DmaNextHalf = 0;          /* which half is refilled next */
unsigned DmaFirstCount = 0;
unsigned DmaSecondCount = 0;

long QueuedBlock = 0;
unsigned QueuedBlockSize = 0;
unsigned char BlockConsumed = 0;
unsigned char SpeechFinished = 1;
unsigned DmaBlockCount = 0;
unsigned char SpeechStreaming = 1;
SpeechCache *CurrentSpeech = 0;

unsigned DmaBasePort = 0;
unsigned DmaAddressPort = 2;
unsigned DmaCountPort = 3;
unsigned DmaChannel = 1;
int DmaPagePort = 0x83;
int DmaPagePorts[8] = { 0x87, 0x83, 0x81, 0x82, 0x8f, 0x8b, 0x89, 0x8a };
char CardErrorFormat[] = "%s line#%d";

SoundBlaster SpeechCard;
void (far *DmaDoneHandler)(void) = 0;

static void near WritePort(int port, unsigned char value)
{
	outportb(port, value);
}

static unsigned char near ReadPort(int port)
{
	return inportb(port);
}

/* Wait until the DSP can take a byte. */
static void near WaitDspReady(void)
{
	do {
		_DX = CardPort + DSP_WRITE;
		asm in      al, dx;
	} while ((char)(_AL & 0x80));
}

/* Mask the card's IRQ at the interrupt controller. */
void MaskCardIrq(void)
{
	if (CardIrq < 8)
		WritePort(0x21, ReadPort(0x21) | (1 << CardIrq));
	else
		WritePort(0xa1, ReadPort(0xa1) | (1 << (CardIrq - 8)));
}

/* Unmask it. */
void UnmaskCardIrq(void)
{
	if (CardIrq < 8)
		WritePort(0x21, ReadPort(0x21) & ~(1 << CardIrq));
	else
		WritePort(0xa1, ReadPort(0xa1) & ~(1 << (CardIrq - 8)));
}

/* End of interrupt. */
void AcknowledgeCardIrq(void)
{
	if (CardIrq >= 8)
		WritePort(0xa0, 0x20);
	WritePort(0x20, 0x20);
}

void SoundBlaster::init(unsigned size, unsigned rate, int port, unsigned irq, unsigned dma)
{
	if (!ready) {
		DmaHalfSize = size;
		SpeechRate = rate;
		CardPort = port;
		CardIrq = irq;
		setDmaChannel(dma);
		DspTimeConstant = 256 - 1000000L / SpeechRate;
		resetDsp();
		allocateDmaBuffer();
		ready = 1;
	}
}

void SoundBlaster::setDmaChannel(unsigned dma)
{
	DmaChannel = dma;
	DmaBasePort = 0;
	if (dma > 3) {
		DmaBasePort = 0xc0;
		dma -= 4;
	}
	DmaAddressPort = DmaBasePort + dma * 2;
	DmaCountPort = DmaAddressPort + 1;
	DmaPagePort = DmaPagePorts[dma];
}

void SoundBlaster::resetPlayback()
{
	DmaNextHalf = 0;
	SpeechFinished = 0;
	DmaBlockCount = 0;
	BlockConsumed = 0;
}

SoundBlaster::~SoundBlaster()
{
	shutdown();
	if (DmaBuffer)
		FreeFarHeap(DmaBuffer);
	ready = 0;
}

void SoundBlaster::setRate(unsigned rate)
{
	if (rate != 0)
		DspTimeConstant = 256 - 1000000L / rate;
}

void SoundBlaster::setTimeConstant(int constant)
{
	if (constant != 0)
		DspTimeConstant = constant;
}

/* Reset the DSP, hook its interrupt and turn the speaker on. */
void SoundBlaster::resetDsp()
{
	unsigned char found;
	int i;
	char *vector;
	Timer delay;

	WritePort(CardPort + DSP_RESET, 1);
	Timer_delay(&delay, 1L);
	WritePort(CardPort + DSP_RESET, 0);
	Timer_delay(&delay, 1L);
	found = 0;
	for (i = 0; i < 100; i++) {
		if (ReadPort(CardPort + DSP_READ_STATUS) & 0x80)
			if (ReadPort(CardPort + DSP_READ) == DSP_READY)
				found = 1;
	}
	if (!found)
		FatalError("Speech card initialization failed.");
	vector = new char[10];
	if (!vector)
		ReportOutOfFarMemory();
	disable();
	if (CardIrq < 8)
		HookInterruptVector(CardIrq + 8, (InterruptHandler)SoundIrqHandler, (HookRecord *)vector, 0);
	else
		HookInterruptVector(CardIrq + 0x68, (InterruptHandler)SoundIrqHandler, (HookRecord *)vector, 0);
	DmaDoneHandler = (void (far *)(void)) MK_FP(_CS, OnDmaDone);
	WaitDspReady();
	WritePort(CardPort + DSP_WRITE, DSP_SET_TIME_CONSTANT);
	WaitDspReady();
	WritePort(CardPort + DSP_WRITE, DspTimeConstant);
	WaitDspReady();
	WritePort(CardPort + DSP_WRITE, DSP_SPEAKER_ON);
	MaskCardIrq();
	enable();
}

/* Allocate a DMA buffer that does not cross a 64K page. */
void SoundBlaster::allocateDmaBuffer()
{
	void far *rejected;
	unsigned long start, end;

	DmaBuffer = (char far *)AllocateFarHeap(DmaHalfSize * 2, 0);
	if (!DmaBuffer)
		ReportOutOfFarMemory();
	rejected = 0;
	do {
		start = PointerToLinear(DmaBuffer);
		end = start + (unsigned long) DmaHalfSize * 2;
		if ((end & 0xffff0000L) != (start & 0xffff0000L)) {
			rejected = DmaBuffer;
			DmaBuffer = (char far *)AllocateFarHeap(DmaHalfSize * 2, 0);
			if (!DmaBuffer)
				ReportOutOfFarMemory();
			FreeFarHeap(rejected);
		} else
			rejected = 0;
	} while (rejected);
	DmaPage = start >> 16;
	DmaFirstOffset = start & 0xffff;
	DmaSecondOffset = DmaFirstOffset + DmaHalfSize;
}

void SoundBlaster::shutdown()
{
	if (ready != 0) {
		WritePort(CardPort + DSP_WRITE, DSP_SPEAKER_OFF);
		WritePort(CardPort + DSP_WRITE, DSP_HALT_DMA);
		MaskCardIrq();
		if (CardIrq < 8)
			UnhookInterrupt(CardIrq + 8);
		else
			UnhookInterrupt(CardIrq + 0x68);
	}
}

void SoundBlaster::stop()
{
	WritePort(CardPort + DSP_WRITE, DSP_SPEAKER_OFF);
	WritePort(CardPort + DSP_WRITE, DSP_HALT_DMA);
	MaskCardIrq();
}

/* Start playing a sound through the first half of the DMA buffer. */
void SoundBlaster::play(SpeechCache *sound)
{
	if (!(unsigned char)sound->primed)
		return;
	setRate(sound->rate);
	SpeechStreaming = sound->streaming;
	CurrentSpeech = sound;
	disable();
	WaitDspReady();
	WritePort(CardPort + DSP_WRITE, DSP_SET_TIME_CONSTANT);
	WaitDspReady();
	WritePort(CardPort + DSP_WRITE, DspTimeConstant);
	WaitDspReady();
	WritePort(CardPort + DSP_WRITE, DSP_SPEAKER_ON);
	DmaBlockCount = 0;
	SpeechFinished = 0;
	sound->fillDoubleBuffer(DmaBuffer, &DmaFirstCount, &DmaSecondCount);
	WritePort(DmaBasePort + DMA_MASK, DmaChannel | DMA_MASK_ON);
	WritePort(DmaBasePort + DMA_CLEAR_FLIPFLOP, 0);
	WritePort(DmaBasePort + DMA_MODE, DmaChannel | DMA_PLAY_MODE);
	WritePort(DmaAddressPort, DmaFirstOffset);
	WritePort(DmaAddressPort, DmaFirstOffset >> 8);
	WritePort(DmaPagePort, DmaPage);
	WritePort(DmaCountPort, DmaFirstCount);
	WritePort(DmaCountPort, DmaFirstCount >> 8);
	WritePort(DmaBasePort + DMA_MASK, DmaChannel);
	UnmaskCardIrq();
	WaitDspReady();
	WritePort(CardPort + DSP_WRITE, DSP_PLAY_8BIT);
	WaitDspReady();
	WritePort(CardPort + DSP_WRITE, DmaFirstCount - 1);
	WaitDspReady();
	WritePort(CardPort + DSP_WRITE, (DmaFirstCount - 1) >> 8);
	enable();
}

/* The DMA completion handler: play the half just filled and refill the other. */
void OnDmaDone(void)
{
	unsigned address, count;
	unsigned length, fill;

	ReadPort(CardPort + DSP_READ_STATUS);
	if (QueuedBlockSize == 0) {
		WaitDspReady();
		AcknowledgeCardIrq();
		CurrentSpeech->stop();
		SpeechFinished = 1;
		return;
	}
	if (DmaNextHalf == 0) {
		if (DmaFirstCount < DmaHalfSize) {
			WaitDspReady();
			AcknowledgeCardIrq();
			BlockConsumed = 0;
			CurrentSpeech->stop();
			SpeechFinished = 1;
			return;
		}
		address = DmaSecondOffset;
		count = DmaSecondCount;
		fill = 0;
	} else {
		if (DmaSecondCount < DmaHalfSize) {
			WaitDspReady();
			AcknowledgeCardIrq();
			BlockConsumed = 0;
			CurrentSpeech->stop();
			SpeechFinished = 1;
			return;
		}
		address = DmaFirstOffset;
		count = DmaFirstCount;
		fill = DmaHalfSize;
	}
	WritePort(DmaBasePort + DMA_MASK, DmaChannel | DMA_MASK_ON);
	WritePort(DmaBasePort + DMA_CLEAR_FLIPFLOP, 0);
	WritePort(DmaBasePort + DMA_MODE, DmaChannel | DMA_PLAY_MODE);
	WritePort(DmaAddressPort, address);
	WritePort(DmaAddressPort, address >> 8);
	WritePort(DmaPagePort, DmaPage);
	WritePort(DmaCountPort, count);
	WritePort(DmaCountPort, count >> 8);
	WritePort(DmaBasePort + DMA_MASK, DmaChannel);
	WaitDspReady();
	WritePort(CardPort + DSP_WRITE, DSP_PLAY_8BIT);
	WaitDspReady();
	WritePort(CardPort + DSP_WRITE, count - 1);
	WaitDspReady();
	WritePort(CardPort + DSP_WRITE, (count - 1) >> 8);
	if (SpeechStreaming == 1) {
		MoveLinear(PointerToLinear(DmaBuffer), QueuedBlock, (unsigned long) QueuedBlockSize, 0x111);
		if (QueuedBlockSize < DmaHalfSize)
			--QueuedBlockSize;
		if (DmaNextHalf == 0) {
			DmaFirstCount = QueuedBlockSize;
			DmaNextHalf = 1;
		} else {
			DmaSecondCount = QueuedBlockSize;
			DmaNextHalf = 0;
		}
		BlockConsumed = 1;
	} else {
		length = CurrentSpeech->readBytes(DmaBuffer + fill, DmaHalfSize);
		if (length != 0 && length < DmaHalfSize)
			--length;
		if (DmaNextHalf == 0) {
			DmaFirstCount = length;
			DmaNextHalf = 1;
		} else {
			DmaSecondCount = length;
			DmaNextHalf = 0;
		}
	}
	AcknowledgeCardIrq();
}

void SoundBlaster::queueBlock(long source, unsigned count)
{
	QueuedBlock = source;
	QueuedBlockSize = count;
}

void SoundBlaster::fail(char *message)
{
	shutdown();
	FatalError(message);
}
