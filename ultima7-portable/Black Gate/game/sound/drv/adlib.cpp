/* The AdLib sound effects driver, native.
 *
 * Up to 16 sounds play at once on the chip's 9 channels; each tick the channels go to the
 * sounds of highest priority. A sound opens with a header; every parameter is an envelope run by
 * a script of (ticks, step) word pairs that follows it. Channels 2-8 are lent to the music driver
 * while music plays. A voice's handle is its number plus one.
 */

#include "u7port.h"
#include "plat.h"
#include "adlib.h"

#define NVOICES         16
#define NCHANNELS       9       /* as a voice's channel, none */
#define HOLDING         2
#define RELEASING       1
#define ADLIB_PORT      0x388

/* OPL2 registers */
#define OPL_TEST        0x01
#define OPL_CSM         0x08
#define OPL_RHYTHM      0xbd
#define OPL_MULT        0x20    /* add the operator */
#define OPL_LEVEL       0x40
#define OPL_AD          0x60
#define OPL_SR          0x80
#define OPL_WAVE        0xe0
#define OPL_FNUM        0xa0    /* add the channel */
#define OPL_KEYON       0xb0
#define OPL_FEEDBACK    0xc0

/* A sound's header: words, each parameter as its start value, attack and release scripts */
#define SND_SIZE        0
#define SND_HOLD        2
#define SND_PITCH       4
#define SND_LEVEL1      10
#define SND_LEVEL2      16
#define SND_PRIORITY    22
#define SND_FEEDBACK    28
#define SND_MULT1       34
#define SND_MULT2       40
#define SND_WAVE        46
#define SND_ADSR        52      /* both operators' AD and SR bytes, held */
#define SND_RELEASE_ADSR 56     /* and released */
#define PART_START      0
#define PART_ATTACK     2
#define PART_RELEASE    4

extern void ( *DriverReleaseChannelEntry)(int16_t);
extern void ( *DriverClaimChannelEntry)(int16_t);

/* A value driven by a script of (ticks, step) word pairs. */
struct Envelope {
	uint16_t script;        /* the next pair */
	uint16_t count;         /* ticks left in this one */
	uint16_t value;
	uint16_t step;          /* added to the value each tick */
};

enum { PITCH, LEVEL1, LEVEL2, FEEDBACK, PRIORITY, MULT1, MULT2, WAVE, ENVELOPES };

/* Each envelope's part of the sound header. */
static const uint8_t SoundPart[ENVELOPES] = {
	SND_PITCH, SND_LEVEL1, SND_LEVEL2, SND_FEEDBACK, SND_PRIORITY, SND_MULT1, SND_MULT2, SND_WAVE,
};

struct AdlibVoice {
	uint16_t state;         /* 0 when free */
	uint16_t hold;          /* ticks left before release */
	uint8_t *sound;
	Envelope env[ENVELOPES];
	uint16_t keyon;         /* key-on and block bits, in the high byte */
	uint16_t ksl;           /* key scaling bits, op1 high, op2 low */
	uint16_t multFlags;     /* register 20h flag bits, op1 high, op2 low */
	uint16_t connect;       /* connection bit for register C0h, in the high byte */
	uint16_t channel;       /* NCHANNELS while it has none */
	uint16_t volume;        /* its row of VolumeTable */
};

static AdlibVoice Voices[NVOICES];
static uint16_t Ended;          /* the voice's level envelopes that have faded out */
static uint8_t OplRegs[256];    /* the last value written to each register */
static uint8_t Channel;         /* the channel being programmed */
static uint8_t Op1, Op2 = 3;    /* and its two operators */
static const uint8_t OpOffsets[NCHANNELS] = { 0, 1, 2, 6, 7, 8, 0x0c, 0x0d, 0x0e };
static uint8_t ChannelBusy[NCHANNELS];
static int16_t Priority[NVOICES];       /* each voice's claim to a channel, worked out every tick */
static int16_t Playing;                 /* voices playing, then channels left to hand out */
static uint16_t Suspended;              /* set while channels 2 to 8 are lent out */

/* Sixteen rows of 64 output levels, one per volume step; a level indexes a row. */
static const uint8_t VolumeTable[1024] = {
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e,
	0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d,
	0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e,
	0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c,
	0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3a, 0x3a, 0x3a, 0x3a, 0x3a, 0x3a, 0x3a, 0x3a,
	0x39, 0x39, 0x39, 0x39, 0x39, 0x39, 0x39, 0x39, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d,
	0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3a, 0x3a, 0x3a, 0x3a, 0x3a,
	0x39, 0x39, 0x39, 0x39, 0x39, 0x39, 0x38, 0x38, 0x38, 0x38, 0x38, 0x37, 0x37, 0x37, 0x37, 0x37,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x35, 0x35, 0x35, 0x35, 0x35, 0x34, 0x34, 0x34, 0x34, 0x34,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3e, 0x3e, 0x3e, 0x3e, 0x3d, 0x3d, 0x3d, 0x3d, 0x3c, 0x3c, 0x3c, 0x3c,
	0x3b, 0x3b, 0x3b, 0x3b, 0x3a, 0x3a, 0x3a, 0x3a, 0x39, 0x39, 0x39, 0x39, 0x38, 0x38, 0x38, 0x38,
	0x37, 0x37, 0x37, 0x37, 0x36, 0x36, 0x36, 0x36, 0x35, 0x35, 0x35, 0x35, 0x34, 0x34, 0x34, 0x34,
	0x33, 0x33, 0x33, 0x33, 0x32, 0x32, 0x32, 0x32, 0x31, 0x31, 0x31, 0x31, 0x30, 0x30, 0x30, 0x30,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3e, 0x3e, 0x3e, 0x3d, 0x3d, 0x3d, 0x3c, 0x3c, 0x3c, 0x3b, 0x3b, 0x3b,
	0x3a, 0x3a, 0x3a, 0x3a, 0x39, 0x39, 0x39, 0x38, 0x38, 0x38, 0x37, 0x37, 0x37, 0x36, 0x36, 0x36,
	0x35, 0x35, 0x35, 0x35, 0x34, 0x34, 0x34, 0x33, 0x33, 0x33, 0x32, 0x32, 0x32, 0x31, 0x31, 0x31,
	0x30, 0x30, 0x30, 0x30, 0x2f, 0x2f, 0x2f, 0x2e, 0x2e, 0x2e, 0x2d, 0x2d, 0x2d, 0x2c, 0x2c, 0x2c,
	0x3f, 0x3f, 0x3f, 0x3e, 0x3e, 0x3e, 0x3d, 0x3d, 0x3c, 0x3c, 0x3c, 0x3b, 0x3b, 0x3b, 0x3a, 0x3a,
	0x39, 0x39, 0x39, 0x38, 0x38, 0x38, 0x37, 0x37, 0x36, 0x36, 0x36, 0x35, 0x35, 0x35, 0x34, 0x34,
	0x33, 0x33, 0x33, 0x32, 0x32, 0x32, 0x31, 0x31, 0x30, 0x30, 0x30, 0x2f, 0x2f, 0x2f, 0x2e, 0x2e,
	0x2d, 0x2d, 0x2d, 0x2c, 0x2c, 0x2c, 0x2b, 0x2b, 0x2a, 0x2a, 0x2a, 0x29, 0x29, 0x29, 0x28, 0x28,
	0x3f, 0x3f, 0x3f, 0x3e, 0x3e, 0x3d, 0x3d, 0x3c, 0x3c, 0x3c, 0x3b, 0x3b, 0x3a, 0x3a, 0x39, 0x39,
	0x38, 0x38, 0x38, 0x37, 0x37, 0x36, 0x36, 0x35, 0x35, 0x35, 0x34, 0x34, 0x33, 0x33, 0x32, 0x32,
	0x31, 0x31, 0x31, 0x30, 0x30, 0x2f, 0x2f, 0x2e, 0x2e, 0x2e, 0x2d, 0x2d, 0x2c, 0x2c, 0x2b, 0x2b,
	0x2a, 0x2a, 0x2a, 0x29, 0x29, 0x28, 0x28, 0x27, 0x27, 0x27, 0x26, 0x26, 0x25, 0x25, 0x24, 0x24,
	0x3f, 0x3f, 0x3e, 0x3e, 0x3d, 0x3d, 0x3c, 0x3c, 0x3b, 0x3b, 0x3a, 0x3a, 0x39, 0x39, 0x38, 0x38,
	0x37, 0x37, 0x36, 0x36, 0x35, 0x35, 0x34, 0x34, 0x33, 0x33, 0x32, 0x32, 0x31, 0x31, 0x30, 0x30,
	0x2f, 0x2f, 0x2e, 0x2e, 0x2d, 0x2d, 0x2c, 0x2c, 0x2b, 0x2b, 0x2a, 0x2a, 0x29, 0x29, 0x28, 0x28,
	0x27, 0x27, 0x26, 0x26, 0x25, 0x25, 0x24, 0x24, 0x23, 0x23, 0x22, 0x22, 0x21, 0x21, 0x20, 0x20,
	0x3f, 0x3f, 0x3e, 0x3e, 0x3d, 0x3d, 0x3c, 0x3c, 0x3b, 0x3a, 0x3a, 0x39, 0x39, 0x38, 0x38, 0x37,
	0x36, 0x36, 0x35, 0x35, 0x34, 0x34, 0x33, 0x33, 0x32, 0x31, 0x31, 0x30, 0x30, 0x2f, 0x2f, 0x2e,
	0x2d, 0x2d, 0x2c, 0x2c, 0x2b, 0x2b, 0x2a, 0x2a, 0x29, 0x28, 0x28, 0x27, 0x27, 0x26, 0x26, 0x25,
	0x24, 0x24, 0x23, 0x23, 0x22, 0x22, 0x21, 0x21, 0x20, 0x1f, 0x1f, 0x1e, 0x1e, 0x1d, 0x1d, 0x1c,
	0x3f, 0x3f, 0x3e, 0x3e, 0x3d, 0x3c, 0x3c, 0x3b, 0x3a, 0x3a, 0x39, 0x39, 0x38, 0x37, 0x37, 0x36,
	0x35, 0x35, 0x34, 0x34, 0x33, 0x32, 0x32, 0x31, 0x30, 0x30, 0x2f, 0x2f, 0x2e, 0x2d, 0x2d, 0x2c,
	0x2b, 0x2b, 0x2a, 0x2a, 0x29, 0x28, 0x28, 0x27, 0x26, 0x26, 0x25, 0x25, 0x24, 0x23, 0x23, 0x22,
	0x21, 0x21, 0x20, 0x20, 0x1f, 0x1e, 0x1e, 0x1d, 0x1c, 0x1c, 0x1b, 0x1b, 0x1a, 0x19, 0x19, 0x18,
	0x3f, 0x3f, 0x3e, 0x3d, 0x3d, 0x3c, 0x3b, 0x3b, 0x3a, 0x39, 0x39, 0x38, 0x37, 0x37, 0x36, 0x35,
	0x34, 0x34, 0x33, 0x32, 0x32, 0x31, 0x30, 0x30, 0x2f, 0x2e, 0x2e, 0x2d, 0x2c, 0x2c, 0x2b, 0x2a,
	0x29, 0x29, 0x28, 0x27, 0x27, 0x26, 0x25, 0x25, 0x24, 0x23, 0x23, 0x22, 0x21, 0x21, 0x20, 0x1f,
	0x1e, 0x1e, 0x1d, 0x1c, 0x1c, 0x1b, 0x1a, 0x1a, 0x19, 0x18, 0x18, 0x17, 0x16, 0x16, 0x15, 0x14,
	0x3f, 0x3f, 0x3e, 0x3d, 0x3c, 0x3c, 0x3b, 0x3a, 0x39, 0x39, 0x38, 0x37, 0x36, 0x36, 0x35, 0x34,
	0x33, 0x33, 0x32, 0x31, 0x30, 0x30, 0x2f, 0x2e, 0x2d, 0x2d, 0x2c, 0x2b, 0x2a, 0x2a, 0x29, 0x28,
	0x27, 0x27, 0x26, 0x25, 0x24, 0x24, 0x23, 0x22, 0x21, 0x21, 0x20, 0x1f, 0x1e, 0x1e, 0x1d, 0x1c,
	0x1b, 0x1b, 0x1a, 0x19, 0x18, 0x18, 0x17, 0x16, 0x15, 0x15, 0x14, 0x13, 0x12, 0x12, 0x11, 0x10,
	0x3f, 0x3f, 0x3e, 0x3d, 0x3c, 0x3b, 0x3b, 0x3a, 0x39, 0x38, 0x37, 0x37, 0x36, 0x35, 0x34, 0x33,
	0x32, 0x32, 0x31, 0x30, 0x2f, 0x2e, 0x2e, 0x2d, 0x2c, 0x2b, 0x2a, 0x2a, 0x29, 0x28, 0x27, 0x26,
	0x25, 0x25, 0x24, 0x23, 0x22, 0x21, 0x21, 0x20, 0x1f, 0x1e, 0x1d, 0x1d, 0x1c, 0x1b, 0x1a, 0x19,
	0x18, 0x18, 0x17, 0x16, 0x15, 0x14, 0x14, 0x13, 0x12, 0x11, 0x10, 0x10, 0x0f, 0x0e, 0x0d, 0x0c,
	0x3f, 0x3f, 0x3e, 0x3d, 0x3c, 0x3b, 0x3a, 0x39, 0x38, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32,
	0x31, 0x31, 0x30, 0x2f, 0x2e, 0x2d, 0x2c, 0x2b, 0x2a, 0x2a, 0x29, 0x28, 0x27, 0x26, 0x25, 0x24,
	0x23, 0x23, 0x22, 0x21, 0x20, 0x1f, 0x1e, 0x1d, 0x1c, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16,
	0x15, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08,
	0x3f, 0x3f, 0x3e, 0x3d, 0x3c, 0x3b, 0x3a, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31,
	0x30, 0x30, 0x2f, 0x2e, 0x2d, 0x2c, 0x2b, 0x2a, 0x29, 0x28, 0x27, 0x26, 0x25, 0x24, 0x23, 0x22,
	0x21, 0x21, 0x20, 0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13,
	0x12, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04,
	0x3f, 0x3e, 0x3d, 0x3c, 0x3b, 0x3a, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x30,
	0x2f, 0x2e, 0x2d, 0x2c, 0x2b, 0x2a, 0x29, 0x28, 0x27, 0x26, 0x25, 0x24, 0x23, 0x22, 0x21, 0x20,
	0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10,
	0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
};

struct OplLock {
	OplLock() { plat_sound_lock(); }
	~OplLock() { plat_sound_unlock(); }
};

static uint8_t *SoundAreaEnd;

/* The effect buffers; a script may read past its own sound into whatever the buffers hold. */
void SetAdlibSoundArea(uint8_t *start, int32_t size)
{
	SoundAreaEnd = start + size;
}

/* A word of the sound at that offset, which wraps at 64K as a DOS segment did. Past the effect
 * buffers DOS had other memory; zeros here. */
static uint16_t SoundWord(const uint8_t *sound, uint16_t at)
{
	uint16_t next = at + 1;
	uint8_t low = sound + at < SoundAreaEnd ? sound[at] : 0;
	uint8_t high = sound + next < SoundAreaEnd ? sound[next] : 0;

	return low | high << 8;
}

/* The operator offset of a channel; past the table, as on DOS, ChannelBusy. */
static uint8_t OpOffsetOf(uint16_t channel)
{
	return channel < NCHANNELS ? OpOffsets[channel] : ChannelBusy[channel - NCHANNELS];
}

/* Writes value to register reg unless the register already holds it. */
static void OplWrite(uint8_t reg, uint8_t value)
{
	if (OplRegs[reg] == value)
		return;
	OplRegs[reg] = value;
	plat_opl_write(ADLIB_PORT, reg);
	plat_opl_write(ADLIB_PORT + 1, value);
}

/* Writes it whatever the register held. */
static void OplWriteForced(uint8_t reg, uint8_t value)
{
	OplRegs[reg] = reg ^ 0xff;
	OplWrite(reg, value);
}

/* Reads envelope e's script up to its next step. A pair with no ticks jumps by its value, -1
 * ticks sets the envelope's value and -2 sets register bits. Ten such commands in a row end
 * the script. */
static void NextStep(AdlibVoice *v, int16_t e)
{
	Envelope *env = &v->env[e];
	uint16_t at = env->script, count;
	int16_t i;

	for (i = 0; i < 10; i++) {
		count = SoundWord(v->sound, at);
		env->count = count;
		env->step = SoundWord(v->sound, at + 2);
		at += 4;
		env->script = at;
		if (count == 0)
			at += env->step - 4;
		else if (count == 0xffff)
			env->value = env->step;
		else if (count == 0xfffe) {
			if (e == MULT1)
				v->multFlags = (v->multFlags & 0x00ff) | (env->step & 0xff) << 8;
			else if (e == MULT2)
				v->multFlags = (v->multFlags & 0xff00) | (env->step & 0xff);
			else if (e == LEVEL1)
				v->ksl = (v->ksl & 0x00ff) | (env->step & 0xff) << 8;
			else if (e == LEVEL2)
				v->ksl = (v->ksl & 0xff00) | (env->step & 0xff);
			else if (e == PITCH)
				v->keyon = env->step;
			else if (e == FEEDBACK)
				v->connect = env->step;
			else
				return;
		} else
			return;
	}
	/* the script went nowhere: hold still for good */
	env->step = 0;
	env->count = 0xffff;
}

static void StartEnvelopes(AdlibVoice *v)
{
	int16_t e;

	for (e = 0; e < ENVELOPES; e++)
		NextStep(v, e);
}

static void StepEnvelope(AdlibVoice *v, int16_t e)
{
	Envelope *env = &v->env[e];

	env->value += env->step;
	if (--env->count == 0)
		NextStep(v, e);
}

/* Writes the voice's current parameters to its channel, if it has one, then steps the envelopes
 * that run the same way held or released. */
static void UpdateVoice(AdlibVoice *v)
{
	uint16_t pitch;

	if (Channel != NCHANNELS) {
		OplWrite(OPL_LEVEL + Op1, ((uint16_t)~v->env[LEVEL1].value >> 10) | v->ksl >> 8);
		/* op2's level is scaled by the voice's volume row */
		OplWrite(OPL_LEVEL + Op2, VolumeTable[v->volume + (v->env[LEVEL2].value >> 10)] | (v->ksl & 0xff));
		OplWrite(OPL_FEEDBACK + Channel, ((v->env[FEEDBACK].value >> 12) & 0x0e) | v->connect >> 8);
		OplWrite(OPL_MULT + Op1, (v->env[MULT1].value >> 12) | v->multFlags >> 8);
		OplWrite(OPL_MULT + Op2, (v->env[MULT2].value >> 12) | (v->multFlags & 0xff));
		OplWrite(OPL_WAVE + Op1, v->env[WAVE].value >> 8);
		OplWrite(OPL_WAVE + Op2, v->env[WAVE].value & 0xff);
		/* the frequency is the pitch's top ten bits */
		pitch = v->env[PITCH].value >> 6;
		OplWrite(OPL_FNUM + Channel, pitch & 0xff);
		OplWrite(OPL_KEYON + Channel, (v->keyon >> 8) | pitch >> 8);
	}
	StepEnvelope(v, PITCH);
	StepEnvelope(v, FEEDBACK);
	StepEnvelope(v, MULT1);
	StepEnvelope(v, MULT2);
	StepEnvelope(v, PRIORITY);
	StepEnvelope(v, WAVE);
}

/* Steps a released voice's level envelope. Once the level is below 400h, or would pass zero, it
 * stays at zero and counts in Ended. */
static void FadeEnvelope(AdlibVoice *v, int16_t e)
{
	Envelope *env = &v->env[e];
	int16_t value = (int16_t)env->value;
	int16_t step = (int16_t)env->step;

	if (value == 0) {
		Ended++;
		return;
	}
	if (value > 0 && value < 1024)
		goto silence;
	if (step < 0 && (uint32_t)env->value + env->step < 0x10000)
		goto silence;
	env->value += env->step;
	if (--env->count == 0)
		NextStep(v, e);
	return;
silence:
	env->value = 0;
	env->step = 0;
	env->count = 0xffff;
	Ended++;
}

/* Gives the voice the VolumeTable row for volume 0 to 127. */
static void SetVolume(AdlibVoice *v, int16_t volume)
{
	v->volume = (volume & 0x78) << 3;
}

/* Keys off the voice's channel, quietens it, and loads the sound's AD and SR bytes for the
 * state. A sound whose scripts start straight after the header has none, and gets a slow
 * default. */
static void ProgramChannel(AdlibVoice *v, uint16_t state)
{
	uint16_t first = 0xff0f, second = 0xff0f;

	if (v->channel == NCHANNELS)
		return;
	if (SoundWord(v->sound, SND_PITCH + PART_ATTACK) != SND_ADSR) {
		first = SoundWord(v->sound, SND_ADSR);
		second = SoundWord(v->sound, SND_ADSR + 2);
		if (state == RELEASING) {
			first = SoundWord(v->sound, SND_RELEASE_ADSR);
			second = SoundWord(v->sound, SND_RELEASE_ADSR + 2);
		}
	}
	Op1 = OpOffsetOf(v->channel);
	Op2 = Op1 + 3;
	Channel = v->channel;
	OplWrite(OPL_KEYON + Channel, 0);
	OplWrite(OPL_LEVEL + Op1, 0x3f);
	OplWrite(OPL_LEVEL + Op2, 0x3f);
	OplWrite(OPL_AD + Op1, 0xff);
	OplWrite(OPL_AD + Op2, 0xff);
	OplWrite(OPL_SR + Op1, 0xff);
	OplWrite(OPL_SR + Op2, 0xff);
	OplWrite(OPL_AD + Op1, first >> 8);
	OplWrite(OPL_AD + Op2, second >> 8);
	OplWrite(OPL_SR + Op1, first & 0xff);
	OplWrite(OPL_SR + Op2, second & 0xff);
}

/* Starts the voice on the sound: every envelope on its attack script, then its channel set up
 * for the held note. */
static void AttackVoice(AdlibVoice *v, uint8_t *sound)
{
	int16_t e;

	v->sound = sound;
	v->multFlags = 0x2020;          /* sustain on for both operators */
	v->keyon = 0x2820;              /* key on, block 2 */
	v->ksl = 0;
	v->connect = 0;
	v->hold = SoundWord(sound, SND_HOLD);
	for (e = 0; e < ENVELOPES; e++) {
		v->env[e].value = SoundWord(sound, SoundPart[e] + PART_START);
		v->env[e].script = SoundWord(sound, SoundPart[e] + PART_ATTACK);
	}
	StartEnvelopes(v);
	ProgramChannel(v, HOLDING);
	v->state = HOLDING;
}

/* Every envelope moves to its release script and the channel takes the sound's released AD and
 * SR bytes. */
static void ReleaseVoice(AdlibVoice *v)
{
	uint16_t first = 0xff0f, second = 0xff0f;
	int16_t e;

	v->multFlags = 0x2020;
	v->keyon = 0x2820;
	v->ksl = 0;
	v->connect = 0;
	v->state--;
	for (e = 0; e < ENVELOPES; e++)
		v->env[e].script = SoundWord(v->sound, SoundPart[e] + PART_RELEASE);
	StartEnvelopes(v);
	if (v->channel == NCHANNELS)
		return;
	if (SoundWord(v->sound, SND_PITCH + PART_ATTACK) != SND_ADSR) {
		first = SoundWord(v->sound, SND_RELEASE_ADSR);
		second = SoundWord(v->sound, SND_RELEASE_ADSR + 2);
	}
	OplWrite(OPL_AD + Op1, first >> 8);
	OplWrite(OPL_AD + Op2, second >> 8);
	OplWrite(OPL_SR + Op1, first & 0xff);
	OplWrite(OPL_SR + Op2, second & 0xff);
}

/* Frees the voice and keys its channel off. The first voice left waiting for a channel takes it
 * over; otherwise the channel is free. */
static void FreeVoice(AdlibVoice *v)
{
	uint16_t channel = v->channel;
	int16_t i;

	v->state = 0;
	if (Channel != NCHANNELS)
		OplWrite(OPL_KEYON + Channel, 0);
	for (i = 0; i < NVOICES; i++)
		if (Voices[i].state != 0 && Voices[i].channel == NCHANNELS) {
			Voices[i].channel = channel;
			ProgramChannel(&Voices[i], Voices[i].state);
			return;
		}
	/* for no channel the original clears the byte after ChannelBusy, which nothing reads first */
	if ((channel & 0xff) < NCHANNELS)
		ChannelBusy[channel & 0xff] = 0;
}

/* With more voices playing than channels, hands the channels to the voices of highest priority,
 * taking one from a lower voice where needed. */
static void ShareChannels(void)
{
	int16_t i, best, high, low, at;
	int16_t value;

	Playing = 0;
	for (i = 0; i < NVOICES; i++) {
		value = 0;      /* a free voice ranks 0 */
		if (Voices[i].state != 0) {
			Playing++;
			value = Voices[i].env[PRIORITY].value;
			if (value == 0)
				value++;
			value -= 0x7fff;
			if (value == 0)
				value++;
		}
		Priority[i] = value;
	}
	if (Playing <= NCHANNELS)
		return;
	Playing = NCHANNELS;
	do {
		/* take the highest voice not yet placed */
		best = 0;
		high = -1;
		for (i = 0; i < NVOICES; i++)
			if (Priority[i] != 0 && Priority[i] > best) {
				high = i;
				best = Priority[i];
			}
		if (high < 0)
			return;
		best = Priority[high];
		Priority[high] = 0;
		if (Voices[high].channel == NCHANNELS) {
			/* it has no channel: take the one of the lowest voice holding one */
			low = -1;
			at = 0;
			for (i = 0; i < NVOICES; i++)
				if (Priority[i] != 0 && Priority[i] < best && Voices[i].channel != NCHANNELS) {
					low = i;
					best = Priority[i];
					at = i;
				}
			if (low < 0)
				return;
			Priority[at] = 0;
			Voices[high].channel = Voices[low].channel;
			ProgramChannel(&Voices[high], Voices[high].state);
			Voices[low].channel = NCHANNELS;
		}
	} while (--Playing != 0);
}

/* Starts the sound on a free voice at the volume; returns the voice's handle, or 0 when all are
 * busy. The voice also takes a free channel if there is one. */
int16_t StartAdlibSound(uint8_t *sound, int16_t volume)
{
	OplLock hold;
	AdlibVoice *v;
	int16_t i, channel;

	for (i = 0; i < NVOICES; i++)
		if (Voices[i].state == 0)
			break;
	if (i == NVOICES)
		return 0;
	v = &Voices[i];
	for (channel = 0; channel < NCHANNELS; channel++)
		if (ChannelBusy[channel] == 0) {
			ChannelBusy[channel]++;
			break;
		}
	v->channel = channel;
	SetVolume(v, volume);
	AttackVoice(v, sound);
	return i + 1;
}

/* Advances every playing voice, releases those whose hold has run out, frees those that have
 * faded, then shares out the channels. */
void TickAdlibSounds()
{
	OplLock hold;
	AdlibVoice *v;
	int16_t i;

	for (i = 0; i < NVOICES; i++) {
		v = &Voices[i];
		if (v->state == 0)
			continue;
		Channel = v->channel;
		Op1 = OpOffsetOf(v->channel);
		Op2 = Op1 + 3;
		if (v->state != HOLDING) {
			UpdateVoice(v);
			Ended = 0;
			FadeEnvelope(v, LEVEL1);
			FadeEnvelope(v, LEVEL2);
			if (Ended == 2)         /* both levels faded out */
				FreeVoice(v);
		} else {
			UpdateVoice(v);
			StepEnvelope(v, LEVEL1);
			StepEnvelope(v, LEVEL2);
			if (--v->hold == 0)
				ReleaseVoice(v);
		}
	}
	ShareChannels();
}

/* Resets the chip and every table, quietens each channel, and reports each channel handed back. */
void ResetAdlib(void)
{
	OplLock hold;
	int16_t i;

	/* so that every register is written below */
	memset(OplRegs, 0xff, sizeof OplRegs);
	memset(Voices, 0, sizeof Voices);
	for (i = 0; i < 256; i++)
		OplWrite(i, 0);
	memset(ChannelBusy, 0, sizeof ChannelBusy);
	OplWrite(OPL_TEST, 0x20);
	OplWrite(OPL_RHYTHM, 0);
	OplWrite(OPL_CSM, 0);
	for (i = 0; i < NCHANNELS; i++) {
		Op1 = OpOffsets[i];
		Op2 = Op1 + 3;
		OplWrite(OPL_LEVEL + Op1, 0x3f);
		OplWrite(OPL_LEVEL + Op2, 0x3f);
		OplWrite(OPL_AD + Op1, 0xff);
		OplWrite(OPL_AD + Op2, 0xff);
		OplWrite(OPL_SR + Op1, 0x0f);
		OplWrite(OPL_SR + Op2, 0x0f);
	}
	for (i = 0; i < NCHANNELS; i++)
		DriverReleaseChannelEntry(i);
}

static void StopVoice(AdlibVoice *v)
{
	if (v->state == 0)
		return;
	Channel = v->channel;
	Op1 = OpOffsetOf(v->channel);
	Op2 = Op1 + 3;
	FreeVoice(v);
}

void StopAdlibVoice(int16_t voice)
{
	OplLock hold;

	StopVoice(&Voices[voice - 1]);
}

void SetAdlibVolume(int16_t voice, int16_t volume)
{
	OplLock hold;

	SetVolume(&Voices[voice - 1], volume);
}

/* 0 once the voice has ended. */
int16_t GetAdlibVoiceState(int16_t voice)
{
	OplLock hold;

	return Voices[voice - 1].state;
}

/* Lends out channels 2 to 8: reports each one, marks it taken, and takes it from any voice using
 * it. */
void YieldAdlibChannels()
{
	OplLock hold;
	uint8_t busy;
	int16_t channel, i;

	if (Suspended != 0)
		return;
	for (channel = 2; channel < NCHANNELS; channel++) {
		DriverClaimChannelEntry(channel);
		busy = ChannelBusy[channel];
		ChannelBusy[channel] = 1;
		if (busy == 0)
			continue;
		for (i = 0; i < NVOICES; i++)
			if (Voices[i].state != 0 && Voices[i].channel == channel) {
				Voices[i].channel = NCHANNELS;
				OplWrite(OPL_KEYON + channel, 0);
			}
	}
	Suspended = 1;
}

/* Takes back channels 2 to 8: reports each one, resets its registers and marks it free. */
void ReclaimAdlibChannels(void)
{
	OplLock hold;
	int16_t channel;

	if (Suspended == 0)
		return;
	for (channel = 2; channel < NCHANNELS; channel++) {
		DriverReleaseChannelEntry(channel);
		Channel = channel;
		Op1 = OpOffsets[channel];
		Op2 = Op1 + 3;
		OplWriteForced(OPL_FNUM + Channel, 0);
		OplWriteForced(OPL_KEYON + Channel, 0);
		OplWriteForced(OPL_LEVEL + Op1, 0x3f);
		OplWriteForced(OPL_LEVEL + Op2, 0x3f);
		OplWriteForced(OPL_FEEDBACK + Channel, 0);
		OplWriteForced(OPL_MULT + Op1, 0);
		OplWriteForced(OPL_MULT + Op2, 0);
		OplWriteForced(OPL_WAVE + Op1, 0);
		OplWriteForced(OPL_WAVE + Op2, 0);
		OplWriteForced(OPL_AD + Op1, 0xff);
		OplWriteForced(OPL_AD + Op2, 0xff);
		OplWriteForced(OPL_SR + Op1, 0xff);
		OplWriteForced(OPL_SR + Op2, 0xff);
		ChannelBusy[channel] = 0;
	}
	Suspended = 0;
}

extern "C" void ResetAdlibGlobals(void)
{
	memset(Voices, 0, sizeof Voices);
	Ended = 0;
	memset(OplRegs, 0, sizeof OplRegs);
	Channel = 0;
	Op1 = 0;
	Op2 = 3;
	memset(ChannelBusy, 0, sizeof ChannelBusy);
	memset(Priority, 0, sizeof Priority);
	Playing = 0;
	Suspended = 0;
	SoundAreaEnd = 0;
}
