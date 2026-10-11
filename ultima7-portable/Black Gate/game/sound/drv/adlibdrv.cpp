/* The AdLib (OPL2) music driver, native.
 *
 * Voices 0-8 are the chip's channels. In rhythm mode voices 6-10 are the rhythm sounds (bass
 * drum, snare, tom, cymbal, hi-hat) and only 0-5 are melodic. Melodic voices sit on two circular
 * lists through VoiceNext: free voices after node 11 and busy ones after node 12, oldest first.
 * Pitches are in 256ths of a semitone, 60 is middle C.
 *
 * The driver's variables live in Image at the offsets the original had, so an index past an
 * array lands where it did on DOS (stealing from an empty busy list uses voice 12, for one).
 * Image starts as the original's first 0x340 bytes; past its data that is its code, which only
 * such indexes read. A timbre is named by the segment DOS would give it: BANK_SEGMENT plus three
 * paragraphs per timbre.
 */

#include "u7port.h"
#include "plat.h"
#include "midiplay.h"
#include "adlibdrv.h"

#define TIMBRE_SIZE     48
#define NO_TIMBRE       0xffff
#define NO_CHANNEL      0xff
#define FREE_LIST       11
#define BUSY_LIST       12
#define BANK_SEGMENT    0x3000  /* nominal: DOS's depended on where the bank was loaded */
#define BANK_OFFSET     5       /* the first timbre's offset there, also nominal */

/* A timbre's bytes */
#define T_MOD           0x00    /* modulator 20h, 40h, 60h, 80h, E0h values */
#define T_CAR           0x05    /* carrier, the same five */
#define T_FEEDBACK      0x0a    /* C0h value */
#define T_RHYTHM        0x0b    /* rhythm voice 6-10, or 0 for melodic */
#define T_CAR_TOUCH     0x0c    /* carrier level's velocity scaling, 0 none */
#define T_MOD_TOUCH     0x0d
#define T_BEND_RANGE    0x0e
#define T_WHEEL         0x0f    /* vibrato depth added at full mod wheel */
#define T_VIB_RATE      0x10
#define T_VIB_DEPTH     0x11
#define T_SLIDE         0x12    /* pitch slide start, then rate and target words per stage */
#define T_TUNE          0x24
#define T_LAYER         0x26    /* nonzero: the next timbre plays too */
#define T_KEY_SHIFT     0x27    /* note distance from middle C is shifted right by this */
#define T_PROGRAM       0x2f

/* Offsets in Image */
#define TIMBRE_COUNT    0x32
#define OPL_PORT        0x33
#define VOICE_COUNT     0x35    /* melodic voices: 9, or 6 in rhythm mode */
#define RHYTHM_BITS     0x36    /* register BDh */
#define VOICE_NOTE      0x37
#define VOICE_PITCH     0x42    /* words: the note as a pitch, before bend, slide and vibrato */
#define VOICE_TIMBRE    0x58    /* words */
#define VOICE_STATE     0x6e    /* 0 released, 1 idle, 2 up held (slide stages) */
#define VOICE_NEXT      0x79
#define VOICE_CHANNEL   0x86
#define CHANNEL_TIMBRE  0x91    /* words */
#define VOICE_SLIDE     0xd1    /* words */
#define CHANNEL_BEND    0xe7    /* words */
#define VOICE_VIBRATO   0x127   /* words */
#define VOICE_PHASE     0x13d
#define CHANNEL_VIB_RATE 0x148
#define CHANNEL_VIB_DEPTH 0x168 /* words */
#define CHANNEL_VOLUME  0x1a8   /* words: volume + 80h */
#define RHYTHM_BIT      0x1e8   /* register BDh bit of rhythm voices 6-10 */
#define CARRIER_SLOT    0x1ed   /* by voice; rhythm mode's table starts 9 on */
#define MODULATOR_SLOT  0x201
#define FNUMBER         0x215   /* 13 words, a semitone apart */
#define SLIDE_STAGE     0x22f   /* by voice state: half the stage's offset in a timbre, plus 8 */

static const uint8_t ImageStart[0x340] = {
	0x00, 0x00, 0x87, 0x02, 0x8d, 0x02, 0x70, 0x03, 0x07, 0x0e, 0x44, 0x07, 0x84, 0x0b, 0x9b, 0x0a,
	0xd4, 0x0c, 0xdc, 0x02, 0xea, 0x02, 0xf1, 0x02, 0x43, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x88, 0x03, 0x09, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x0b, 0xff, 0xff, 0x00, 0x0c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00, 0x06, 0x00, 0x07,
	0x00, 0x08, 0x00, 0x09, 0x00, 0x0a, 0x00, 0x0b, 0x00, 0x0c, 0x00, 0x0d, 0x00, 0x0e, 0x00, 0x05,
	0x00, 0x10, 0x00, 0x11, 0x00, 0x12, 0x00, 0x13, 0x00, 0x14, 0x00, 0x15, 0x00, 0x16, 0x00, 0x17,
	0x00, 0x18, 0x00, 0x19, 0x00, 0x1a, 0x00, 0x1b, 0x00, 0x1c, 0x00, 0x1d, 0x00, 0x1e, 0x00, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
	0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
	0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
	0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
	0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x10, 0x08, 0x04, 0x02, 0x01, 0x03, 0x04, 0x05,
	0x0b, 0x0c, 0x0d, 0x13, 0x14, 0x15, 0x03, 0x04, 0x05, 0x0b, 0x0c, 0x0d, 0x13, 0x14, 0x12, 0x15,
	0x11, 0x00, 0x01, 0x02, 0x08, 0x09, 0x0a, 0x10, 0x11, 0x12, 0x00, 0x01, 0x02, 0x08, 0x09, 0x0a,
	0x10, 0x14, 0x12, 0x15, 0x11, 0xe5, 0x01, 0x02, 0x02, 0x20, 0x02, 0x41, 0x02, 0x63, 0x02, 0x87,
	0x02, 0xae, 0x02, 0xd7, 0x02, 0x02, 0x03, 0x30, 0x03, 0x60, 0x03, 0x93, 0x03, 0xca, 0x03, 0x18,
	0x00, 0x12, 0x14, 0x16, 0x00, 0x00, 0x00, 0x56, 0x53, 0x33, 0xf6, 0xb0, 0x06, 0xe6, 0x43, 0xb0,
	0x36, 0xe4, 0x40, 0x8a, 0xd8, 0xe4, 0x40, 0x8a, 0xf8, 0x8b, 0xd3, 0xb0, 0x06, 0xe6, 0x43, 0xb0,
	0x36, 0xe4, 0x40, 0x8a, 0xd8, 0xe4, 0x40, 0x8a, 0xf8, 0x2b, 0xd3, 0x73, 0x04, 0x81, 0xc2, 0xae,
	0x4d, 0x03, 0xf2, 0x3b, 0xf1, 0x72, 0xe2, 0x5b, 0x5e, 0xc3, 0x9c, 0xfa, 0x8b, 0x16, 0x33, 0x00,
	0xee, 0xb9, 0x0a, 0x00, 0xe8, 0xc0, 0xff, 0x8b, 0x16, 0x33, 0x00, 0x42, 0x8a, 0xc4, 0xee, 0xb9,
	0x37, 0x00, 0xe8, 0xb2, 0xff, 0x9d, 0xc3, 0xb8, 0x1a, 0x00, 0x8c, 0xca, 0xcb, 0x55, 0x8b, 0xec,
	0x56, 0x57, 0x1e, 0x8c, 0xc8, 0x8e, 0xd8, 0x8e, 0xc0, 0x2e, 0xc6, 0x06, 0x35, 0x00, 0x09, 0xb8,
	0x01, 0x20, 0xe8, 0xc5, 0xff, 0xb8, 0xbd, 0x00, 0xe8, 0xbf, 0xff, 0xb8, 0x08, 0x00, 0xe8, 0xb9,
	0xff, 0x8b, 0x5e, 0x06, 0x8e, 0x5e, 0x08, 0x8a, 0x07, 0x2e, 0xa2, 0x32, 0x00, 0x43, 0x2e, 0x8c,
	0x1e, 0x30, 0x00, 0x2e, 0x89, 0x1e, 0x2e, 0x00, 0xb9, 0x20, 0x00, 0xbf, 0x91, 0x00, 0xf3, 0xab,
	0xb8, 0x1a, 0x00, 0x8c, 0xca, 0x1f, 0x5f, 0x5e, 0x5d, 0xca, 0x04, 0x00, 0x55, 0x8b, 0xec, 0x8b,
	0x46, 0x06, 0x2e, 0xa3, 0x33, 0x00, 0x5d, 0xca, 0x02, 0x00, 0x55, 0x8b, 0xec, 0x5d, 0xca, 0x0a,
	0x00, 0x55, 0x8b, 0xec, 0x1e, 0x8c, 0xc8, 0x8e, 0xd8, 0x8b, 0x76, 0x06, 0x83, 0xfe, 0x00, 0x7c,
	0x3d, 0x83, 0xfe, 0x08, 0x7f, 0x38, 0x8a, 0x84, 0x79, 0x00, 0x32, 0xe4, 0x3b, 0xc6, 0x74, 0x2e,
	0x9c, 0xfa, 0xc6, 0x84, 0x86, 0x00, 0xff, 0xc6, 0x84, 0x6e, 0x00, 0x01, 0xd1, 0xe6, 0xc7, 0x84,
	0x58, 0x00, 0xff, 0xff, 0xd1, 0xee, 0x8b, 0xc6, 0x04, 0xa0, 0x32, 0xe4, 0xe8, 0x3b, 0xff, 0x8b,
	0xc6, 0x04, 0xb0, 0x32, 0xe4, 0xe8, 0x32, 0xff, 0x56, 0x0e, 0xe8, 0xb0, 0x01, 0x9d, 0x1f, 0x5d,
};

static uint8_t Image[sizeof ImageStart];
static SoundDriver AdlibInfo = { 1, 30 };
static uint8_t *Bank;

#define VoiceCount          Image[VOICE_COUNT]
#define RhythmBits          Image[RHYTHM_BITS]
#define VoiceNote(v)        Image[VOICE_NOTE + (v)]
#define VoiceState(v)       Image[VOICE_STATE + (v)]
#define VoiceNext(v)        Image[VOICE_NEXT + (v)]
#define VoiceChannel(v)     Image[VOICE_CHANNEL + (v)]
#define VoicePhase(v)       Image[VOICE_PHASE + (v)]
#define ChannelVibRate(c)   Image[CHANNEL_VIB_RATE + (c)]

struct OplLock {
	OplLock() { plat_sound_lock(); }
	~OplLock() { plat_sound_unlock(); }
};

static int16_t Get(int16_t at, int16_t index)
{
	at += index * 2;
	return (int16_t)(Image[at] | Image[at + 1] << 8);
}

static void Put(int16_t at, int16_t index, int16_t value)
{
	at += index * 2;
	Image[at] = value & 0xff;
	Image[at + 1] = (value >> 8) & 0xff;
}

/* A byte of the timbre at that segment, its offset wrapping at 64K as on DOS. Just before the
 * first timbre is the bank's count; past the bank DOS had undefined memory, zeros here. */
static uint8_t TByte(uint16_t t, int16_t at)
{
	int32_t linear = ((int32_t)t - BANK_SEGMENT) * 16 + (uint16_t)(BANK_OFFSET + at) - BANK_OFFSET;

	if (linear < -1 || linear >= (int32_t)Image[TIMBRE_COUNT] * TIMBRE_SIZE)
		return 0;
	return Bank[linear];
}

static int16_t TWord(uint16_t t, int16_t at)
{
	return (int16_t)(TByte(t, at) | TByte(t, at + 1) << 8);
}

/* An arithmetic shift as the 386 did it, the count taken mod 32. */
static int16_t Sar(int16_t value, uint8_t count)
{
	count &= 31;
	if (count > 15)
		return value < 0 ? -1 : 0;
	return value >> count;
}

static void OplWrite(uint8_t reg, uint8_t value)
{
	uint16_t port = Get(OPL_PORT, 0);

	plat_opl_write(port, reg);
	plat_opl_write(port + 1, value);
}

SoundDriver *AdlibDescribe(void)
{
	return &AdlibInfo;
}

/* Keeps the timbre bank, a count byte then the timbres. Every channel's timbre starts as the
 * count, which names no timbre. */
int16_t AdlibInit(void *timbres)
{
	OplLock hold;
	uint8_t *p = (uint8_t *)timbres;
	int16_t i;

	VoiceCount = 9;
	OplWrite(0x01, 0x20);
	OplWrite(0xbd, 0);
	OplWrite(0x08, 0);
	Image[TIMBRE_COUNT] = *p;
	Bank = p + 1;
	for (i = 0; i < 32; i++)
		Put(CHANNEL_TIMBRE, i, *p);
	return 1;
}

void AdlibPort(int16_t port)
{
	Put(OPL_PORT, 0, port);
}

void AdlibSysex(int32_t, int16_t, void *)
{
}

static void Unlink(int16_t voice)
{
	int16_t i;

	for (i = 0; i < 13; i++)
		if (VoiceNext(i) == voice) {
			VoiceNext(i) = VoiceNext(voice);
			VoiceNext(voice) = voice;
		}
}

/* The list heads are nodes too, so in front of a head is the end of its list. */
static void LinkBefore(int16_t voice, int16_t node)
{
	int16_t i;

	for (i = 0; i < 13; i++)
		if (VoiceNext(i) == node) {
			VoiceNext(i) = voice;
			VoiceNext(voice) = node;
			break;
		}
}

/* Lends melodic voice 0-8 to the sound effects. */
void AdlibReleaseVoice(int16_t voice)
{
	OplLock hold;

	if (voice < 0 || voice > 8 || VoiceNext(voice) == voice)
		return;
	VoiceChannel(voice) = NO_CHANNEL;
	VoiceState(voice) = 1;
	Put(VOICE_TIMBRE, voice, NO_TIMBRE);
	OplWrite(0xa0 + voice, 0);
	OplWrite(0xb0 + voice, 0);
	Unlink(voice);
}

void AdlibClaimVoice(int16_t voice)
{
	OplLock hold;

	if (voice < 0 || voice > 8 || VoiceNext(voice) != voice)
		return;
	LinkBefore(voice, FREE_LIST);
}

void AdlibShutdown(void)
{
	OplLock hold;
	int16_t i;

	VoiceCount = 0;
	OplWrite(0xbd, 0);
	for (i = 0; i < 9; i++) {
		OplWrite(0xa0 + i, 0);
		OplWrite(0xb0 + i, 0);
	}
}

static uint8_t CarrierOf(int16_t voice)
{
	if (VoiceCount < 9)
		voice += 9;
	return Image[CARRIER_SLOT + voice];
}

static uint8_t ModulatorOf(int16_t voice)
{
	if (VoiceCount < 9)
		voice += 9;
	return Image[MODULATOR_SLOT + voice];
}

/* The rhythm sounds that use one operator (rhythm voice 7 up) leave the carrier alone. */
static void LoadTimbre(int16_t voice, uint16_t t)
{
	uint8_t carrier = CarrierOf(voice);
	uint8_t modulator = ModulatorOf(voice);

	OplWrite(modulator + 0x20, TByte(t, T_MOD));
	OplWrite(modulator + 0x40, TByte(t, T_MOD + 1));
	OplWrite(modulator + 0x60, TByte(t, T_MOD + 2));
	OplWrite(modulator + 0x80, TByte(t, T_MOD + 3));
	OplWrite(modulator + 0xe0, TByte(t, T_MOD + 4));
	if (VoiceCount == 9 || TByte(t, T_RHYTHM) < 7) {
		OplWrite(carrier + 0x20, TByte(t, T_CAR));
		OplWrite(carrier + 0x40, TByte(t, T_CAR + 1));
		OplWrite(carrier + 0x60, TByte(t, T_CAR + 2));
		OplWrite(carrier + 0x80, TByte(t, T_CAR + 3));
		OplWrite(carrier + 0xe0, TByte(t, T_CAR + 4));
		OplWrite(voice + 0xc0, TByte(t, T_FEEDBACK));
	}
}

/* The voice for a note, or -1. In rhythm mode a rhythm sound has its own voice, keyed off first.
 * A note on takes the oldest free voice, or steals the oldest busy one when the timbre is the
 * channel's own; a note off finds the voice playing it and frees it. */
static int16_t AllocateVoice(uint16_t t, int16_t velocity, int16_t note, int16_t channel)
{
	int16_t v = -1;

	if (VoiceCount < 9 && TByte(t, T_RHYTHM) != 0) {
		v = TByte(t, T_RHYTHM);
		RhythmBits &= ~Image[RHYTHM_BIT + v - 6];
		OplWrite(0xbd, RhythmBits);
	} else if (velocity != 0) {
		if (VoiceNext(FREE_LIST) != FREE_LIST) {
			v = VoiceNext(FREE_LIST);
			VoiceNext(FREE_LIST) = VoiceNext(v);
			LinkBefore(v, BUSY_LIST);
		} else if ((uint16_t)Get(CHANNEL_TIMBRE, channel) == t) {
			v = VoiceNext(BUSY_LIST);
			VoiceNext(BUSY_LIST) = VoiceNext(v);
			LinkBefore(v, BUSY_LIST);
			OplWrite(0xa0 + v, 0);
			OplWrite(0xb0 + v, 0);
		}
		if (v >= 0) {
			VoiceState(v) = 2;
			VoiceNote(v) = note;
		}
	} else {
		for (v = 0; v < VoiceCount; v++)
			if (VoiceState(v) > 1 && VoiceNote(v) == note && VoiceChannel(v) == channel
				&& (uint16_t)Get(VOICE_TIMBRE, v) == t) {
				VoiceState(v) = 0;
				Unlink(v);
				LinkBefore(v, FREE_LIST);
				break;
			}
		if (v >= VoiceCount)
			v = -1;
	}
	if (v >= 0 && (VoiceChannel(v) != channel || (uint16_t)Get(VOICE_TIMBRE, v) != t)) {
		LoadTimbre(v, t);
		VoiceChannel(v) = channel;
		Put(VOICE_TIMBRE, v, t);
	}
	return v;
}

/* Block in bits 10-12, F-number below, interpolated between table entries. The upper entry is
 * found from (semitone - 18) mod 12, which is negative below semitone 18. */
static uint16_t PitchToFrequency(int16_t pitch)
{
	int16_t semitone = pitch >> 8;
	int16_t block, f, step;

	block = (semitone + 6) / 12 - 2;
	if (block > 7)
		block = 7;
	if (block < 0)
		block = 0;
	f = Get(FNUMBER, (semitone + 6) % 12);
	if (pitch & 0xff) {
		step = Get(FNUMBER + 2, (semitone - 18) % 12) - f;
		f += (int16_t)(step * (pitch & 0xff)) >> 8;
	}
	return (block << 10) + f;
}

/* The timbre with that program, else the first. */
static uint16_t FindTimbre(uint8_t program)
{
	int16_t i;

	for (i = 0; i < Image[TIMBRE_COUNT]; i++)
		if (Bank[i * TIMBRE_SIZE + T_PROGRAM] == program)
			return BANK_SEGMENT + i * 3;
	return BANK_SEGMENT;
}

/* Channel from 1; velocity 0 is note off. Plays every timbre of the channel's chain. */
void AdlibNote(int16_t channel, int16_t note, int16_t velocity)
{
	OplLock hold;
	uint16_t t, freq;
	uint8_t level;
	int16_t v, reg, tune, pitch, x, scaled;

	channel--;
	for (t = Get(CHANNEL_TIMBRE, channel); t != NO_TIMBRE; t = TByte(t, T_LAYER) == 0 ? NO_TIMBRE : t + 3) {
		v = AllocateVoice(t, velocity, note, channel);
		reg = v > 8 ? 17 - v : v;
		if (v < 0)
			continue;
		tune = TWord(t, T_TUNE);
		if (velocity != 0) {
			Put(VOICE_VIBRATO, v, 0);
			VoicePhase(v) = 0;
			Put(VOICE_SLIDE, v, TWord(t, T_SLIDE));
		}
		pitch = (uint16_t)(uint8_t)(note - 60) << 8;
		if ((int8_t)TByte(t, T_KEY_SHIFT) < 0)
			pitch = Sar(-pitch, ~TByte(t, T_KEY_SHIFT));
		else
			pitch = Sar(pitch, TByte(t, T_KEY_SHIFT));
		pitch += 60 << 8;
		Put(VOICE_PITCH, v, pitch);
		pitch += Get(CHANNEL_BEND, channel) + Get(VOICE_SLIDE, v) + Get(VOICE_VIBRATO, v) + tune;
		freq = PitchToFrequency(pitch);
		if (velocity == 0) {
			if (v > VoiceCount && v > 6)
				RhythmBits &= ~Image[RHYTHM_BIT + v - 6];
			else {
				OplWrite(0xa0 + reg, freq & 0xff);
				OplWrite(0xb0 + reg, freq >> 8);
			}
		} else {
			level = TByte(t, T_CAR + 1);
			if (TByte(t, T_CAR_TOUCH) != 0 || Get(CHANNEL_VOLUME, channel) < 0x100) {
				x = Sar(0x3f - velocity, 7 - TByte(t, T_CAR_TOUCH)) + (level & 0x3f);
				x = 0x3f - Sar((int16_t)(Get(CHANNEL_VOLUME, channel) * (0x3f - x)), 8);
				if (x > 0x3f)
					x = 0x3f;
				if (x < 0)
					x = 0;
				OplWrite(CarrierOf(v) + 0x40, (level & 0xc0) + x);
			}
			level = TByte(t, T_MOD + 1);
			if (TByte(t, T_MOD_TOUCH) != 0 || Get(CHANNEL_VOLUME, channel) < 0x100) {
				x = Sar(0x3f - velocity, 7 - TByte(t, T_MOD_TOUCH)) + (level & 0x3f);
				scaled = x;
				if (TByte(t, T_FEEDBACK) & 1) {
					scaled = Sar((int16_t)(Get(CHANNEL_VOLUME, channel) * (0x3f - x)), 8);
					x = 0x3f - scaled;
				}
				/* the original tests the scaled level here, not the result */
				if (scaled > 0x3f)
					x = 0x3f;
				if (x < 0)
					x = 0;
				OplWrite(ModulatorOf(v) + 0x40, (level & 0xc0) + x);
			}
			if (TByte(t, T_RHYTHM) == 0 || v == 6) {
				if (TByte(t, T_RHYTHM) == 0)
					freq += 0x2000;
				OplWrite(0xa0 + reg, freq & 0xff);
				OplWrite(0xb0 + reg, freq >> 8);
			}
			if (TByte(t, T_RHYTHM) != 0)
				RhythmBits |= Image[RHYTHM_BIT + v - 6];
		}
		if (TByte(t, T_RHYTHM) != 0)
			OplWrite(0xbd, RhythmBits);
	}
}

/* The 14-bit bend, scaled by the timbre's bend range; retunes the channel's sounding voices. */
void AdlibPitch(int16_t channel, int16_t low, int16_t high)
{
	OplLock hold;
	int16_t v, tune, pitch;
	uint16_t freq, t;
	int32_t bend;

	channel--;
	t = Get(CHANNEL_TIMBRE, channel);
	bend = (int32_t)(int16_t)((high << 7) + low - 0x2000) * TByte(t, T_BEND_RANGE);
	Put(CHANNEL_BEND, channel, (int16_t)(bend >> 8));
	for (v = 0; v < VoiceCount; v++) {
		if (VoiceState(v) <= 1 || VoiceChannel(v) != channel)
			continue;
		t = Get(VOICE_TIMBRE, v);
		tune = t != NO_TIMBRE ? TWord(t, T_TUNE) : 0;
		pitch = Get(VOICE_PITCH, v) + Get(CHANNEL_BEND, channel) + Get(VOICE_SLIDE, v)
			+ Get(VOICE_VIBRATO, v) + tune;
		freq = PitchToFrequency(pitch) + 0x2000;
		OplWrite(0xa0 + v, freq & 0xff);
		OplWrite(0xb0 + v, freq >> 8);
	}
}

/* 1 vibrato depth, 7 volume, 79h reset, 7Bh all notes off, which also leaves rhythm mode once no
 * rhythm sound plays. Channel 10 passes it on to channels 11-30 first. */
void AdlibControl(int16_t channel, int16_t control, int16_t value)
{
	OplLock hold;
	int16_t ch = channel, v, busy;
	uint16_t t;

	if (ch == 10) {
		for (ch = 11; ch <= 30; ch++)
			AdlibControl(ch, control, value);
		ch = 10;
	}
	ch--;
	switch (control) {
	case 1:
		t = Get(CHANNEL_TIMBRE, ch);
		Put(CHANNEL_VIB_DEPTH, ch, Sar((int16_t)(TByte(t, T_WHEEL) * value), 7) + TByte(t, T_VIB_DEPTH));
		break;
	case 7:
		Put(CHANNEL_VOLUME, ch, value + 0x80);
		break;
	case 0x79:
		AdlibControl(channel, 1, 0);
		AdlibControl(channel, 7, 0x7f);
		AdlibPitch(channel, 0, 0x40);
		break;
	case 0x7b:
		busy = 0;
		for (v = 0; v < 11; v++) {
			if (VoiceState(v) <= 1)
				continue;
			if (VoiceChannel(v) == ch)
				AdlibNote(ch + 1, VoiceNote(v), 0);
			else if (v >= VoiceCount)
				busy = 1;
		}
		if (busy == 0 && VoiceCount < 9) {
			OplWrite(0xbd, 0);
			VoiceCount = 9;
			VoiceNext(6) = 7;
			VoiceNext(7) = 8;
			VoiceNext(8) = VoiceNext(FREE_LIST);
			VoiceNext(FREE_LIST) = 6;
		}
		break;
	}
}

/* Stops the channel's voices and gives it the timbre. The first rhythm timbre turns rhythm mode
 * on, which takes voices 6-8 off the lists. */
void AdlibProgram(int16_t channel, int16_t program)
{
	OplLock hold;
	uint16_t t = FindTimbre(program);
	int16_t v, i;

	channel--;
	for (v = 0; v < 11; v++)
		if (VoiceChannel(v) == channel) {
			AdlibNote(channel + 1, VoiceNote(v), 0);
			VoiceChannel(v) = NO_CHANNEL;
			Put(VOICE_TIMBRE, v, NO_TIMBRE);
		}
	Put(CHANNEL_TIMBRE, channel, t);
	ChannelVibRate(channel) = TByte(t, T_VIB_RATE);
	Put(CHANNEL_VIB_DEPTH, channel, TByte(t, T_VIB_DEPTH));
	if (TByte(t, T_RHYTHM) != 0 && VoiceCount == 9) {
		OplWrite(0xa6, 0x00);
		OplWrite(0xb6, 0x00);
		OplWrite(0xa7, 0x00);
		OplWrite(0xb7, 0x0a);
		OplWrite(0xa8, 0x54);
		OplWrite(0xb8, 0x09);
		VoiceCount = 6;
		for (v = 6; v < 9; v++) {
			for (i = 0; i < 13; i++)
				if (VoiceNext(i) == v) {
					VoiceNext(i) = VoiceNext(v);
					break;
				}
			VoiceNext(v) = 0xff;
		}
		RhythmBits = 0x20;
		OplWrite(0xbd, RhythmBits);
	}
}

static int16_t Distance(int16_t a, int16_t b)
{
	return a > b ? a - b : b - a;
}

/* Each tick steps every voice's pitch slide and vibrato, and rewrites its frequency when either
 * moved or the timbre is detuned. */
void AdlibTimer(void)
{
	OplLock hold;
	int16_t v, stage, tune, rate, target, slide, pitch, changed;
	uint8_t ch;
	uint16_t t, freq;
	int8_t phase;

	for (v = 0; v < VoiceCount; v++) {
		changed = 0;
		ch = VoiceChannel(v);
		stage = Image[SLIDE_STAGE + VoiceState(v)];
		t = Get(VOICE_TIMBRE, v);
		if (t != NO_TIMBRE)
			tune = TWord(t, T_TUNE);
		else {
			t = BANK_SEGMENT;
			tune = 0;
		}
		if (stage != 0) {
			rate = TWord(t, stage * 2 - 0x10);
			target = TWord(t, stage * 2 - 0x0e);
			slide = Get(VOICE_SLIDE, v);
			if ((uint16_t)Distance(target, slide) < (uint16_t)rate) {
				Put(VOICE_SLIDE, v, target);
				VoiceState(v)++;
			} else if (slide < target)
				Put(VOICE_SLIDE, v, slide + rate);
			else
				Put(VOICE_SLIDE, v, slide - rate);
			changed = 1;
		}
		if (ch < 32 && ChannelVibRate(ch) != 0) {
			VoicePhase(v) += ChannelVibRate(ch);
			phase = (int8_t)VoicePhase(v);
			if (phase > 0x3f || phase < -0x40)
				phase = (int8_t)(0x80 - phase);
			Put(VOICE_VIBRATO, v, Sar((int16_t)(Get(CHANNEL_VIB_DEPTH, ch) * phase), 4));
			changed = 1;
		}
		if (changed || tune != 0) {
			pitch = Get(VOICE_PITCH, v) + Get(CHANNEL_BEND, ch) + Get(VOICE_SLIDE, v)
				+ Get(VOICE_VIBRATO, v) + tune;
			freq = PitchToFrequency(pitch);
			if (VoiceState(v) > 1)
				freq += 0x2000;
			OplWrite(0xa0 + v, freq & 0xff);
			OplWrite(0xb0 + v, freq >> 8);
		}
	}
}

extern "C" void ResetAdlibdrvGlobals(void)
{
	memcpy(Image, ImageStart, sizeof Image);
	Bank = 0;
}
