/* The synthesizer half of the Miles AIL 2.14 XMIDI driver for the Ad Lib (ADLIB.ADV, YAMAHA.INC
 * built for one YM3812), rewritten natively, with Origin's time-variant effects (TVFX) the game's
 * build of the driver carries. It writes the platform's OPL chip.
 *
 *   16 slots (virtual voices) share 9 hardware voices: a note takes a free slot and, if one is
 *   free, a voice; otherwise the voice priorities decide who sounds.
 *   Timbres: 14-byte .BNK instruments, or TVFX timbres whose frequency, levels, multipliers,
 *   feedback, wave select and priority follow small command lists, refreshed at 60 Hz.
 *   The timbre cache lives in memory the program gives the driver, packed, least recently
 *   used first out. Timbres are read as the driver read them, at 16-bit offsets in the cache's
 *   segment: a command list that points past the end of the segment wraps to its start.
 *
 * The 1991 build of the driver (the intro's and the ending's ADLIB.ADV) differs in places, marked
 * Old: it skips register writes that would not change the register, voices are searched from
 * the first, a slot loses its timbre only when its note ends, and a few smaller things.
 */

#include "u7port.h"
#include "plat.h"
#include "ailxmidi.h"
#include "arena.h"
#include "dosio.h"

#define NUM_CHANS       16
#define NUM_VOICES      9
#define NUM_SLOTS       16
#define MAX_TIMBS       192
#define DEF_TC_SIZE     3584
#define DEF_SYNTH_VOL   100
#define DEF_PITCH_RANGE 12
#define DEF_PITCH_L     0x00
#define DEF_PITCH_H     0x40
#define QUANT_RATE      120

#define MODULATION      1
#define PART_VOLUME     7
#define PANPOT          10
#define EXPRESSION      11
#define SUSTAIN         64
#define RESET_ALL_CTRLS 121
#define ALL_NOTES_OFF   123
#define VOICE_PROTECT   112
#define TIMBRE_PROTECT  113
#define PATCH_BANK_SEL  114

/* S_status */
#define FREE            0
#define KEYON           1
#define KEYOFF          2

/* S_type */
#define BNK_INST        0
#define TV_INST         1
#define TV_EFFECT       2
#define OPL3_INST       3

/* S_update: the registers a slot's voice needs written */
#define U_ALL_REGS      0xf9
#define U_AVEKM         0x80
#define U_KSLTL         0x40
#define U_ADSR          0x20
#define U_WS            0x10
#define U_FBC           0x08
#define U_FREQ          0x01

#define TIMB_IN_USE     0x80
#define TIMB_PROTECTED  0x40

#define BNK_SIZE        14
#define OPL3BNK_SIZE    25

/* The TVFX parameters, each a command list with a position, a step count, a value and a step. */
#define TV_F            0       /* frequency */
#define TV_V0           1       /* modulator level */
#define TV_V1           2       /* carrier level */
#define TV_P            3       /* priority */
#define TV_FB           4       /* feedback */
#define TV_M0           5       /* modulator multiplier */
#define TV_M1           6       /* carrier multiplier */
#define TV_WS           7       /* wave selects */
#define TV_PARAMS       8

static const uint16_t FreqTable[192] = {
	0x02b2, 0x02b4, 0x02b7, 0x02b9, 0x02bc, 0x02be, 0x02c1, 0x02c3,
	0x02c6, 0x02c9, 0x02cb, 0x02ce, 0x02d0, 0x02d3, 0x02d6, 0x02d8,
	0x02db, 0x02dd, 0x02e0, 0x02e3, 0x02e5, 0x02e8, 0x02eb, 0x02ed,
	0x02f0, 0x02f3, 0x02f6, 0x02f8, 0x02fb, 0x02fe, 0x0301, 0x0303,
	0x0306, 0x0309, 0x030c, 0x030f, 0x0311, 0x0314, 0x0317, 0x031a,
	0x031d, 0x0320, 0x0323, 0x0326, 0x0329, 0x032b, 0x032e, 0x0331,
	0x0334, 0x0337, 0x033a, 0x033d, 0x0340, 0x0343, 0x0346, 0x0349,
	0x034c, 0x034f, 0x0352, 0x0356, 0x0359, 0x035c, 0x035f, 0x0362,
	0x0365, 0x0368, 0x036b, 0x036f, 0x0372, 0x0375, 0x0378, 0x037b,
	0x037f, 0x0382, 0x0385, 0x0388, 0x038c, 0x038f, 0x0392, 0x0395,
	0x0399, 0x039c, 0x039f, 0x03a3, 0x03a6, 0x03a9, 0x03ad, 0x03b0,
	0x03b4, 0x03b7, 0x03bb, 0x03be, 0x03c1, 0x03c5, 0x03c8, 0x03cc,
	0x03cf, 0x03d3, 0x03d7, 0x03da, 0x03de, 0x03e1, 0x03e5, 0x03e8,
	0x03ec, 0x03f0, 0x03f3, 0x03f7, 0x03fb, 0x03fe, 0xfe01, 0xfe03,
	0xfe05, 0xfe07, 0xfe08, 0xfe0a, 0xfe0c, 0xfe0e, 0xfe10, 0xfe12,
	0xfe14, 0xfe16, 0xfe18, 0xfe1a, 0xfe1c, 0xfe1e, 0xfe20, 0xfe21,
	0xfe23, 0xfe25, 0xfe27, 0xfe29, 0xfe2b, 0xfe2d, 0xfe2f, 0xfe31,
	0xfe34, 0xfe36, 0xfe38, 0xfe3a, 0xfe3c, 0xfe3e, 0xfe40, 0xfe42,
	0xfe44, 0xfe46, 0xfe48, 0xfe4a, 0xfe4c, 0xfe4f, 0xfe51, 0xfe53,
	0xfe55, 0xfe57, 0xfe59, 0xfe5c, 0xfe5e, 0xfe60, 0xfe62, 0xfe64,
	0xfe67, 0xfe69, 0xfe6b, 0xfe6d, 0xfe6f, 0xfe72, 0xfe74, 0xfe76,
	0xfe79, 0xfe7b, 0xfe7d, 0xfe7f, 0xfe82, 0xfe84, 0xfe86, 0xfe89,
	0xfe8b, 0xfe8d, 0xfe90, 0xfe92, 0xfe95, 0xfe97, 0xfe99, 0xfe9c,
	0xfe9e, 0xfea1, 0xfea3, 0xfea5, 0xfea8, 0xfeaa, 0xfead, 0xfeaf
};
static const uint8_t NoteOctave[96] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};
static const uint8_t NoteHalftone[96] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};
static const uint8_t Array0Init[245] = {
	32, 0, 0, 96, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63,
	63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
	63, 63, 63, 63, 63, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 192, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0
};
static const uint8_t VelGraph[16] = {
	82, 85, 88, 91, 94, 97, 100, 103, 106, 109, 112, 115, 118, 121, 124, 127
};

static const uint8_t Op0[18] = { 0, 1, 2, 6, 7, 8, 12, 13, 14, 18, 19, 20, 24, 25, 26, 30, 31, 32 };
static const uint8_t Op1[18] = { 3, 4, 5, 9, 10, 11, 15, 16, 17, 21, 22, 23, 27, 28, 29, 33, 34, 35 };
static const uint8_t OpIndex[18] = { 0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, 20, 21 };
static const uint8_t VoiceNum[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
static const char DeviceNames[] = "Ad Lib(R) Music Synthesizer Card\0";

static drvr_desc Description;
static uint8_t Old;                     /* the 1991 driver */
static uint16_t AddrStat;               /* the chip's address/status port */
static uint16_t DataOut;
static uint8_t Shadow[256];             /* Old: the registers as last written */

static uint32_t NoteEvent;
static uint32_t TimbHist[MAX_TIMBS];
static uint16_t TimbOffsets[MAX_TIMBS];
static uint8_t TimbBank[MAX_TIMBS];
static uint8_t TimbNum[MAX_TIMBS];
static uint8_t TimbAttribs[MAX_TIMBS];

static uint32_t CacheSegment;           /* the linear address of the cache's segment */
static uint16_t CacheOffset;            /* the cache's offset in it */
static uint16_t CacheSize;
static uint16_t CacheEnd;

static uint16_t TvAccum;                /* the 60 Hz TVFX refresh, from the 120 Hz service */
static uint16_t PriAccum;               /* the 5 Hz priority check */
static uint8_t VolUpdate;               /* TVFX levels go out every other refresh */
static uint16_t Rover;                  /* the voice the last search stopped at */

static uint16_t STimbre[NUM_SLOTS];     /* the timbre's offset in the cache segment */
static uint16_t SDuration[NUM_SLOTS];
static uint8_t SStatus[NUM_SLOTS];
static uint8_t SType[NUM_SLOTS];
static uint8_t SVoice[NUM_SLOTS];
static uint8_t SChannel[NUM_SLOTS];
static uint8_t SNote[NUM_SLOTS];
static uint8_t SKeynum[NUM_SLOTS];
static uint8_t STranspose[NUM_SLOTS];
static uint8_t SVelocity[NUM_SLOTS];
static uint8_t SSustain[NUM_SLOTS];
static uint8_t SUpdate[NUM_SLOTS];
static uint8_t SKbfShadow[NUM_SLOTS];
static uint8_t SBlock[NUM_SLOTS];
static uint8_t SFbc[NUM_SLOTS];
static uint8_t SKsltl0[NUM_SLOTS];
static uint8_t SKsltl1[NUM_SLOTS];
static uint8_t SAvekm0[NUM_SLOTS];
static uint8_t SAvekm1[NUM_SLOTS];
static uint8_t SAd0[NUM_SLOTS];
static uint8_t SAd1[NUM_SLOTS];
static uint8_t SSr0[NUM_SLOTS];
static uint8_t SSr1[NUM_SLOTS];
static uint8_t SScale01[NUM_SLOTS];

/* TVFX state per parameter and slot; Value is what the registers are made from, for every
 * timbre type. */
static uint16_t TvPos[TV_PARAMS][NUM_SLOTS];
static uint16_t TvCount[TV_PARAMS][NUM_SLOTS];
static uint16_t TvValue[TV_PARAMS][NUM_SLOTS];
static uint16_t TvStep[TV_PARAMS][NUM_SLOTS];

static uint8_t MidiVol[NUM_CHANS];
static uint8_t MidiPan[NUM_CHANS];
static uint8_t MidiPitchL[NUM_CHANS];
static uint8_t MidiPitchH[NUM_CHANS];
static uint8_t MidiExpress[NUM_CHANS];
static uint8_t MidiMod[NUM_CHANS];
static uint8_t MidiSus[NUM_CHANS];
static uint8_t MidiVprot[NUM_CHANS];
static uint8_t MidiTimbre[NUM_CHANS];
static uint8_t MidiBank[NUM_CHANS];
static uint8_t MidiProgram[NUM_CHANS];
static uint8_t RbsTimbres[128];
static uint8_t MidiVoices[NUM_CHANS];
static uint8_t VChannel[NUM_VOICES];
static uint16_t SVPriority[NUM_SLOTS];

static void UpdateVoice(int16_t slot);
static void NoteOff(uint8_t chan, uint8_t note);

static uint16_t ReadWord(const uint8_t *p)
{
	return (uint16_t) (p[0] | p[1] << 8);
}

/* A byte or word at an offset in the cache's segment. */
static uint8_t SegmentByte(uint16_t offset)
{
	return LinearGet8((int32_t) (CacheSegment + offset));
}

static uint16_t SegmentWord(uint16_t offset)
{
	return (uint16_t) (SegmentByte(offset) | SegmentByte((uint16_t) (offset + 1)) << 8);
}

static uint8_t *SegmentPointer(uint16_t offset)
{
	return LINEAR(CacheSegment + offset);
}

/* A byte or word of the timbre at an offset in the cache segment. */
static uint8_t TimbreByte(uint16_t timbre, uint16_t at)
{
	return SegmentByte((uint16_t) (timbre + at));
}

static uint16_t TimbreWord(uint16_t timbre, uint16_t at)
{
	return SegmentWord((uint16_t) (timbre + at));
}

/* ---- Chip access ---- */

/* The driver read the status port 6 and 42 times after the address and the data, for the chip's
 * settling time; only detection depends on that time. */
static void UpdateReg(uint8_t reg, uint8_t value)
{
	Shadow[reg] = value;
	plat_opl_write(AddrStat, reg);
	plat_opl_write(AddrStat + 1, value);
}

/* Old: a write that would leave the register as it is is skipped. */
static void WriteVoiceReg(uint8_t reg, uint8_t value)
{
	if (Old && Shadow[reg] == value)
		return;
	UpdateReg(reg, value);
}

static void WriteRegister(uint8_t op, uint8_t base, uint8_t value)
{
	WriteVoiceReg((uint8_t) (OpIndex[op] + base), value);
}

static void SendByte(uint8_t voice, uint8_t base, uint8_t value)
{
	WriteVoiceReg((uint8_t) (VoiceNum[voice] + base), value);
}

static void DetectSend(uint8_t reg, uint8_t value)
{
	plat_opl_write(AddrStat, reg);
	for (int16_t i = 0; i < 6; i++)
		plat_opl_read(AddrStat);
	plat_opl_write(DataOut, value);
	for (int16_t i = 0; i < 42; i++)
		plat_opl_read(AddrStat);
}

/* The timers of a YM3812: idle, then timer 1 overflowing within 100 microseconds. */
static uint16_t DetectAdlib(void)
{
	uint8_t before, after = 0;

	/* Old: the registers detection writes are taken as cleared */
	if (Old) {
		Shadow[4] = 0;
		Shadow[2] = 0;
	}
	DetectSend(4, 0x60);
	DetectSend(4, 0x80);
	before = plat_opl_read(AddrStat);
	DetectSend(2, 0xff);
	DetectSend(4, 0x21);
	for (int16_t i = 200; i > 0; i--)
		after = plat_opl_read(AddrStat);
	DetectSend(4, 0x60);
	DetectSend(4, 0x80);
	return (before & 0xe0) == 0 && (after & 0xe0) == 0xc0;
}

static void ResetSynth(void)
{
	for (uint16_t reg = 1; reg <= 0xf5; reg++)
		UpdateReg((uint8_t) reg, Array0Init[reg - 1]);
}

/* ---- Timbre cache ---- */

static int16_t IndexTimbre(uint16_t global)
{
	uint8_t bank = (uint8_t) (global >> 8), patch = (uint8_t) global;

	for (int16_t i = 0; i < MAX_TIMBS; i++) {
		if ((TimbAttribs[i] & TIMB_IN_USE) && TimbBank[i] == bank && TimbNum[i] == patch)
			return i;
	}
	return -1;
}

static void ReleaseVoice(int16_t slot);

/* Removes the least recently used unprotected timbre and closes the gap after it. */
static void DeleteLru(void)
{
	int16_t index = -1;
	uint32_t oldest = UINT32_MAX;
	uint16_t size, offset, timbre;

	for (int16_t i = 0; i < MAX_TIMBS; i++) {
		if (!(TimbAttribs[i] & TIMB_IN_USE) || (TimbAttribs[i] & TIMB_PROTECTED))
			continue;
		if (TimbHist[i] <= oldest) {
			oldest = TimbHist[i];
			index = i;
		}
	}
	if (index == -1)
		return;
	offset = TimbOffsets[index];
	timbre = (uint16_t) (CacheOffset + offset);
	size = SegmentWord(timbre);
	memmove(SegmentPointer(timbre), SegmentPointer((uint16_t) (timbre + size)),
		(uint16_t) (CacheEnd - (uint16_t) (offset + size)));
	TimbAttribs[index] = 0;
	CacheEnd -= size;
	/* Old: channels and playing slots keep pointing where the timbre was */
	if (Old) {
		for (int16_t i = 0; i < MAX_TIMBS; i++) {
			if ((TimbAttribs[i] & TIMB_IN_USE) && TimbOffsets[i] > offset)
				TimbOffsets[i] -= size;
		}
		return;
	}
	for (int16_t chan = 0; chan < NUM_CHANS; chan++) {
		if (MidiTimbre[chan] != 0xff && MidiTimbre[chan] == index)
			MidiTimbre[chan] = 0xff;
	}
	for (int16_t key = 0; key < 128; key++) {
		if (RbsTimbres[key] == index)
			RbsTimbres[key] = 0xff;
	}
	for (int16_t i = 0; i < MAX_TIMBS; i++) {
		if ((TimbAttribs[i] & TIMB_IN_USE) && TimbOffsets[i] > offset)
			TimbOffsets[i] -= size;
	}
	for (int16_t slot = 0; slot < NUM_SLOTS; slot++) {
		if (SStatus[slot] == FREE || STimbre[slot] < timbre)
			continue;
		if (STimbre[slot] == timbre) {
			ReleaseVoice(slot);
			SStatus[slot] = FREE;
		} else
			STimbre[slot] -= size;
	}
}

/* Copies the timbre into the cache, making room as needed, and points the channels that asked
 * for it at it. */
static void InstallTimbre(uint8_t bank, uint8_t patch, const uint8_t *timbre)
{
	int16_t index = IndexTimbre((uint16_t) (bank << 8 | patch));
	int16_t slot;
	uint16_t end;

	if (index == -1) {
		if (timbre == 0)
			return;
		for (;;) {
			for (slot = 0; slot < MAX_TIMBS && (TimbAttribs[slot] & TIMB_IN_USE); slot++)
				;
			if (slot < MAX_TIMBS) {
				end = (uint16_t) (ReadWord(timbre) + CacheEnd);
				if (end <= CacheSize)
					break;
			}
			DeleteLru();
		}
		index = slot;
		TimbNum[slot] = patch;
		TimbBank[slot] = bank;
		TimbAttribs[slot] = TIMB_IN_USE;
		TimbHist[slot] = NoteEvent++;
		TimbOffsets[slot] = CacheEnd;
		memcpy(SegmentPointer((uint16_t) (CacheOffset + CacheEnd)), timbre, ReadWord(timbre));
		CacheEnd = end;
	}
	for (int16_t chan = 0; chan < NUM_CHANS; chan++) {
		if (MidiProgram[chan] == patch && MidiBank[chan] == bank)
			MidiTimbre[chan] = (uint8_t) index;
	}
}

static uint16_t TimbreRequest(const uint8_t *timb)
{
	if (timb == 0 || memcmp(timb, "TIMB", 4) != 0)
		return 0xffff;
	const uint8_t *entry = timb + 8;
	uint16_t count = ReadWord(entry);

	do {
		entry += 2;
		if (IndexTimbre(ReadWord(entry)) == -1)
			return ReadWord(entry);
	} while (--count != 0);
	return 0xffff;
}

/* The timbre's offset in the cache plus one (Old: its index plus one), or 0. */
static uint16_t TimbreStatus(uint8_t bank, uint8_t patch)
{
	int16_t index = IndexTimbre((uint16_t) (bank << 8 | patch));

	if (index == -1 || Old)
		return (uint16_t) (index + 1);
	return (uint16_t) (TimbOffsets[index] + 1);
}

/* Bank and patch 0xff stand for every timbre. */
static void ProtectTimbre(uint8_t bank, uint8_t patch, uint8_t on)
{
	int16_t first = 0, end = MAX_TIMBS;

	if (bank != 0xff || patch != 0xff) {
		first = IndexTimbre((uint16_t) (bank << 8 | patch));
		if (first == -1)
			return;
		end = first + 1;
	}
	for (int16_t i = first; i < end; i++) {
		if (on)
			TimbAttribs[i] |= TIMB_PROTECTED;
		else
			TimbAttribs[i] &= ~TIMB_PROTECTED;
	}
}

static void DefineCache(uint8_t *base, uint16_t size)
{
	uint32_t linear = PointerToLinear(base);

	CacheSegment = linear & ~UINT32_C(15);
	CacheOffset = (uint16_t) (linear & 15);
	CacheSize = size;
	CacheEnd = 0;
}

/* ---- Voices ---- */

static void InitSynth(void)
{
	NoteEvent = 0;
	memset(TimbAttribs, 0, sizeof TimbAttribs);
	memset(MidiTimbre, 0xff, sizeof MidiTimbre);
	memset(MidiVoices, 0, sizeof MidiVoices);
	memset(MidiProgram, 0xff, sizeof MidiProgram);
	memset(MidiBank, 0, sizeof MidiBank);
	memset(SStatus, FREE, sizeof SStatus);
	memset(VChannel, 0xff, sizeof VChannel);
	memset(RbsTimbres, 0xff, sizeof RbsTimbres);
	TvAccum = 0;
	PriAccum = 0;
	VolUpdate = 0;
	Rover = 0xffff;
}

/* Gives a slot's voice back: the note is keyed off, and a .BNK slot ends with it. */
static void ReleaseVoice(int16_t slot)
{
	if (SVoice[slot] == 0xff)
		return;
	SBlock[slot] &= 0xdf;
	SUpdate[slot] |= U_FREQ;
	UpdateVoice(slot);
	MidiVoices[SChannel[slot]]--;
	VChannel[SVoice[slot]] = 0xff;
	SVoice[slot] = 0xff;
	if (!Old && (SType[slot] == OPL3_INST || SType[slot] == BNK_INST))
		SStatus[slot] = FREE;
}

/* Hands voices to the highest-priority slots without one, taking them from the lowest. Old: only
 * when there are more slots than voices. */
static void UpdatePriority(void)
{
	int16_t slotCount = 0, high = 0, low = 0;

	for (int16_t slot = 0; slot < NUM_SLOTS; slot++) {
		uint8_t chan;
		uint16_t priority;

		if (SStatus[slot] == FREE)
			continue;
		slotCount++;
		chan = SChannel[slot] & 0x0f;
		priority = (int8_t) MidiVprot[chan] >= 64 ? 0xffff : TvValue[TV_P][slot];
		priority = priority < MidiVoices[chan] ? 0 : (uint16_t) (priority - MidiVoices[chan]);
		SVPriority[slot] = priority;
	}
	if (Old && slotCount <= NUM_VOICES)
		return;
	for (;;) {
		uint16_t highest = 0, lowest = 0xffff;
		uint8_t voice;

		for (int16_t slot = 0; slot < NUM_SLOTS; slot++) {
			uint16_t priority = SVPriority[slot];

			if (SStatus[slot] == FREE)
				continue;
			if (SVoice[slot] == 0xff) {
				if (priority >= highest) {
					highest = priority;
					high = slot;
				}
			} else if (priority <= lowest) {
				lowest = priority;
				low = slot;
			}
		}
		if (highest < lowest || (!Old && highest == 0))
			return;
		voice = SVoice[low];
		ReleaseVoice(low);
		if (Old && SType[low] == BNK_INST)
			SStatus[low] = FREE;
		SVoice[high] = voice;
		MidiVoices[SChannel[high]]++;
		VChannel[voice] = SChannel[high];
		SUpdate[high] = U_ALL_REGS;
		UpdateVoice(high);
		if (--slotCount == 0)
			return;
	}
}

/* The next free voice after the last one handed out (Old: the first free one), else the
 * priority search. */
static void AssignVoice(int16_t slot)
{
	uint16_t voice = Old ? 0xffff : Rover;

	for (int16_t tries = 0; tries < NUM_VOICES; tries++) {
		if (++voice == NUM_VOICES)
			voice = 0;
		if (!Old)
			Rover = voice;
		if (VChannel[voice] != 0xff)
			continue;
		SVoice[slot] = (uint8_t) voice;
		MidiVoices[SChannel[slot]]++;
		VChannel[voice] = SChannel[slot];
		SUpdate[slot] = U_ALL_REGS;
		UpdateVoice(slot);
		return;
	}
	UpdatePriority();
}

/* (x * y) / 128 as the driver rounded it: anything below one unit but above zero becomes one
 * more. */
static uint8_t ScaleVolume(uint8_t x, uint8_t y)
{
	uint8_t v = (uint8_t) ((uint16_t) ((uint16_t) (x * y) << 1) >> 8);

	return v != 0 ? (uint8_t) (v + 1) : 0;
}

/* Writes the registers the slot's update flags ask for. */
static void UpdateVoice(int16_t slot)
{
	uint8_t voice, voice0, voice1, vol = 0, chan;
	uint16_t fNum;

	if (SVoice[slot] == 0xff)
		return;
	chan = SChannel[slot] & 0x0f;
	if (SUpdate[slot] & U_KSLTL)
		vol = ScaleVolume(ScaleVolume(MidiVol[chan], MidiExpress[chan]), SVelocity[slot]);
	voice = SVoice[slot];
	voice0 = Op0[voice];
	voice1 = Op1[voice];
	if (SUpdate[slot] & U_AVEKM) {
		uint8_t vib = (int8_t) MidiMod[chan] >= 64 ? 0x40 : 0;

		WriteRegister(voice0, 0x20, (uint8_t) ((TvValue[TV_M0][slot] >> 12) | vib | SAvekm0[slot]));
		WriteRegister(voice1, 0x20, (uint8_t) ((TvValue[TV_M1][slot] >> 12) | vib | SAvekm1[slot]));
		SUpdate[slot] &= ~U_AVEKM;
	}
	if (SUpdate[slot] & U_KSLTL) {
		uint8_t level = (uint8_t) (TvValue[TV_V0][slot] >> 10);

		if (SScale01[slot] & 1)
			level = (uint8_t) (level * vol / 127);
		WriteRegister(voice0, 0x40, (uint8_t) ((~level & 0x3f) | SKsltl0[slot]));
		level = (uint8_t) (TvValue[TV_V1][slot] >> 10);
		if (SScale01[slot] & 2)
			level = (uint8_t) (level * vol / 127);
		WriteRegister(voice1, 0x40, (uint8_t) ((~level & 0x3f) | SKsltl1[slot]));
		SUpdate[slot] &= ~U_KSLTL;
	}
	if (SUpdate[slot] & U_ADSR) {
		WriteRegister(voice0, 0x60, SAd0[slot]);
		WriteRegister(voice1, 0x60, SAd1[slot]);
		WriteRegister(voice0, 0x80, SSr0[slot]);
		WriteRegister(voice1, 0x80, SSr1[slot]);
		SUpdate[slot] &= ~U_ADSR;
	}
	if (SUpdate[slot] & U_WS) {
		WriteRegister(voice1, 0xe0, (uint8_t) TvValue[TV_WS][slot]);
		WriteRegister(voice0, 0xe0, (uint8_t) (TvValue[TV_WS][slot] >> 8));
		SUpdate[slot] &= ~U_WS;
	}
	if (SUpdate[slot] & U_FBC) {
		SendByte(voice, 0xc0, (uint8_t) (((TvValue[TV_FB][slot] >> 12) & 0x0e) |
			(Old ? SFbc[slot] : SFbc[slot] & 1)));
		SUpdate[slot] &= ~U_FBC;
	}
	if (!(SUpdate[slot] & U_FREQ))
		return;
	if (SType[slot] == TV_EFFECT)
		fNum = (uint16_t) (TvValue[TV_F][slot] >> 6);
	else if (!(SBlock[slot] & 0x20)) {
		SendByte(voice, 0xb0, SKbfShadow[slot] & 0xdf);
		SUpdate[slot] &= ~U_FREQ;
		return;
	} else {
		int16_t bend, note, pitch, halftone;
		uint8_t block;
		int16_t entry;

		bend = (int16_t) ((MidiPitchH[SChannel[slot]] << 7 | MidiPitchL[SChannel[slot]]) - 0x2000);
		bend = (int16_t) ((bend >> 5) * DEF_PITCH_RANGE);
		note = (int16_t) (SNote[slot] + (int8_t) STranspose[slot] - 24);
		do
			note += 12;
		while (note < 0);
		note += 12;
		do
			note -= 12;
		while (note > 95);
		pitch = (int16_t) (bend + (uint16_t) ((uint8_t) note << 8));
		pitch = (int16_t) ((int16_t) (pitch + 8) >> 4);
		pitch -= 12 * 16;
		do
			pitch += 12 * 16;
		while (pitch < 0);
		pitch += 12 * 16;
		do
			pitch -= 12 * 16;
		while (pitch > 96 * 16 - 1);
		halftone = NoteHalftone[pitch >> 4];
		entry = (int16_t) (halftone * 16 + (pitch & 15));
		fNum = FreqTable[entry];
		block = (uint8_t) (NoteOctave[pitch >> 4] - 1);
		if ((int16_t) fNum < 0)
			block++;
		if ((int8_t) block < 0) {
			block++;
			fNum = (uint16_t) ((int16_t) fNum >> 1);
		}
		fNum = (uint16_t) ((fNum & 0xff) | (((fNum >> 8) & 3) | (uint8_t) (block << 2)) << 8);
	}
	SendByte(voice, 0xa0, (uint8_t) fNum);
	SKbfShadow[slot] = (uint8_t) ((fNum >> 8) | SBlock[slot]);
	SendByte(voice, 0xb0, SKbfShadow[slot]);
	SUpdate[slot] &= ~U_FREQ;
}

static void BnkPhase(int16_t slot)
{
	uint16_t t = STimbre[slot];

	SBlock[slot] = 0x20;
	SType[slot] = BNK_INST;
	SDuration[slot] = 0xffff;
	TvValue[TV_P][slot] = 32767;
	SFbc[slot] = TimbreByte(t, 8) & 1;
	TvValue[TV_FB][slot] = (uint16_t) (TimbreByte(t, 8) << 12);
	SKsltl0[slot] = TimbreByte(t, 4) & 0xc0;
	TvValue[TV_V0][slot] = (uint16_t) ((~TimbreByte(t, 4) & 0x3f) << 10);
	SKsltl1[slot] = TimbreByte(t, 10) & 0xc0;
	TvValue[TV_V1][slot] = (uint16_t) ((~TimbreByte(t, 10) & 0x3f) << 10);
	SAvekm0[slot] = TimbreByte(t, 3) & 0xf0;
	TvValue[TV_M0][slot] = (uint16_t) (TimbreByte(t, 3) << 12);
	SAvekm1[slot] = TimbreByte(t, 9) & 0xf0;
	TvValue[TV_M1][slot] = (uint16_t) (TimbreByte(t, 9) << 12);
	SAd0[slot] = TimbreByte(t, 5);
	SSr0[slot] = TimbreByte(t, 6);
	SAd1[slot] = TimbreByte(t, 11);
	SSr1[slot] = TimbreByte(t, 12);
	TvValue[TV_WS][slot] = (uint16_t) (TimbreByte(t, 13) | TimbreByte(t, 7) << 8);
	SScale01[slot] = SFbc[slot] | 2;
	SUpdate[slot] = U_ALL_REGS;
}

/* ---- TVFX ---- */

/* A slot that lost its voice gets the first free one, if any. */
static void TvSwitchVoice(void)
{
	for (int16_t slot = 0; slot < NUM_SLOTS; slot++) {
		if (SStatus[slot] == FREE || SVoice[slot] != 0xff)
			continue;
		for (uint8_t voice = 0; voice < NUM_VOICES; voice++) {
			if (VChannel[voice] != 0xff)
				continue;
			SVoice[slot] = voice;
			MidiVoices[SChannel[slot]]++;
			VChannel[voice] = SChannel[slot];
			SUpdate[slot] = U_ALL_REGS;
			UpdatePriority();
			return;
		}
		return;
	}
}

/* Runs a parameter's command list up to its next ramp: 0 jumps by the operand, FFFF sets the
 * value, FFFE sets a register byte, anything else is a step count with the step to add each
 * refresh. Ten commands without a ramp end the list. */
static void TvFetch(int16_t slot, int16_t param)
{
	for (int16_t n = 0; n < 10; n++) {
		uint16_t command = (uint16_t) (STimbre[slot] + TvPos[param][slot]);
		uint16_t op = SegmentWord(command), operand = SegmentWord((uint16_t) (command + 2));

		if (op == 0) {
			TvPos[param][slot] += operand;
			continue;
		}
		TvPos[param][slot] += 4;
		if (op == 0xffff) {
			TvValue[param][slot] = operand;
			continue;
		}
		if (op != 0xfffe) {
			TvCount[param][slot] = op;
			TvStep[param][slot] = operand;
			return;
		}
		switch (param) {
		case TV_M0:
			SAvekm0[slot] = (uint8_t) operand;
			break;
		case TV_M1:
			SAvekm1[slot] = (uint8_t) operand;
			break;
		case TV_V0:
			SKsltl0[slot] = (uint8_t) operand;
			break;
		case TV_V1:
			SKsltl1[slot] = (uint8_t) operand;
			break;
		case TV_F:
			SBlock[slot] = (uint8_t) (operand >> 8);
			if (SType[slot] == TV_INST)
				SBlock[slot] &= 0xe0;
			break;
		case TV_FB:
			SFbc[slot] = (uint8_t) (operand >> 8);
			SScale01[slot] = (uint8_t) ((operand >> 8) | 2);
			break;
		}
	}
	TvStep[param][slot] = 0;
	TvCount[param][slot] = 0xffff;
}

/* Starts a TVFX timbre's key-on or key-off phase. */
static void TvPhase(int16_t slot)
{
	/* where each parameter's start value and key-on and key-off lists are in the timbre */
	static const uint8_t Params[TV_PARAMS] = { TV_F, TV_V0, TV_V1, TV_P, TV_FB, TV_M0, TV_M1, TV_WS };
	static const uint8_t ValueAt[TV_PARAMS] = { 0x06, 0x0c, 0x12, 0x18, 0x1e, 0x24, 0x2a, 0x30 };
	uint16_t t = STimbre[slot];
	uint8_t type = TimbreByte(t, 3);
	uint16_t envelope0 = 0xff0f, envelope1 = 0xff0f;
	uint8_t block = 0x20;

	SFbc[slot] = 0;
	SKsltl0[slot] = 0;
	SKsltl1[slot] = 0;
	SScale01[slot] = 2;
	SAvekm0[slot] = 0x20;
	SAvekm1[slot] = 0x20;
	if (type != 1)
		block |= 8;
	SType[slot] = type;
	SBlock[slot] = block;
	if ((uint16_t) (TimbreWord(t, 8) + 2) != 0x36) {
		envelope0 = TimbreWord(t, 0x36);
		envelope1 = TimbreWord(t, 0x38);
		if (SStatus[slot] == KEYOFF) {
			envelope0 = TimbreWord(t, 0x3a);
			envelope1 = TimbreWord(t, 0x3c);
		}
	}
	SAd0[slot] = (uint8_t) (envelope1 >> 8);
	SSr0[slot] = (uint8_t) envelope1;
	SAd1[slot] = (uint8_t) (envelope0 >> 8);
	SSr1[slot] = (uint8_t) envelope0;
	if (SStatus[slot] == KEYOFF) {
		for (int16_t i = 0; i < TV_PARAMS; i++)
			TvPos[Params[i]][slot] = (uint16_t) (TimbreWord(t, ValueAt[i] + 4) + 2);
	} else {
		SDuration[slot] = type == 1 ? 0xffff : (uint16_t) (TimbreWord(t, 4) + 1);
		for (int16_t i = 0; i < TV_PARAMS; i++) {
			TvValue[Params[i]][slot] = TimbreWord(t, ValueAt[i]);
			TvPos[Params[i]][slot] = (uint16_t) (TimbreWord(t, ValueAt[i] + 2) + 2);
		}
	}
	SUpdate[slot] = U_ALL_REGS;
	for (int16_t i = 0; i < TV_PARAMS; i++) {
		TvCount[i][slot] = 1;
		TvStep[i][slot] = 0;
	}
}

/* Steps one parameter: its value moves by the step, and a finished ramp fetches the next. */
static void TvStepParam(int16_t slot, int16_t param, uint8_t changed)
{
	if (TvStep[param][slot] != 0) {
		TvValue[param][slot] += TvStep[param][slot];
		SUpdate[slot] |= changed;
	}
	if (--TvCount[param][slot] == 0) {
		TvFetch(slot, param);
		SUpdate[slot] |= changed;
	}
}

/* A level steps as the others, but on key-off one that wraps past zero stops at zero, and it
 * goes out only every other refresh. */
static void TvStepLevel(int16_t slot, int16_t param)
{
	uint16_t step = TvStep[param][slot];

	if (step != 0) {
		uint16_t old = TvValue[param][slot];
		uint16_t now = (uint16_t) (old + step);

		TvValue[param][slot] = now;
		if (SStatus[slot] == KEYOFF && ((now ^ old) & 0x8000) && !((now ^ step) & 0x8000))
			TvValue[param][slot] = 0;
		SUpdate[slot] |= VolUpdate;
	}
	if (--TvCount[param][slot] == 0) {
		TvFetch(slot, param);
		SUpdate[slot] |= U_KSLTL;
	}
}

/* The driver's own service, after the sequences: the TVFX refresh at 60 Hz and the voice
 * priority check at 5 Hz. */
static void ServeSynth(void)
{
	TvAccum += 60;
	if (TvAccum >= 120) {
		TvAccum -= 120;
		VolUpdate ^= U_KSLTL;
		for (int16_t slot = 0; slot < NUM_SLOTS; slot++) {
			if (SStatus[slot] == FREE)
				continue;
			if (Old ? SType[slot] == BNK_INST : SType[slot] != TV_INST && SType[slot] != TV_EFFECT)
				continue;
			TvStepParam(slot, TV_F, U_FREQ);
			TvStepParam(slot, TV_FB, U_FBC);
			TvStepParam(slot, TV_M0, U_AVEKM);
			TvStepParam(slot, TV_M1, U_AVEKM);
			TvStepLevel(slot, TV_V0);
			TvStepLevel(slot, TV_V1);
			TvStepParam(slot, TV_WS, U_WS);
			TvValue[TV_P][slot] += TvStep[TV_P][slot];
			if (--TvCount[TV_P][slot] == 0)
				TvFetch(slot, TV_P);
			if (SUpdate[slot] & U_ALL_REGS)
				UpdateVoice(slot);
			if (SStatus[slot] != KEYOFF) {
				if (--SDuration[slot] == 0) {
					SStatus[slot] = KEYOFF;
					TvPhase(slot);
				}
			} else if (TvValue[TV_V0][slot] < 0x400 && TvValue[TV_V1][slot] < 0x400) {
				ReleaseVoice(slot);
				SStatus[slot] = FREE;
				TvSwitchVoice();
			}
		}
	}
	PriAccum += 5;
	if (PriAccum >= 120) {
		PriAccum -= 120;
		UpdatePriority();
	}
}

/* ---- MIDI ---- */

static void NoteOff(uint8_t chan, uint8_t note)
{
	for (int16_t slot = 0; slot < NUM_SLOTS; slot++) {
		if (SStatus[slot] != KEYON || SKeynum[slot] != note || SChannel[slot] != chan)
			continue;
		if ((int8_t) MidiSus[chan] >= 64)
			SSustain[slot] = 1;
		else if (SType[slot] == OPL3_INST || SType[slot] == BNK_INST) {
			ReleaseVoice(slot);
			SStatus[slot] = FREE;
			TvSwitchVoice();
		} else
			SDuration[slot] = 1;
	}
}

static void NoteOn(uint8_t chan, uint8_t note, uint8_t velocity)
{
	uint8_t index = MidiTimbre[chan];
	uint16_t timbre;
	int16_t slot;

	if (chan == 9) {
		index = RbsTimbres[note];
		if (index == 0xff) {
			index = (uint8_t) IndexTimbre((uint16_t) (127 << 8 | note));
			RbsTimbres[note] = index;
		}
	}
	if (index == 0xff)
		return;
	timbre = (uint16_t) (CacheOffset + TimbOffsets[index]);
	TimbHist[index] = ++NoteEvent;
	for (slot = 0; slot < NUM_SLOTS && SStatus[slot] != FREE; slot++)
		;
	if (slot == NUM_SLOTS)
		return;
	SChannel[slot] = chan;
	SKeynum[slot] = note;
	if (chan == 9) {
		SNote[slot] = TimbreByte(timbre, 2);
		STranspose[slot] = 0;
	} else {
		SNote[slot] = note;
		STranspose[slot] = TimbreByte(timbre, 2);
	}
	SVelocity[slot] = VelGraph[(uint8_t) (velocity >> 3)];
	STimbre[slot] = timbre;
	SStatus[slot] = KEYON;
	SSustain[slot] = 0;
	switch (SegmentWord(timbre)) {
	case OPL3BNK_SIZE:
		if (!Old)
			break;
		TvPhase(slot);
		break;
	case BNK_SIZE:
		BnkPhase(slot);
		break;
	default:
		TvPhase(slot);
		break;
	}
	SVoice[slot] = 0xff;
	AssignVoice(slot);
}

/* Ends the sustained notes, as note-offs for their slots' notes on the given channel. */
static void ReleaseSustain(uint8_t chan, uint8_t noteChan)
{
	for (int16_t slot = 0; slot < NUM_SLOTS; slot++) {
		if (SStatus[slot] != FREE && SChannel[slot] == chan && SSustain[slot] != 0)
			NoteOff(noteChan, SNote[slot]);
	}
}

/* Marks the registers a channel's change affects and updates its slots. */
static void FlagUpdates(uint8_t chan, uint8_t flags)
{
	for (int16_t slot = 0; slot < NUM_SLOTS; slot++) {
		if (SStatus[slot] == FREE || SChannel[slot] != chan)
			continue;
		SUpdate[slot] |= flags;
		UpdateVoice(slot);
	}
}

static void SendMidi(uint8_t status, uint8_t d1, uint8_t d2)
{
	uint8_t chan = status & 0x0f;
	uint8_t *shadow;
	uint8_t flags;

	switch (status & 0xf0) {
	case 0x90:
		if (chan < 1 || chan > 9)
			return;
		if (d2 != 0) {
			NoteOn(chan, d1, d2);
			return;
		}
		NoteOff(chan, d1);
		return;
	case 0x80:
		NoteOff(chan, d1);
		return;
	case 0xe0:
		MidiPitchL[chan] = d1;
		MidiPitchH[chan] = d2;
		FlagUpdates(chan, U_FREQ);
		return;
	case 0xc0:
		MidiProgram[chan] = d1;
		MidiTimbre[chan] = (uint8_t) IndexTimbre((uint16_t) (MidiBank[chan] << 8 | d1));
		return;
	case 0xb0:
		break;
	default:
		return;
	}
	switch (d1) {
	case PATCH_BANK_SEL:
		MidiBank[chan] = d2;
		return;
	case VOICE_PROTECT:
		MidiVprot[chan] = d2;
		return;
	case TIMBRE_PROTECT:
		if (MidiTimbre[chan] != 0xff) {
			uint8_t *attribs = &TimbAttribs[MidiTimbre[chan]];

			*attribs &= ~TIMB_PROTECTED;
			if ((int8_t) d2 >= 64)
				*attribs |= TIMB_PROTECTED;
		}
		return;
	case MODULATION:
		shadow = MidiMod;
		flags = U_AVEKM;
		break;
	case PART_VOLUME:
		shadow = MidiVol;
		flags = U_KSLTL;
		break;
	case EXPRESSION:
		shadow = MidiExpress;
		flags = U_KSLTL;
		break;
	case PANPOT:
		shadow = MidiPan;
		flags = U_KSLTL;
		break;
	case SUSTAIN:
		MidiSus[chan] = d2;
		if ((int8_t) d2 < 64)
			ReleaseSustain(chan, chan);
		return;
	case RESET_ALL_CTRLS:
		if (!Old)
			MidiSus[chan] = 0;
		ReleaseSustain(chan, chan);
		MidiMod[chan] = 0;
		MidiExpress[chan] = 127;
		MidiPitchL[chan] = DEF_PITCH_L;
		MidiPitchH[chan] = DEF_PITCH_H;
		FlagUpdates(chan, U_AVEKM | U_KSLTL | U_FREQ);
		return;
	case ALL_NOTES_OFF:
		for (int16_t slot = 0; slot < NUM_SLOTS; slot++) {
			if (SStatus[slot] == KEYON && SChannel[slot] == chan)
				NoteOff(chan, SNote[slot]);
		}
		return;
	default:
		return;
	}
	shadow[chan] = d2;
	FlagUpdates(chan, flags);
}

/* ---- Driver ---- */

drvr_desc *OplDescribe(void)
{
	Description.min_API_version = 200;
	Description.drvr_type = XMIDI_DRVR;
	memcpy(Description.data_suffix, "AD\0", 4);
	Description.dev_name_table = DeviceNames;
	Description.default_IO = 0x388;
	Description.default_IRQ = -1;
	Description.default_DMA = -1;
	Description.default_DRQ = -1;
	Description.service_rate = QUANT_RATE;
	Description.display_size = 0;
	return &Description;
}

static void SetPort(uint16_t port)
{
	AddrStat = port;
	DataOut = (uint16_t) (port + 1);
}

void OplSetPort(uint16_t port, uint8_t old)
{
	Old = old;
	SetPort(port);
}

/* Looks for the chip at the port, leaving the driver's own port as it was. */
uint16_t OplDetect(uint16_t port, uint8_t old)
{
	uint16_t addrStat = AddrStat, dataOut = DataOut, found;

	if (!plat_opl_available())
		return 0;
	plat_sound_lock();
	Old = old;
	SetPort(port);
	found = DetectAdlib();
	AddrStat = addrStat;
	DataOut = dataOut;
	plat_sound_unlock();
	return found;
}

static void Shutdown(const char *signOff)
{
}

static void SysexMessage(uint8_t a, uint8_t b, uint8_t c, const uint8_t *data, uint16_t size,
	uint16_t wait)
{
}

static void WriteDisplay(const char *string)
{
}

const XmidiSynth OplSynth = {
	AIL_KIND_ADLIB, DEF_SYNTH_VOL, 0, SendMidi, 0, ServeSynth, ResetSynth, InitSynth, Shutdown,
	TimbreRequest, TimbreStatus, InstallTimbre, ProtectTimbre, DEF_TC_SIZE, DefineCache,
	SysexMessage, WriteDisplay
};

extern "C" void ResetAiloplGlobals(void)
{
	memset(&Description, 0, sizeof Description);
	Old = 0;
	memset(Shadow, 0, sizeof Shadow);
	AddrStat = 0;
	DataOut = 0;
	NoteEvent = 0;
	memset(TimbHist, 0, sizeof TimbHist);
	memset(TimbOffsets, 0, sizeof TimbOffsets);
	memset(TimbBank, 0, sizeof TimbBank);
	memset(TimbNum, 0, sizeof TimbNum);
	memset(TimbAttribs, 0, sizeof TimbAttribs);
	CacheSegment = 0;
	CacheOffset = 0;
	CacheSize = 0;
	CacheEnd = 0;
	TvAccum = 0;
	PriAccum = 0;
	VolUpdate = 0;
	Rover = 0;
	memset(STimbre, 0, sizeof STimbre);
	memset(SDuration, 0, sizeof SDuration);
	memset(SStatus, 0, sizeof SStatus);
	memset(SType, 0, sizeof SType);
	memset(SVoice, 0, sizeof SVoice);
	memset(SChannel, 0, sizeof SChannel);
	memset(SNote, 0, sizeof SNote);
	memset(SKeynum, 0, sizeof SKeynum);
	memset(STranspose, 0, sizeof STranspose);
	memset(SVelocity, 0, sizeof SVelocity);
	memset(SSustain, 0, sizeof SSustain);
	memset(SUpdate, 0, sizeof SUpdate);
	memset(SKbfShadow, 0, sizeof SKbfShadow);
	memset(SBlock, 0, sizeof SBlock);
	memset(SFbc, 0, sizeof SFbc);
	memset(SKsltl0, 0, sizeof SKsltl0);
	memset(SKsltl1, 0, sizeof SKsltl1);
	memset(SAvekm0, 0, sizeof SAvekm0);
	memset(SAvekm1, 0, sizeof SAvekm1);
	memset(SAd0, 0, sizeof SAd0);
	memset(SAd1, 0, sizeof SAd1);
	memset(SSr0, 0, sizeof SSr0);
	memset(SSr1, 0, sizeof SSr1);
	memset(SScale01, 0, sizeof SScale01);
	memset(TvPos, 0, sizeof TvPos);
	memset(TvCount, 0, sizeof TvCount);
	memset(TvValue, 0, sizeof TvValue);
	memset(TvStep, 0, sizeof TvStep);
	memset(MidiVol, 0, sizeof MidiVol);
	memset(MidiPan, 0, sizeof MidiPan);
	memset(MidiPitchL, 0, sizeof MidiPitchL);
	memset(MidiPitchH, 0, sizeof MidiPitchH);
	memset(MidiExpress, 0, sizeof MidiExpress);
	memset(MidiMod, 0, sizeof MidiMod);
	memset(MidiSus, 0, sizeof MidiSus);
	memset(MidiVprot, 0, sizeof MidiVprot);
	memset(MidiTimbre, 0, sizeof MidiTimbre);
	memset(MidiBank, 0, sizeof MidiBank);
	memset(MidiProgram, 0, sizeof MidiProgram);
	memset(RbsTimbres, 0, sizeof RbsTimbres);
	memset(MidiVoices, 0, sizeof MidiVoices);
	memset(VChannel, 0, sizeof VChannel);
	memset(SVPriority, 0, sizeof SVPriority);
}
