/* The Miles AIL 2.14 Sound Blaster digital driver (SBDIG.ADV, SBPDIG.ADV), rewritten natively
 * over the platform's speech output.
 *
 * Two ways to play. Buffer mode: the program registers up to two buffers and refills each as
 * it is played; buffer 0 goes before 1 when both wait. VOC mode: a whole Creative Voice file
 * from a marker on. Samples go to the platform as soon as they are known, so a buffer's status
 * follows from how much of the output has been played: the sound tick and every status call
 * bring it up to date.
 */

#include "u7port.h"
#include "plat.h"
#include "aildrv.h"

#define BUF_MODE        0
#define VOC_MODE        1

/* Creative Voice block types. */
#define VOC_END         0
#define VOC_SOUND       1
#define VOC_MORE_SOUND  2
#define VOC_SILENCE     3
#define VOC_MARKER      4
#define VOC_REPEAT      6
#define VOC_END_REPEAT  7
#define VOC_EXTENDED    8

struct SoundBuffer {
	uint16_t status;
	uint16_t rate;              /* time constant */
	const uint8_t *data;
	uint32_t length;
	uint32_t fed;               /* bytes given to the output */
	uint32_t end;               /* output position once all of it is played */
	uint8_t queued;             /* handed to the output */
};

static const char DeviceNames[] = "Creative Labs Sound Blaster(TM) Digital Sound\0";

static drvr_desc Description;
static uint8_t Initialized;
static uint8_t Mode;
static uint16_t DacStatus;
static SoundBuffer Buffers[2];
static int16_t Order[2];        /* buffers handed to the output, the playing one first */
static int16_t OrderCount;
static uint32_t Queued;         /* samples given to the platform since it was started */
static uint16_t OutputRate;     /* the time constant the platform plays at; 0 when stopped */

/* VOC mode: where the file has got to. */
static const uint8_t *Block;
static const uint8_t *Data;     /* samples left in the current block */
static uint32_t DataLeft;
static uint32_t SilenceLeft;
static const uint8_t *LoopStart;
static uint16_t LoopCount;
static uint8_t ExtendedPending;
static uint16_t ExtendedRate;
static uint8_t VocFinished;     /* all of it queued */

/* Compressed sound, expanded as the card's DSP expanded it. */
static uint8_t AdpcmReference;
static uint8_t AdpcmStep;
static uint16_t VocPack;        /* the current VOC block's pack type */
static uint8_t *Expanded[3];    /* the two buffers and the VOC block, expanded */
static uint32_t ExpandedSize[3];

static uint32_t Played(void)
{
	return Queued - (uint32_t) plat_pcm_pending();
}

/* Starts the output afresh at a new rate; the same rate carries on. */
static void Output(uint16_t timeConstant)
{
	if (OutputRate == timeConstant)
		return;
	plat_pcm_start(INT32_C(1000000) / (256 - (timeConstant & 0xff)));
	OutputRate = timeConstant;
	Queued = 0;
}

static uint32_t Queue(const uint8_t *samples, uint32_t count)
{
	uint32_t taken = 0;

	while (taken < count) {
		int32_t piece = count - taken > 0x4000 ? 0x4000 : (int32_t) (count - taken);
		int32_t n = plat_pcm_queue(samples + taken, piece);

		taken += (uint32_t) n;
		Queued += (uint32_t) n;
		if (n < piece)
			break;
	}
	return taken;
}

static void Silence(void)
{
	plat_pcm_stop();
	OutputRate = 0;
	Queued = 0;
}

/* ---- Compressed sound ---- */

/* One code of a 4, 3 (2.6) or 2-bit ADPCM sample: the low bits scale a step from the last
 * sample, the top bit gives its sign, and the step size moves with each code. */
static uint8_t AdpcmSample(uint8_t code, int16_t bits)
{
	static const uint8_t change2[] = {0, 1, 1, 3, 2, 6, 4, 12, 8, 24, 16, 48};
	static const uint8_t step2[] = {0, 2, 0, 4, 2, 6, 4, 8, 6, 10, 8, 10};
	static const uint8_t change3[] = {0, 1, 2, 3, 1, 3, 5, 7, 2, 6, 10, 14, 4, 12, 20, 28,
		8, 24, 40, 56};
	static const uint8_t step3[] = {0, 0, 0, 4, 0, 4, 4, 8, 4, 8, 8, 12, 8, 12, 12, 16,
		12, 16, 16, 16};
	static const uint8_t change4[] = {0, 1, 2, 3, 4, 5, 6, 7, 1, 3, 5, 7, 9, 11, 13, 15,
		2, 6, 10, 14, 18, 22, 26, 30, 4, 12, 20, 28, 36, 44, 52, 60};
	static const uint8_t step4[] = {0, 0, 0, 0, 0, 8, 8, 8, 0, 8, 8, 8, 8, 16, 16, 16,
		8, 16, 16, 16, 16, 24, 24, 24, 16, 24, 24, 24, 24, 24, 24, 24};
	int16_t sample = AdpcmReference;
	uint8_t index;

	if (bits == 2) {
		index = (code & 1) | AdpcmStep;
		sample += (code & 2) ? -change2[index] : change2[index];
		AdpcmStep = step2[index];
	} else if (bits == 3) {
		index = (code & 3) | AdpcmStep;
		sample += (code & 4) ? -change3[index] : change3[index];
		AdpcmStep = step3[index];
	} else {
		index = (code & 7) | AdpcmStep;
		sample += (code & 8) ? -change4[index] : change4[index];
		AdpcmStep = step4[index];
	}
	AdpcmReference = (uint8_t) (sample < 0 ? 0 : sample > 255 ? 255 : sample);
	return AdpcmReference;
}

/* Expands count bytes of sound into slot's buffer as 8-bit samples; returns how many. pack is
 * the driver's DSP command index: 0 and 4 plain 8-bit, 1-3 ADPCM starting with a reference
 * byte, 5-7 ADPCM going on from the last sample. */
static uint32_t Expand(int16_t slot, const uint8_t *in, uint32_t count, uint16_t pack)
{
	uint8_t *out;
	uint32_t n = 0;

	if (ExpandedSize[slot] < count * 4) {
		delete[] Expanded[slot];
		ExpandedSize[slot] = count * 4;
		Expanded[slot] = new uint8_t[ExpandedSize[slot]];
	}
	out = Expanded[slot];
	if (count && pack < 4) {
		AdpcmReference = *in++;
		AdpcmStep = 0;
		out[n++] = AdpcmReference;
		count--;
	}
	for (; count > 0; count--, in++) {
		switch (pack & 3) {
		case 1:
			out[n++] = AdpcmSample(*in >> 4, 4);
			out[n++] = AdpcmSample(*in & 15, 4);
			break;
		case 2:
			out[n++] = AdpcmSample(*in >> 5, 3);
			out[n++] = AdpcmSample((*in >> 2) & 7, 3);
			out[n++] = AdpcmSample(((*in & 2) << 1) | (*in & 1), 3);
			break;
		case 3:
			out[n++] = AdpcmSample(*in >> 6, 2);
			out[n++] = AdpcmSample((*in >> 4) & 3, 2);
			out[n++] = AdpcmSample((*in >> 2) & 3, 2);
			out[n++] = AdpcmSample(*in & 3, 2);
			break;
		}
	}
	return n;
}

/* ---- Buffer mode ---- */

/* The first registered buffer not yet handed to the output, or -1. */
static int16_t NextBuffer(void)
{
	if (Buffers[0].status == DAC_STOPPED && !Buffers[0].queued)
		return 0;
	if (Buffers[1].status == DAC_STOPPED && !Buffers[1].queued)
		return 1;
	return -1;
}

/* Hands waiting buffers to the output behind the playing one, feeds them as the output has
 * room, and moves the playing one on as each ends. A buffer at another rate waits until the
 * output is idle. */
static void ServeBuffers(void)
{
	int16_t next;

	if (DacStatus != DAC_PLAYING)
		return;
	while (OrderCount < 2 && (next = NextBuffer()) != -1) {
		SoundBuffer *b = &Buffers[next];

		if (OrderCount == 0)
			Output(b->rate);
		else if (b->rate != OutputRate)
			break;
		b->queued = 1;
		b->fed = 0;
		Order[OrderCount++] = next;
		if (OrderCount == 1)
			b->status = DAC_PLAYING;
	}
	for (int16_t i = 0; i < OrderCount; i++) {
		SoundBuffer *b = &Buffers[Order[i]];

		if (b->fed < b->length) {
			b->fed += Queue(b->data + b->fed, b->length - b->fed);
			b->end = Queued;
			if (b->fed < b->length)
				break;
		}
	}
	while (OrderCount > 0) {
		SoundBuffer *b = &Buffers[Order[0]];

		if (b->fed < b->length || Played() < b->end)
			return;
		b->status = DAC_DONE;
		b->queued = 0;
		Order[0] = Order[1];
		if (--OrderCount > 0)
			Buffers[Order[0]].status = DAC_PLAYING;
		else if (NextBuffer() == -1)
			DacStatus = DAC_DONE;
		else
			ServeBuffers();
	}
}

/* ---- VOC mode ---- */

static uint32_t BlockLength(const uint8_t *block)
{
	return block[1] | (uint32_t) block[2] << 8 | (uint32_t) block[3] << 16;
}

static const uint8_t *NextBlock(const uint8_t *block)
{
	return block + 4 + BlockLength(block);
}

/* Moves on to the next block with sound or silence; a marker or the end finishes the file. */
static void ProcessBlock(void)
{
	for (;;) {
		const uint8_t *block = Block;
		uint16_t rate;

		switch (block[0]) {
		case VOC_END:
		case VOC_MARKER:
			VocFinished = 1;
			return;
		case VOC_SOUND:
			rate = block[4];
			if (ExtendedPending) {
				rate = ExtendedRate;
				ExtendedPending = 0;
			}
			Output(rate);
			Data = block + 6;
			DataLeft = BlockLength(block) - 2;
			VocPack = block[5] & 0x7f;
			if (VocPack & 3) {
				DataLeft = Expand(2, Data, DataLeft, VocPack);
				Data = Expanded[2];
				if (VocPack < 4)
					VocPack += 4;
			}
			return;
		case VOC_MORE_SOUND:
			Data = block + 4;
			DataLeft = BlockLength(block);
			if (VocPack & 3) {
				DataLeft = Expand(2, Data, DataLeft, VocPack);
				Data = Expanded[2];
			}
			return;
		case VOC_SILENCE:
			Output(block[6]);
			SilenceLeft = (uint32_t) (block[4] | block[5] << 8) + 1;
			return;
		case VOC_REPEAT:
			LoopCount = (uint16_t) (block[4] | block[5] << 8);
			Block = NextBlock(block);
			LoopStart = Block;
			continue;
		case VOC_END_REPEAT:
			if (LoopCount != 0) {
				Block = LoopStart;
				if (LoopCount != 0xffff)
					LoopCount--;
				continue;
			}
			break;
		case VOC_EXTENDED:
			/* the time constant's high byte, as a mono sample would give it */
			ExtendedRate = block[5];
			ExtendedPending = 1;
			break;
		}
		Block = NextBlock(block);
	}
}

static void ServeVoc(void)
{
	if (DacStatus != DAC_PLAYING)
		return;
	while (!VocFinished) {
		if (SilenceLeft) {
			uint8_t quiet[256];
			uint32_t n = SilenceLeft < sizeof quiet ? SilenceLeft : sizeof quiet;
			uint32_t taken;

			memset(quiet, 0x80, sizeof quiet);
			taken = Queue(quiet, n);
			SilenceLeft -= taken;
			if (taken < n)
				return;
		} else if (DataLeft) {
			uint32_t taken = Queue(Data, DataLeft);

			Data += taken;
			DataLeft -= taken;
			if (DataLeft)
				return;
		} else {
			Block = NextBlock(Block);
			ProcessBlock();
		}
	}
	if (Played() >= Queued)
		DacStatus = DAC_DONE;
}

static void Serve(void)
{
	if (Mode == VOC_MODE)
		ServeVoc();
	else
		ServeBuffers();
}

/* ---- Driver ---- */

drvr_desc *DigitalDescribe(void)
{
	Description.min_API_version = 200;
	Description.drvr_type = DSP_DRVR;
	memcpy(Description.data_suffix, "VOC", 4);
	Description.dev_name_table = DeviceNames;
	Description.default_IO = 0x220;
	Description.default_IRQ = 7;
	Description.default_DMA = 1;
	Description.default_DRQ = -1;
	Description.service_rate = -1;
	Description.display_size = 0;
	return &Description;
}

uint16_t DigitalDetect(void)
{
	return 1;
}

static void Stop(void)
{
	DacStatus = DAC_STOPPED;
	Silence();
	for (int16_t i = 0; i < 2; i++) {
		Buffers[i].status = DAC_DONE;
		Buffers[i].queued = 0;
	}
	OrderCount = 0;
}

void DigitalInit(void)
{
	Mode = BUF_MODE;
	Stop();
	ExtendedPending = 0;
	Initialized = 1;
}

void DigitalShutdown(void)
{
	if (!Initialized)
		return;
	Stop();
	Initialized = 0;
}

void DigitalServe(void)
{
	if (Initialized)
		Serve();
}

/* ---- AIL calls ---- */

#define DIGITAL_CALL(driver, failed) \
	do { if (AilDriverKind(driver) != AIL_KIND_DIGITAL) return failed; } while (0)

void AIL_register_sound_buffer(HDRIVER driver, uint16_t buffer_num, sound_buff *buff)
{
	SoundBuffer *b;

	DIGITAL_CALL(driver, );
	if (buffer_num > 1)
		return;
	plat_sound_lock();
	if (Mode == VOC_MODE) {
		Stop();
		Mode = BUF_MODE;
	}
	b = &Buffers[buffer_num];
	b->rate = buff->sample_rate;
	b->data = (const uint8_t *) buff->data;
	b->length = buff->len;
	if ((buff->pack_type & 0x7f & 3) != 0) {
		b->length = Expand(buffer_num, b->data, b->length, buff->pack_type & 0x7f);
		b->data = Expanded[buffer_num];
	}
	b->queued = 0;
	b->status = DAC_STOPPED;
	Serve();
	plat_sound_unlock();
}

uint16_t AIL_sound_buffer_status(HDRIVER driver, uint16_t buffer_num)
{
	uint16_t status;

	DIGITAL_CALL(driver, 0);
	if (buffer_num > 1)
		return 0;
	plat_sound_lock();
	Serve();
	status = Buffers[buffer_num].status;
	plat_sound_unlock();
	return status;
}

/* From the first block, or from the given marker. */
void AIL_play_VOC_file(HDRIVER driver, void *VOC_file, int16_t block_marker)
{
	const uint8_t *file = (const uint8_t *) VOC_file;

	DIGITAL_CALL(driver, );
	plat_sound_lock();
	ExtendedPending = 0;
	Stop();
	Mode = VOC_MODE;
	DacStatus = DAC_DONE;
	Block = file + (file[0x14] | file[0x15] << 8);
	if (block_marker != -1) {
		for (;;) {
			const uint8_t *block = Block;

			if (block[0] == VOC_END) {
				plat_sound_unlock();
				return;
			}
			if (block[0] == VOC_EXTENDED) {
				ExtendedRate = block[5];
				ExtendedPending = 1;
			}
			Block = NextBlock(block);
			if (block[0] == VOC_MARKER && (int16_t) (block[4] | block[5] << 8) == block_marker)
				break;
		}
	}
	DacStatus = DAC_STOPPED;
	plat_sound_unlock();
}

uint16_t AIL_VOC_playback_status(HDRIVER driver)
{
	uint16_t status;

	DIGITAL_CALL(driver, 0);
	plat_sound_lock();
	Serve();
	status = DacStatus;
	plat_sound_unlock();
	return status;
}

void AIL_start_digital_playback(HDRIVER driver)
{
	DIGITAL_CALL(driver, );
	plat_sound_lock();
	if (Mode == VOC_MODE) {
		if (DacStatus == DAC_STOPPED) {
			DacStatus = DAC_PLAYING;
			Data = 0;
			DataLeft = 0;
			SilenceLeft = 0;
			VocFinished = 0;
			ProcessBlock();
		}
	} else if (DacStatus != DAC_PLAYING && NextBuffer() != -1) {
		DacStatus = DAC_PLAYING;
		OrderCount = 0;
	}
	Serve();
	plat_sound_unlock();
}

void AIL_stop_digital_playback(HDRIVER driver)
{
	DIGITAL_CALL(driver, );
	plat_sound_lock();
	Stop();
	plat_sound_unlock();
}

extern "C" void ResetAildigGlobals(void)
{
	memset(&Description, 0, sizeof Description);
	Initialized = 0;
	Mode = BUF_MODE;
	DacStatus = 0;
	memset(Buffers, 0, sizeof Buffers);
	memset(Order, 0, sizeof Order);
	OrderCount = 0;
	Queued = 0;
	OutputRate = 0;
	Block = 0;
	Data = 0;
	DataLeft = 0;
	SilenceLeft = 0;
	LoopStart = 0;
	LoopCount = 0;
	ExtendedPending = 0;
	ExtendedRate = 0;
	VocFinished = 0;
	AdpcmReference = 0;
	AdpcmStep = 0;
	VocPack = 0;
	for (int16_t i = 0; i < 3; i++) {
		delete[] Expanded[i];
		Expanded[i] = 0;
		ExpandedSize[i] = 0;
	}
}
