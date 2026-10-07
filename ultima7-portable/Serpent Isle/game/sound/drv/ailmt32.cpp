/* The Miles AIL 2.14 XMIDI driver for the Roland MT-32 (MT32MPU.ADV), rewritten natively. It
 * plays to the platform's MT-32.
 *
 *   main thread: the AIL_ calls below, under the sound lock
 *   sound tick (60 Hz): Mt32Serve twice, the driver's 120 Hz service
 *
 *   sequence layer: up to 8 sequences, each with a note queue, FOR/NEXT loops, channel locks,
 *                   indirect controllers, beat and bar counts, volume and tempo ramps
 *        |  channel messages on physical channels
 *   MT-32 layer:    64 memory timbres as a cache, patch banks, controller SysEx
 *
 * An XMIDI event stream is MIDI without running status or note-offs: a byte below 0x80 waits
 * that many 120 Hz intervals, and a note-on carries its length as a variable-length number.
 *
 * A sequence locks a channel to play on one of the MT-32's parts: the lowest-numbered busy
 * channel is taken from the others, whose messages for it are dropped until it is released.
 *
 * The driver waited for retraces after writing MT-32 memory, with interrupts off. On the main
 * thread that wait is real and holds the music back too. In the service it stalls the music:
 * the service stops after the event that waited and carries on once the wait has passed.
 */

#include "u7port.h"
#include "plat.h"
#include "aildrv.h"

/* No waits after MT-32 memory writes (a launcher switch, not in the original): Munt takes them
 * at once. */
extern "C" uint8_t Mt32ShortWaits = 0;

#define NUM_CHANS       16
#define MIN_TRUE_CHAN   2       /* 1-based: the channels open to locking */
#define MAX_TRUE_CHAN   9
#define MAX_REC_CHAN    10
#define MAX_NOTES       32
#define FOR_NEST        4
#define NSEQS           8
#define NUM_TIMBS       64
#define SYSEX_SIZE      32
#define SYSEX_Q_CNT     3
#define DEF_SYNTH_VOL   90      /* keeps the MT-32 from distorting */
#define STATE_TABLE_SIZE 520    /* what the driver asked the program to allocate per sequence */

#define QUANT_RATE      120
#define QUANT_TIME      8333                /* microseconds an interval */
#define QUANT_TIME_16   INT32_C(133333)     /* the same, times 16 */
#define RETRACE_US      14286

/* Controllers the driver acts on. */
#define MODULATION      1
#define PART_VOLUME     7
#define PANPOT          10
#define EXPRESSION      11
#define SYSEX_FIRST     32      /* three queues of start MSB, KSB, LSB, data byte, final byte */
#define SYSEX_LAST      46
#define RHYTHM_KEY_TIMB 58
#define PATCH_REVERB    59
#define PATCH_BENDER    60
#define REVERB_MODE     61
#define REVERB_TIME     62
#define REVERB_LEVEL    63
#define SUSTAIN         64
#define ALL_NOTES_OFF   123
#define CHAN_LOCK       110
#define CHAN_PROTECT    111
#define VOICE_PROTECT   112
#define TIMBRE_PROTECT  113
#define PATCH_BANK_SEL  114
#define INDIRECT_C_PFX  115
#define FOR_LOOP        116
#define NEXT_LOOP       117
#define CLEAR_BEAT_BAR  118
#define CALLBACK_TRIG   119
#define SEQ_INDEX       120

/* The controllers each sequence and the driver log, in this order. */
#define LOG_PV          0
#define LOG_SUS         4
#define LOG_C_LOCK      6
#define LOG_C_PROT      7
#define LOG_V_PROT      8
#define NUM_CONTROLS    9

#define LOCKED          0x80
#define LOCK_PROTECTED  0x40
#define TIMB_IN_USE     0x80
#define TIMB_PROTECTED  0x40

struct Sequence {
	uint8_t registered;
	const uint8_t *timb;        /* TIMB chunk, or 0 */
	const uint8_t *rbrn;        /* RBRN chunk, or 0 */
	const uint8_t *evnt;        /* EVNT chunk */
	const uint8_t *next;        /* the next event */
	uint8_t *controlTable;      /* where indirect controller values come from */
	int16_t lastCallback;
	int16_t started;
	int16_t status;
	int16_t postRelease;
	int16_t intervalCount;
	int16_t noteCount;
	uint16_t volPercent, volTarget;
	int32_t volAccum, volPeriod;
	int16_t tempoError;
	uint16_t tempoPercent, tempoTarget;
	int32_t tempoAccum, tempoPeriod;
	int16_t beatCount;
	int16_t measureCount;
	uint16_t timeNumerator;
	int32_t timeFraction;       /* 16 x microseconds an interval, scaled to the beat unit */
	int32_t beatFraction;
	int32_t timePerBeat;        /* 16 x microseconds a beat */
	const uint8_t *forStart[FOR_NEST];
	int16_t forCount[FOR_NEST];
	uint8_t chanMap[NUM_CHANS];
	int8_t chanProgram[NUM_CHANS];
	int8_t pitchLow[NUM_CHANS];
	int8_t pitchHigh[NUM_CHANS];
	int8_t chanIndirect[NUM_CHANS];
	int8_t controls[NUM_CONTROLS][NUM_CHANS];
	int8_t noteChan[MAX_NOTES];
	uint8_t noteNum[MAX_NOTES];
	int32_t noteTime[MAX_NOTES];
	uint8_t interrupted;        /* the service stopped in its events for a wait */
};

static const uint8_t LoggedControls[NUM_CONTROLS] = { PART_VOLUME, MODULATION, PANPOT,
	EXPRESSION, SUSTAIN, PATCH_BANK_SEL, CHAN_LOCK, CHAN_PROTECT, VOICE_PROTECT };
static const uint8_t ControlDefaults[NUM_CONTROLS] = { 127, 0, 64, 127, 0, 0, 0, 0, 0 };
static const int16_t ProgramDefaults[9] = { 68, 48, 95, 78, 41, 3, 110, 122, -1 };
static const uint8_t InitReverb[3] = { 0, 3, 2 };
static const uint8_t PartChannels[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
static const uint8_t PartReserve[9] = { 3, 4, 3, 4, 3, 4, 3, 4, 4 };
static const char DeviceNames[] = "Roland MT-32 or compatible with Roland MPU MIDI Interface\0"
	"Roland LAPC-1\0";

static drvr_desc Description;
static uint8_t InitOK;
static uint8_t InService;
static uint8_t MainWaiting;
static int32_t StallUs;
static uint8_t Resuming;
static int16_t ResumeSequence;
static Sequence Sequences[NSEQS];
static int16_t SequenceCount;

/* The driver's copy of every channel's controllers, program and pitch, from whichever
 * sequence set them last: what a released channel gets back. */
static int8_t GlobalControls[NUM_CONTROLS][NUM_CHANS];
static int8_t GlobalProgram[NUM_CHANS];
static int8_t GlobalPitchLow[NUM_CHANS];
static int8_t GlobalPitchHigh[NUM_CHANS];
static uint8_t ActiveNotes[NUM_CHANS];
static uint8_t LockStatus[NUM_CHANS];

/* The MT-32 side. Patch banks are indexed by any data byte. */
static uint8_t PatchBank[256];
static uint8_t SysexQueue[SYSEX_Q_CNT][SYSEX_SIZE];
static uint8_t AddrM[SYSEX_Q_CNT], AddrK[SYSEX_Q_CNT], AddrL[SYSEX_Q_CNT];
static uint8_t QueuePtr[SYSEX_Q_CNT];
static uint32_t NoteEvent;
static uint32_t TimbHist[NUM_TIMBS];
static uint8_t TimbBank[NUM_TIMBS];
static uint8_t TimbNum[NUM_TIMBS];
static uint8_t TimbAttribs[NUM_TIMBS];
static int8_t ChanTimbs[NUM_CHANS];
static uint8_t MidiBank[NUM_CHANS];
static uint8_t MidiProgram[NUM_CHANS];     /* 0xff: none yet */

static void SendMidi(uint8_t status, uint8_t d1, uint8_t d2);
static void ReleaseChannel(uint16_t channel);

/* ---- Output ---- */

static void Wait(uint16_t retraces)
{
	if (Mt32ShortWaits)
		return;
	if (InService) {
		StallUs += (int32_t) retraces * RETRACE_US;
		return;
	}
	if (retraces == 0)
		return;
	MainWaiting = 1;
	plat_sound_unlock();
	while (retraces-- > 0)
		plat_video_wait_retrace();
	plat_sound_lock();
	MainWaiting = 0;
}

static void SendBytes(const uint8_t *bytes, int16_t length)
{
	if (length > 0)
		plat_midi_send(bytes, length);
}

/* A Roland data set to address a, b, c, then a wait. */
static void SendSysex(uint8_t a, uint8_t b, uint8_t c, const uint8_t *data, uint16_t size,
	uint16_t wait)
{
	uint8_t message[SYSEX_SIZE * 2 + 10];
	uint16_t sum = a + b + c;
	int16_t n = 0;

	message[n++] = 0xf0;
	message[n++] = 0x41;
	message[n++] = 0x10;
	message[n++] = 0x16;
	message[n++] = 0x12;
	message[n++] = a;
	message[n++] = b;
	message[n++] = c;
	for (uint16_t i = 0; i < size; i++) {
		message[n++] = data[i];
		sum += data[i];
	}
	message[n++] = (uint8_t) ((0x80 - (sum & 0x7f)) & 0x7f);
	message[n++] = 0xf7;
	SendBytes(message, n);
	Wait(wait);
}

/* Adds to a 21-bit address kept as three 7-bit bytes. */
static void AddAddress(uint16_t addend, uint8_t *m, uint8_t *k, uint8_t *l)
{
	uint16_t low = (uint16_t) (*l + addend), mid = *k, high = *m;

	while (low >= 0x80) {
		low -= 0x80;
		mid++;
	}
	while (mid >= 0x80) {
		mid -= 0x80;
		high++;
	}
	*l = (uint8_t) low;
	*k = (uint8_t) mid;
	*m = (uint8_t) high;
}

static void WriteSystem(uint8_t index, uint8_t value)
{
	SendSysex(0x10, 0, index, &value, 1, 0);
}

static void WriteRhythmSetup(uint8_t key, uint8_t value)
{
	uint8_t m = 3, k = 1, l = 0x10;

	AddAddress((uint16_t) ((key - 24) * 4), &m, &k, &l);
	SendSysex(m, k, l, &value, 1, 4);
}

static void WritePatch(uint8_t patch, uint8_t index, const uint8_t *value, uint16_t size)
{
	uint8_t m = 5, k = 0, l = 0;

	AddAddress((uint16_t) (patch * 8 + index), &m, &k, &l);
	SendSysex(m, k, l, value, size, 2);
}

/* ---- MT-32 layer ---- */

/* The cache slot holding a timbre, or -1. */
static int16_t IndexTimbre(uint8_t bank, uint8_t patch)
{
	for (int16_t i = 0; i < NUM_TIMBS; i++) {
		if ((TimbAttribs[i] & TIMB_IN_USE) && TimbBank[i] == bank && TimbNum[i] == patch)
			return i;
	}
	return -1;
}

/* Points the patch at its bank's timbre: a cached memory timbre, or the built-in one. */
static void SetupPatch(uint8_t patch, uint8_t bank)
{
	uint8_t setting[2];
	int16_t slot = -1;

	PatchBank[patch] = bank;
	if (bank != 0)
		slot = IndexTimbre(bank, patch);
	if (slot >= 0) {
		setting[0] = 2;
		setting[1] = (uint8_t) slot;
	} else {
		setting[0] = patch < 64 ? 0 : 1;
		setting[1] = patch & 63;
	}
	WritePatch(patch, 0, setting, 2);
}

/* Controllers 32-46 fill three SysEx queues; a final byte, or a full queue, sends one. The
 * final byte's address stays the last one written, so later final bytes rewrite it. */
static void SysexController(uint8_t con, uint8_t value)
{
	uint8_t q = (con - SYSEX_FIRST) / 5;
	uint8_t op = (con - SYSEX_FIRST) % 5;
	uint16_t length;

	switch (op) {
	case 0:
		AddrM[q] = value;
		return;
	case 1:
		AddrK[q] = value;
		return;
	case 2:
		AddrL[q] = value;
		return;
	}
	SysexQueue[q][QueuePtr[q]] = value;
	if (op == 3 && QueuePtr[q] < SYSEX_SIZE - 1) {
		QueuePtr[q]++;
		return;
	}
	length = QueuePtr[q] + 1;
	SendSysex(AddrM[q], AddrK[q], AddrL[q], SysexQueue[q], length, 0);
	if (op == 3)
		Wait(4);
	else
		length--;
	AddAddress(length, &AddrM[q], &AddrK[q], &AddrL[q]);
	QueuePtr[q] = 0;
}

/* A channel message on its way to the synth. Notes count toward the timbre cache's
 * least-recently-used order; a program change first points the patch at the channel's bank. */
static void SendMidi(uint8_t status, uint8_t d1, uint8_t d2)
{
	uint8_t chan = status & 0x0f;
	uint8_t message[3];

	switch (status & 0xf0) {
	case 0xb0:
		if (d1 < SYSEX_FIRST)
			break;
		if (d1 <= SYSEX_LAST) {
			SysexController(d1, d2);
			return;
		}
		switch (d1) {
		case PATCH_REVERB:
		case PATCH_BENDER:
			if (MidiProgram[chan] != 0xff) {
				WritePatch(MidiProgram[chan], d1 == PATCH_REVERB ? 6 : 4, &d2, 1);
				message[0] = 0xc0 | chan;
				message[1] = MidiProgram[chan];
				SendBytes(message, 2);
			}
			return;
		case REVERB_MODE:
			WriteSystem(1, d2);
			return;
		case REVERB_TIME:
			WriteSystem(2, d2);
			return;
		case REVERB_LEVEL:
			WriteSystem(3, d2);
			return;
		case PATCH_BANK_SEL:
			MidiBank[chan] = d2;
			return;
		case RHYTHM_KEY_TIMB:
			if (ChanTimbs[chan] != -1)
				WriteRhythmSetup(d2, (uint8_t) ChanTimbs[chan]);
			return;
		case TIMBRE_PROTECT:
			if (ChanTimbs[chan] != -1) {
				uint8_t *attribs = &TimbAttribs[(uint8_t) ChanTimbs[chan]];

				*attribs &= ~TIMB_PROTECTED;
				if ((int8_t) d2 >= 64)
					*attribs |= TIMB_PROTECTED;
			}
			return;
		}
		/* the player's own controllers never reach the synth */
		if (d1 >= CHAN_LOCK && d1 <= SEQ_INDEX)
			return;
		break;
	case 0xc0:
		MidiProgram[chan] = d1;
		if (MidiBank[chan] != PatchBank[d1])
			SetupPatch(d1, MidiBank[chan]);
		ChanTimbs[chan] = (int8_t) IndexTimbre(MidiBank[chan], d1);
		break;
	case 0x90:
		NoteEvent++;
		if (ChanTimbs[chan] != -1)
			TimbHist[(uint8_t) ChanTimbs[chan]] = NoteEvent;
		break;
	}
	message[0] = status;
	message[1] = d1;
	message[2] = d2;
	SendBytes(message, (status & 0xf0) == 0xc0 || (status & 0xf0) == 0xd0 ? 2 : 3);
}

/* Clears the MT-32 and puts its parts on channels 2-10. */
static void ResetSynth(void)
{
	SendSysex(0x7f, 0, 0, PartChannels, 1, 12);
	SendSysex(0x10, 0, 0x0d, PartChannels, 9, 4);
}

static void InitSynth(void)
{
	SendSysex(0x10, 0, 0x04, PartReserve, 9, 4);
	SendSysex(0x10, 0, 0x01, InitReverb, 3, 4);
	memset(QueuePtr, 0, sizeof QueuePtr);
	NoteEvent = 0;
	memset(TimbAttribs, 0, sizeof TimbAttribs);
	memset(ChanTimbs, -1, sizeof ChanTimbs);
	memset(MidiProgram, 0xff, sizeof MidiProgram);
	memset(MidiBank, 0, sizeof MidiBank);
	memset(PatchBank, 0, sizeof PatchBank);
}

/* A 20-character message on the MT-32's display. */
static void WriteDisplay(const char *string)
{
	char text[20];

	if (string == 0)
		return;
	memset(text, ' ', sizeof text);
	for (int16_t i = 0; i < 20 && string[i]; i++)
		text[i] = string[i];
	SendSysex(0x20, 0, 0, (const uint8_t *) text, 20, 4);
}

/* Takes a free cache slot, or the least recently played unprotected one, and uploads the timbre
 * there in five pieces: common part and four partials. */
static void InstallTimbre(uint8_t bank, uint8_t patch, const uint8_t *timbre)
{
	int16_t slot;
	uint8_t high;

	if (bank == 127)
		return;
	if (bank != 0 && IndexTimbre(bank, patch) == -1) {
		if (timbre == 0)
			return;
		for (slot = 0; slot < NUM_TIMBS && (TimbAttribs[slot] & TIMB_IN_USE); slot++)
			;
		if (slot == NUM_TIMBS) {
			uint32_t oldest = UINT32_MAX;

			slot = -1;
			for (int16_t i = 0; i < NUM_TIMBS; i++) {
				if (!(TimbAttribs[i] & TIMB_PROTECTED) && TimbHist[i] <= oldest) {
					oldest = TimbHist[i];
					slot = i;
				}
			}
			if (slot == -1)
				return;
		}
		TimbHist[slot] = NoteEvent++;
		TimbNum[slot] = patch;
		TimbBank[slot] = bank;
		TimbAttribs[slot] = TIMB_IN_USE;
		timbre += 2;
		high = (uint8_t) (slot * 2);
		SendSysex(8, high, 0x00, timbre, 0x0e, 3);
		SendSysex(8, high, 0x0e, timbre + 0x0e, 0x3a, 3);
		SendSysex(8, high, 0x48, timbre + 0x48, 0x3a, 3);
		SendSysex(8, high + 1, 0x02, timbre + 0x82, 0x3a, 3);
		SendSysex(8, high + 1, 0x3c, timbre + 0xbc, 0x3a, 3);
	}
	SetupPatch(patch, bank);
}

/* ---- Sequence layer ---- */

static uint32_t ReadBigLong(const uint8_t *p)
{
	return (uint32_t) p[0] << 24 | (uint32_t) p[1] << 16 | (uint32_t) p[2] << 8 | p[3];
}

static uint16_t ReadWord(const uint8_t *p)
{
	return (uint16_t) (p[0] | p[1] << 8);
}

/* A variable-length number; moves p past it. */
static uint32_t ReadVarLength(const uint8_t **p)
{
	uint32_t value = 0;
	uint8_t byte;

	do {
		byte = *(*p)++;
		value = value << 7 | (byte & 0x7f);
	} while (byte & 0x80);
	return value;
}

static int16_t ControlIndex(uint8_t con)
{
	for (int16_t i = 0; i < NUM_CONTROLS; i++) {
		if (LoggedControls[i] == con)
			return i;
	}
	return -1;
}

static Sequence *GetSequence(HSEQUENCE handle)
{
	return handle >= 0 && handle < NSEQS ? &Sequences[handle] : 0;
}

/* FORM XMID number n, alone or inside a CAT; 0 if absent. */
static const uint8_t *FindSequence(const uint8_t *p, uint16_t n)
{
	int32_t left;

	for (;;) {
		if (memcmp(p, "CAT ", 4) != 0 && memcmp(p, "FORM", 4) != 0)
			return 0;
		if (memcmp(p + 8, "XMID", 4) == 0)
			break;
		p += ReadBigLong(p + 4) + 8;
	}
	left = (int32_t) ReadBigLong(p + 4) - 5;
	if (memcmp(p, "FORM", 4) == 0)
		return n == 0 ? p : 0;
	for (p += 12; ; ) {
		if (memcmp(p + 8, "XMID", 4) == 0 && n-- == 0)
			return p;
		left -= (int32_t) ReadBigLong(p + 4) + 8;
		if (left < 0)
			return 0;
		p += ReadBigLong(p + 4) + 8;
	}
}

static void Rewind(Sequence *s)
{
	for (int16_t i = 0; i < FOR_NEST; i++)
		s->forCount[i] = -1;
	for (uint8_t chan = 0; chan < NUM_CHANS; chan++) {
		s->chanMap[chan] = chan;
		s->chanProgram[chan] = -1;
		s->pitchLow[chan] = -1;
		s->pitchHigh[chan] = -1;
		s->chanIndirect[chan] = -1;
	}
	memset(s->controls, -1, sizeof s->controls);
	memset(s->noteChan, -1, sizeof s->noteChan);
	s->lastCallback = -1;
	s->intervalCount = 0;
	s->noteCount = 0;
	s->volPercent = DEF_SYNTH_VOL;
	s->volTarget = DEF_SYNTH_VOL;
	s->tempoPercent = 100;
	s->tempoTarget = 100;
	s->tempoError = 0;
	s->beatCount = 0;
	s->measureCount = -1;
	s->beatFraction = 0;
	s->timeFraction = 0;
	s->timeNumerator = 4;
	s->timePerBeat = INT32_C(500000) * 16;
	s->interrupted = 0;
}

/* Note-offs for every sequence's notes on a logical channel. */
static void FlushChannelNotes(uint8_t chan)
{
	for (int16_t n = 0; n < NSEQS; n++) {
		Sequence *s = &Sequences[n];

		if (!s->registered || s->noteCount == 0)
			continue;
		for (int16_t i = 0; i < MAX_NOTES; i++) {
			if (s->noteChan[i] == (int8_t) chan) {
				uint8_t phys = s->chanMap[chan];

				s->noteChan[i] = -1;
				ActiveNotes[phys]--;
				SendMidi(0x80 | phys, s->noteNum[i], 0);
				s->noteCount--;
			}
		}
	}
}

static void FlushNoteQueue(Sequence *s)
{
	for (int16_t i = 0; i < MAX_NOTES; i++) {
		if (s->noteChan[i] != -1) {
			uint8_t phys = s->chanMap[(uint8_t) s->noteChan[i]];

			s->noteChan[i] = -1;
			ActiveNotes[phys]--;
			SendMidi(0x80 | phys, s->noteNum[i], 0);
		}
	}
	s->noteCount = 0;
}

/* Lets go of what the sequence held: sustain pedals, locked channels, protections. */
static void ResetSequence(Sequence *s)
{
	for (uint8_t chan = 0; chan < NUM_CHANS; chan++) {
		if (s->controls[LOG_SUS][chan] >= 64) {
			GlobalControls[LOG_SUS][chan] = 0;
			SendMidi(0xb0 | chan, SUSTAIN, 0);
		}
		if (s->controls[LOG_C_LOCK][chan] >= 64) {
			FlushChannelNotes(chan);
			ReleaseChannel(s->chanMap[chan] + 1);
			s->chanMap[chan] = chan;
		}
		if (s->controls[LOG_C_PROT][chan] >= 64)
			LockStatus[chan] &= ~LOCK_PROTECTED;
		if (s->controls[LOG_V_PROT][chan] >= 64)
			SendMidi(0xb0 | chan, VOICE_PROTECT, 0);
	}
}

/* Sends each channel's logged volume, scaled to the sequence's volume. */
static void SendVolumes(Sequence *s)
{
	for (uint8_t chan = 0; chan < NUM_CHANS; chan++) {
		uint32_t volume;

		if (s->controls[LOG_PV][chan] == -1)
			continue;
		volume = (uint32_t) (uint8_t) s->controls[LOG_PV][chan] * s->volPercent / 100;
		if (volume >= 127)
			volume = 127;
		GlobalControls[LOG_PV][chan] = (int8_t) volume;
		if (LockStatus[chan] & LOCKED)
			continue;
		SendMidi(0xb0 | s->chanMap[chan], PART_VOLUME, (uint8_t) volume);
	}
}

/* Takes the physical channel with the fewest notes playing, the highest on a tie, skipping
 * locked and lock-protected ones; failing that, ignores protection. 1-based, 0 for none. */
static uint16_t LockChannel(void)
{
	uint8_t mask = LOCKED | LOCK_PROTECTED;
	uint8_t fewest = 0xff;
	int16_t found = -1;

	for (;;) {
		for (int16_t chan = MAX_TRUE_CHAN - 1; chan >= MIN_TRUE_CHAN - 1; chan--) {
			if ((LockStatus[chan] & mask) || ActiveNotes[chan] >= fewest)
				continue;
			fewest = ActiveNotes[chan];
			found = chan;
		}
		if (found != -1)
			break;
		if (mask == LOCKED)
			return 0;
		mask = LOCKED;
	}
	SendMidi(0xb0 | found, SUSTAIN, 0);
	FlushChannelNotes((uint8_t) found);
	ActiveNotes[found] = 0;
	LockStatus[found] |= LOCKED;
	return (uint16_t) (found + 1);
}

/* Silences a locked channel and gives it back its controllers, program and pitch. */
static void ReleaseChannel(uint16_t channel)
{
	uint8_t chan = (uint8_t) (channel - 1);

	if (channel < 1 || channel > NUM_CHANS || !(LockStatus[chan] & LOCKED))
		return;
	LockStatus[chan] &= ~LOCKED;
	ActiveNotes[chan] = 0;
	SendMidi(0xb0 | chan, SUSTAIN, 0);
	SendMidi(0xb0 | chan, ALL_NOTES_OFF, 0);
	for (int16_t i = 0; i < NUM_CONTROLS; i++) {
		if (GlobalControls[i][chan] != -1)
			SendMidi(0xb0 | chan, LoggedControls[i], (uint8_t) GlobalControls[i][chan]);
	}
	if (GlobalProgram[chan] != -1)
		SendMidi(0xc0 | chan, (uint8_t) GlobalProgram[chan], 0);
	if (GlobalPitchLow[chan] != -1 && GlobalPitchHigh[chan] != -1)
		SendMidi(0xe0 | chan, (uint8_t) GlobalPitchLow[chan], (uint8_t) GlobalPitchHigh[chan]);
}

/* A controller change from the sequence or the program. */
static void XmidiControl(Sequence *s, uint8_t chan, uint8_t con, uint8_t value)
{
	int16_t logged;
	int16_t i;

	if (s->chanIndirect[chan] != -1) {
		uint8_t index = (uint8_t) s->chanIndirect[chan];

		s->chanIndirect[chan] = -1;
		if (s->controlTable)
			value = s->controlTable[index];
	}
	logged = ControlIndex(con);
	if (logged != -1) {
		GlobalControls[logged][chan] = (int8_t) value;
		s->controls[logged][chan] = (int8_t) value;
	}
	switch (con) {
	case PART_VOLUME:
		if (s->volPercent != 100) {
			uint32_t volume = (uint32_t) value * s->volPercent / 100;

			value = volume < 127 ? (uint8_t) volume : 127;
			GlobalControls[LOG_PV][chan] = (int8_t) value;
		}
		break;
	case CLEAR_BEAT_BAR:
		s->beatCount = 0;
		s->measureCount = 0;
		s->beatFraction = -s->timeFraction;
		return;
	case CALLBACK_TRIG:
		s->lastCallback = value;
		return;
	case FOR_LOOP:
		for (i = 0; i < FOR_NEST; i++) {
			if (s->forCount[i] == -1) {
				s->forCount[i] = value;
				/* NEXT comes back to the FOR controller and steps past it */
				s->forStart[i] = s->next;
				break;
			}
		}
		return;
	case NEXT_LOOP:
		if ((int8_t) value < 64)
			return;
		for (i = FOR_NEST - 1; i >= 0 && s->forCount[i] == -1; i--)
			;
		if (i < 0)
			return;
		if (s->forCount[i] != 0 && --s->forCount[i] == 0) {
			s->forCount[i] = -1;
			return;
		}
		s->next = s->forStart[i];
		return;
	case CHAN_PROTECT:
		LockStatus[chan] |= LOCK_PROTECTED;
		if ((int8_t) value < 64)
			LockStatus[chan] &= ~LOCK_PROTECTED;
		return;
	case CHAN_LOCK:
		if ((int8_t) value >= 64) {
			int16_t locked = (int16_t) LockChannel() - 1;

			s->chanMap[chan] = locked == -1 ? chan : (uint8_t) locked;
		} else {
			FlushChannelNotes(chan);
			ReleaseChannel(s->chanMap[chan] + 1);
			s->chanMap[chan] = chan;
		}
		return;
	case INDIRECT_C_PFX:
		s->chanIndirect[chan] = (int8_t) value;
		return;
	}
	if (LockStatus[chan] & LOCKED)
		return;
	SendMidi(0xb0 | s->chanMap[chan], con, value);
}

/* Puts back what the sequence owned when it was stopped. */
static void RestoreSequence(Sequence *s)
{
	for (uint8_t chan = 0; chan < NUM_CHANS; chan++) {
		if (s->controls[LOG_C_LOCK][chan] >= 64) {
			int16_t locked = (int16_t) LockChannel() - 1;

			s->chanMap[chan] = locked == -1 ? chan : (uint8_t) locked;
		}
	}
	for (int16_t i = 0; i < NUM_CONTROLS; i++) {
		if (LoggedControls[i] == CHAN_LOCK)
			continue;
		for (uint8_t chan = 0; chan < NUM_CHANS; chan++) {
			if (s->controls[i][chan] != -1)
				XmidiControl(s, chan, LoggedControls[i], (uint8_t) s->controls[i][chan]);
		}
	}
	for (uint8_t chan = 0; chan < NUM_CHANS; chan++) {
		if (s->pitchLow[chan] != -1 && s->pitchHigh[chan] != -1)
			SendMidi(0xe0 | s->chanMap[chan], (uint8_t) s->pitchLow[chan],
				(uint8_t) s->pitchHigh[chan]);
		if (s->chanProgram[chan] != -1)
			SendMidi(0xc0 | s->chanMap[chan], (uint8_t) s->chanProgram[chan], 0);
	}
}

/* A note with its length; returns the event's size. A full queue loses its first note's
 * note-off. */
static int16_t NoteOn(Sequence *s)
{
	const uint8_t *event = s->next;
	const uint8_t *p = event + 3;
	uint8_t chan = event[0] & 0x0f;
	uint32_t duration = ReadVarLength(&p);
	int16_t slot;
	uint8_t phys;

	if (LockStatus[chan] & LOCKED)
		return (int16_t) (p - event);
	for (slot = 0; slot < MAX_NOTES && s->noteChan[slot] != -1; slot++)
		;
	if (slot == MAX_NOTES)
		slot = 0;
	else
		s->noteCount++;
	s->noteChan[slot] = (int8_t) chan;
	s->noteNum[slot] = event[1];
	s->noteTime[slot] = (int32_t) duration - 1;
	phys = s->chanMap[chan];
	ActiveNotes[phys]++;
	SendMidi(0x90 | phys, event[1], event[2]);
	return (int16_t) (p - event);
}

static void ReleaseSequence(int16_t handle)
{
	Sequence *s = &Sequences[handle];

	if (!s->registered)
		return;
	if (s->status == SEQ_PLAYING) {
		s->postRelease = 1;
		return;
	}
	s->registered = 0;
	SequenceCount--;
}

/* End of track, time signature and tempo; returns the event's size. */
static int16_t MetaEvent(Sequence *s, int16_t handle)
{
	const uint8_t *event = s->next;
	const uint8_t *p = event + 2;
	uint32_t length = ReadVarLength(&p);
	int16_t size = (int16_t) (length + (p - event));
	int16_t shift;

	switch (event[1]) {
	case 0x2f:
		ResetSequence(s);
		s->status = SEQ_DONE;
		if (s->postRelease)
			ReleaseSequence(handle);
		break;
	case 0x58:
		s->timeNumerator = p[0];
		shift = p[1] - 2;
		if (shift >= 0)
			s->timeFraction = QUANT_TIME_16 * (INT32_C(1) << shift);
		else
			s->timeFraction = (int32_t) ((uint32_t) QUANT_TIME_16 >> -shift);
		s->beatFraction = -s->timeFraction;
		s->beatCount = 0;
		s->measureCount++;
		break;
	case 0x51:
		s->timePerBeat = (int32_t) ((uint32_t) p[0] << 16 | p[1] << 8 | p[2]) << 4;
		break;
	}
	return size;
}

/* F0 goes out with its data; F7 sends the data alone. Returns the event's size. */
static int16_t SysexEvent(Sequence *s)
{
	const uint8_t *event = s->next;
	const uint8_t *p = event + 1;
	uint16_t length = (uint16_t) ReadVarLength(&p);

	if (event[0] == 0xf0)
		SendBytes(event, 1);
	SendBytes(p, (int16_t) length);
	return (int16_t) (length + (p - event));
}

/* The driver compared the low halves of these 32-bit counts as signed numbers. */
static uint8_t BeatReached(int32_t fraction, int32_t perBeat)
{
	int16_t high = (int16_t) (fraction >> 16), beatHigh = (int16_t) (perBeat >> 16);

	if (high != beatHigh)
		return high > beatHigh;
	return (int16_t) fraction >= (int16_t) perBeat;
}

/* Plays events until the next interval or the end. Stops after an event that waited, and
 * returns 0 then. */
static uint8_t PlayEvents(Sequence *s, int16_t handle)
{
	for (;;) {
		const uint8_t *event = s->next;
		uint8_t status = event[0] & 0xf0, chan = event[0] & 0x0f;
		int16_t size;

		if (StallUs > 0) {
			s->interrupted = 1;
			return 0;
		}
		if (event[0] < 0x80) {
			s->next++;
			s->intervalCount = event[0];
			return 1;
		}
		switch (status) {
		case 0xf0:
			size = chan == 0x0f ? MetaEvent(s, handle) : SysexEvent(s);
			break;
		case 0xb0:
			XmidiControl(s, chan, event[1], event[2]);
			size = 3;
			break;
		case 0xe0:
			s->pitchLow[chan] = (int8_t) event[1];
			s->pitchHigh[chan] = (int8_t) event[2];
			GlobalPitchLow[chan] = (int8_t) event[1];
			GlobalPitchHigh[chan] = (int8_t) event[2];
			size = 3;
			if (!(LockStatus[chan] & LOCKED))
				SendMidi(status | s->chanMap[chan], event[1], event[2]);
			break;
		case 0xc0:
			s->chanProgram[chan] = (int8_t) event[1];
			GlobalProgram[chan] = (int8_t) event[1];
			size = 2;
			if (!(LockStatus[chan] & LOCKED))
				SendMidi(status | s->chanMap[chan], event[1], event[2]);
			break;
		case 0xd0:
			size = 2;
			if (!(LockStatus[chan] & LOCKED))
				SendMidi(status | s->chanMap[chan], event[1], event[2]);
			break;
		case 0xa0:
			size = 3;
			if (!(LockStatus[chan] & LOCKED))
				SendMidi(status | s->chanMap[chan], event[1], event[2]);
			break;
		default:
			size = NoteOn(s);
			break;
		}
		s->next += size;
		if (s->status != SEQ_PLAYING)
			return 1;
	}
}

/* A step of a volume or tempo ramp: how many percent it moves this service. */
static uint16_t RampStep(int32_t *accum, int32_t period)
{
	uint16_t steps = 0;

	*accum += QUANT_TIME / 100;
	while (*accum - period >= 0) {
		*accum -= period;
		steps++;
	}
	return steps;
}

static void Ramp(uint16_t *percent, uint16_t target, uint16_t steps)
{
	if ((int16_t) *percent < (int16_t) target) {
		*percent += steps;
		if ((int16_t) *percent > (int16_t) target)
			*percent = target;
	} else {
		*percent -= steps;
		if ((int16_t) *percent < (int16_t) target)
			*percent = target;
	}
}

/* One service of one sequence: as many intervals as its tempo gives, each turning off expired
 * notes, then playing what is due and counting beats; then the ramps. Resuming carries on in
 * the events where a wait stopped it. Returns 0 when stopped by a wait again. */
static uint8_t ServeSequence(Sequence *s, int16_t handle, uint8_t resume)
{
	int16_t error;

	if (!resume) {
		error = (int16_t) (s->tempoError + s->tempoPercent);
		s->tempoError = error;
		error -= 100;
		if (error < 0)
			goto ramps;
		s->tempoError = error;
	}
	for (;;) {
		if (resume) {
			s->interrupted = 0;
			resume = 0;
			if (!PlayEvents(s, handle))
				return 0;
			if (s->status != SEQ_PLAYING)
				return 1;
		} else {
			for (int16_t i = 0; i < MAX_NOTES && s->noteCount; i++) {
				if (s->noteChan[i] != -1 && --s->noteTime[i] < 0) {
					uint8_t phys = s->chanMap[(uint8_t) s->noteChan[i]];

					s->noteChan[i] = -1;
					ActiveNotes[phys]--;
					SendMidi(0x80 | phys, s->noteNum[i], 0);
					s->noteCount--;
				}
			}
			if (--s->intervalCount <= 0) {
				if (!PlayEvents(s, handle))
					return 0;
				if (s->status != SEQ_PLAYING)
					return 1;
			}
		}
		s->beatFraction += s->timeFraction;
		if (BeatReached(s->beatFraction, s->timePerBeat)) {
			s->beatFraction -= s->timePerBeat;
			if ((uint16_t) ++s->beatCount >= s->timeNumerator) {
				s->beatCount = 0;
				s->measureCount++;
			}
		}
		error = (int16_t) (s->tempoError - 100);
		if (error < 0)
			break;
		s->tempoError = error;
	}
ramps:
	if (s->tempoPercent != s->tempoTarget) {
		uint16_t steps = RampStep(&s->tempoAccum, s->tempoPeriod);

		if (steps)
			Ramp(&s->tempoPercent, s->tempoTarget, steps);
	}
	if (s->volPercent != s->volTarget) {
		uint16_t steps = RampStep(&s->volAccum, s->volPeriod);

		if (steps) {
			Ramp(&s->volPercent, s->volTarget, steps);
			SendVolumes(s);
		}
	}
	return 1;
}

/* The 120 Hz service. A wait in it holds everything back until it has passed. */
void Mt32Serve(void)
{
	int16_t n = 0;
	uint8_t resume = 0;

	if (!InitOK || MainWaiting)
		return;
	if (StallUs > 0) {
		StallUs -= QUANT_TIME;
		return;
	}
	if (Resuming) {
		Resuming = 0;
		n = ResumeSequence;
		resume = 1;
	}
	InService = 1;
	for (; n < NSEQS; n++, resume = 0) {
		Sequence *s = &Sequences[n];

		if (!s->registered || s->status != SEQ_PLAYING)
			continue;
		if (resume && !s->interrupted)
			resume = 0;
		if (!ServeSequence(s, n, resume)) {
			Resuming = 1;
			ResumeSequence = n;
			break;
		}
	}
	InService = 0;
}

/* The beat and bar the sequence will be at one interval on. */
static void AdvancedCount(Sequence *s, uint16_t *beat, int16_t *bar)
{
	int32_t fraction = s->beatFraction + s->timeFraction;

	*beat = (uint16_t) s->beatCount;
	*bar = s->measureCount;
	if (BeatReached(fraction, s->timePerBeat)) {
		if (++*beat >= s->timeNumerator) {
			*beat = 0;
			++*bar;
		}
	}
	if (*bar < 0)
		*bar = 0;
}

/* The number of 100-microsecond units between ramp steps of one percent. */
static int32_t RampPeriod(uint16_t from, uint16_t to, uint16_t milliseconds)
{
	int16_t delta = (int16_t) (to - from);
	uint32_t period = (uint32_t) milliseconds * 10 / (uint16_t) (delta < 0 ? -delta : delta);

	return period ? (int32_t) period : 1;
}

/* ---- Driver ---- */

drvr_desc *Mt32Describe(void)
{
	Description.min_API_version = 200;
	Description.drvr_type = XMIDI_DRVR;
	memcpy(Description.data_suffix, "MT\0", 4);
	Description.dev_name_table = DeviceNames;
	Description.default_IO = 0x330;
	Description.default_IRQ = -1;
	Description.default_DMA = -1;
	Description.default_DRQ = -1;
	Description.service_rate = QUANT_RATE;
	Description.display_size = 20;
	return &Description;
}

uint16_t Mt32Detect(void)
{
	return plat_midi_available() ? 1 : 0;
}

void Mt32Init(void)
{
	SequenceCount = 0;
	StallUs = 0;
	InService = 0;
	MainWaiting = 0;
	Resuming = 0;
	memset(GlobalControls, -1, sizeof GlobalControls);
	memset(GlobalProgram, -1, sizeof GlobalProgram);
	memset(GlobalPitchLow, -1, sizeof GlobalPitchLow);
	memset(GlobalPitchHigh, -1, sizeof GlobalPitchHigh);
	memset(Sequences, 0, sizeof Sequences);
	memset(LockStatus, 0, sizeof LockStatus);
	memset(ActiveNotes, 0, sizeof ActiveNotes);
	ResetSynth();
	InitSynth();
	for (int16_t i = 0; i < NUM_CONTROLS; i++) {
		for (uint8_t chan = MIN_TRUE_CHAN - 1; chan <= MAX_REC_CHAN - 1; chan++) {
			GlobalControls[i][chan] = (int8_t) ControlDefaults[i];
			SendMidi(0xb0 | chan, LoggedControls[i], ControlDefaults[i]);
		}
	}
	Wait(10);
	for (uint8_t chan = MIN_TRUE_CHAN - 1; chan <= MAX_REC_CHAN - 1; chan++) {
		GlobalPitchLow[chan] = 0x00;
		GlobalPitchHigh[chan] = 0x40;
		SendMidi(0xe0 | chan, 0x00, 0x40);
		if (ProgramDefaults[chan - 1] >= 0) {
			GlobalProgram[chan] = (int8_t) ProgramDefaults[chan - 1];
			SendMidi(0xc0 | chan, (uint8_t) ProgramDefaults[chan - 1], 0);
		}
	}
	Wait(10);
	InitOK = 1;
}

static void StopSequence(Sequence *s)
{
	if (!s->registered || s->status != SEQ_PLAYING)
		return;
	FlushNoteQueue(s);
	ResetSequence(s);
	s->status = SEQ_STOPPED;
}

/* Stops everything, clears the MT-32 again and leaves a message on its display. */
void Mt32Shutdown(const char *signOff)
{
	if (!InitOK)
		return;
	for (int16_t n = 0; n < NSEQS; n++) {
		if (Sequences[n].registered) {
			StopSequence(&Sequences[n]);
			ReleaseSequence(n);
		}
	}
	ResetSynth();
	WriteDisplay(signOff);
	InitOK = 0;
}

/* ---- AIL calls ---- */

#define MT32_CALL(driver, failed) \
	do { if (AilDriverKind(driver) != AIL_KIND_MT32) return failed; } while (0)

uint16_t AIL_state_table_size(HDRIVER driver)
{
	MT32_CALL(driver, 0);
	return STATE_TABLE_SIZE;
}

/* The sequence's state stays here; the program's state table goes unused. */
HSEQUENCE AIL_register_sequence(HDRIVER driver, void *FORM_XMID, uint16_t sequence_num,
	void *state_table, void *controller_table)
{
	const uint8_t *form, *chunk;
	uint32_t chunkLength = 12;
	int16_t handle;
	Sequence *s;

	MT32_CALL(driver, -1);
	plat_sound_lock();
	for (handle = 0; handle < NSEQS && Sequences[handle].registered; handle++)
		;
	form = handle < NSEQS ? FindSequence((const uint8_t *) FORM_XMID, sequence_num) : 0;
	if (form == 0) {
		plat_sound_unlock();
		return -1;
	}
	s = &Sequences[handle];
	s->timb = 0;
	s->rbrn = 0;
	for (chunk = form; ; ) {
		chunk += chunkLength;
		chunkLength = ReadBigLong(chunk + 4) + 8;
		if (memcmp(chunk, "TIMB", 4) == 0)
			s->timb = chunk;
		else if (memcmp(chunk, "RBRN", 4) == 0)
			s->rbrn = chunk;
		else if (memcmp(chunk, "EVNT", 4) == 0)
			break;
	}
	s->evnt = chunk;
	s->controlTable = (uint8_t *) controller_table;
	s->postRelease = 0;
	s->started = 0;
	s->status = SEQ_STOPPED;
	s->registered = 1;
	SequenceCount++;
	Rewind(s);
	plat_sound_unlock();
	return handle;
}

/* A playing sequence is released when it ends. */
void AIL_release_sequence_handle(HDRIVER driver, HSEQUENCE sequence)
{
	MT32_CALL(driver, );
	if (!GetSequence(sequence))
		return;
	plat_sound_lock();
	ReleaseSequence(sequence);
	plat_sound_unlock();
}

/* The MT-32 keeps its timbres in its own memory. */
uint16_t AIL_default_timbre_cache_size(HDRIVER driver)
{
	MT32_CALL(driver, 0);
	return 0;
}

void AIL_define_timbre_cache(HDRIVER driver, void *cache_addr, uint16_t cache_size)
{
}

/* The next timbre the sequence needs and the cache lacks, as bank << 8 | patch; 0xffff if none.
 * Banks 0 (built in) and 127 (rhythm) are never asked for. */
uint16_t AIL_timbre_request(HDRIVER driver, HSEQUENCE sequence)
{
	Sequence *s = GetSequence(sequence);
	uint16_t request = 0xffff;
	const uint8_t *entry;

	MT32_CALL(driver, 0);
	if (!s)
		return 0xffff;
	plat_sound_lock();
	if (s->timb && memcmp(s->timb, "TIMB", 4) == 0) {
		uint16_t count = ReadWord(s->timb + 8);

		for (entry = s->timb + 10; count > 0; count--, entry += 2) {
			uint8_t patch = entry[0], bank = entry[1];

			if (bank != 0 && bank != 127 && IndexTimbre(bank, patch) == -1) {
				request = ReadWord(entry);
				break;
			}
		}
	}
	plat_sound_unlock();
	return request;
}

/* Nonzero when resident. */
uint16_t AIL_timbre_status(HDRIVER driver, int16_t bank, int16_t patch)
{
	uint16_t status;

	MT32_CALL(driver, 0);
	plat_sound_lock();
	if ((uint8_t) bank == 0 || (uint8_t) bank == 127)
		status = (uint16_t) (((uint8_t) bank << 8 | (uint8_t) patch) + 1);
	else
		status = (uint16_t) (IndexTimbre((uint8_t) bank, (uint8_t) patch) + 1);
	plat_sound_unlock();
	return status;
}

/* src_addr: a 16-bit size, then the MT-32 timbre's 246 bytes. */
void AIL_install_timbre(HDRIVER driver, int16_t bank, int16_t patch, void *src_addr)
{
	MT32_CALL(driver, );
	plat_sound_lock();
	InstallTimbre((uint8_t) bank, (uint8_t) patch, (const uint8_t *) src_addr);
	plat_sound_unlock();
}

/* Bank and patch -1 stand for every timbre. */
static void SetTimbreProtection(int16_t bank, int16_t patch, uint8_t on)
{
	int16_t first = 0, end = NUM_TIMBS;

	if ((uint8_t) bank != 0xff || (uint8_t) patch != 0xff) {
		first = IndexTimbre((uint8_t) bank, (uint8_t) patch);
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

void AIL_protect_timbre(HDRIVER driver, int16_t bank, int16_t patch)
{
	MT32_CALL(driver, );
	plat_sound_lock();
	SetTimbreProtection(bank, patch, 1);
	plat_sound_unlock();
}

void AIL_unprotect_timbre(HDRIVER driver, int16_t bank, int16_t patch)
{
	MT32_CALL(driver, );
	plat_sound_lock();
	SetTimbreProtection(bank, patch, 0);
	plat_sound_unlock();
}

void AIL_start_sequence(HDRIVER driver, HSEQUENCE sequence)
{
	Sequence *s = GetSequence(sequence);

	MT32_CALL(driver, );
	if (!s)
		return;
	plat_sound_lock();
	StopSequence(s);
	Rewind(s);
	s->next = s->evnt + 8;
	s->status = SEQ_PLAYING;
	s->started = 1;
	plat_sound_unlock();
}

void AIL_stop_sequence(HDRIVER driver, HSEQUENCE sequence)
{
	Sequence *s = GetSequence(sequence);

	MT32_CALL(driver, );
	if (!s)
		return;
	plat_sound_lock();
	StopSequence(s);
	plat_sound_unlock();
}

void AIL_resume_sequence(HDRIVER driver, HSEQUENCE sequence)
{
	Sequence *s = GetSequence(sequence);

	MT32_CALL(driver, );
	if (!s)
		return;
	plat_sound_lock();
	if (s->registered && s->status == SEQ_STOPPED && s->started) {
		RestoreSequence(s);
		s->status = SEQ_PLAYING;
	}
	plat_sound_unlock();
}

/* 0xffff for no sequence. */
uint16_t AIL_sequence_status(HDRIVER driver, HSEQUENCE sequence)
{
	Sequence *s = GetSequence(sequence);
	uint16_t status;

	MT32_CALL(driver, 0);
	if (!s)
		return 0xffff;
	plat_sound_lock();
	status = (uint16_t) s->status;
	plat_sound_unlock();
	return status;
}

uint16_t AIL_relative_volume(HDRIVER driver, HSEQUENCE sequence)
{
	Sequence *s = GetSequence(sequence);

	MT32_CALL(driver, 0);
	return s ? s->volPercent : 0xffff;
}

uint16_t AIL_relative_tempo(HDRIVER driver, HSEQUENCE sequence)
{
	Sequence *s = GetSequence(sequence);

	MT32_CALL(driver, 0);
	return s ? s->tempoPercent : 0xffff;
}

/* Over milliseconds, or at once for 0. */
void AIL_set_relative_volume(HDRIVER driver, HSEQUENCE sequence, uint16_t percent,
	uint16_t milliseconds)
{
	Sequence *s = GetSequence(sequence);

	MT32_CALL(driver, );
	if (!s)
		return;
	plat_sound_lock();
	s->volTarget = percent;
	if (milliseconds == 0) {
		s->volPercent = percent;
		SendVolumes(s);
	} else if (s->volTarget != s->volPercent) {
		s->volPeriod = RampPeriod(s->volPercent, s->volTarget, milliseconds);
		s->volAccum = 0;
	}
	plat_sound_unlock();
}

void AIL_set_relative_tempo(HDRIVER driver, HSEQUENCE sequence, uint16_t percent,
	uint16_t milliseconds)
{
	Sequence *s = GetSequence(sequence);

	MT32_CALL(driver, );
	if (!s)
		return;
	plat_sound_lock();
	s->tempoTarget = percent;
	if (milliseconds == 0)
		s->tempoPercent = percent;
	else if (s->tempoTarget != s->tempoPercent) {
		s->tempoPeriod = RampPeriod(s->tempoPercent, s->tempoTarget, milliseconds);
		s->tempoAccum = 0;
	}
	plat_sound_unlock();
}

/* The logged value of a controller on a 1-based channel, the last callback controller, or -1. */
int16_t AIL_controller_value(HDRIVER driver, HSEQUENCE sequence, uint16_t channel,
	uint16_t controller_num)
{
	Sequence *s = GetSequence(sequence);
	int16_t logged;

	MT32_CALL(driver, 0);
	if (!s)
		return -1;
	if (controller_num == CALLBACK_TRIG)
		return s->lastCallback;
	logged = controller_num < 256 ? ControlIndex((uint8_t) controller_num) : -1;
	if (logged == -1 || channel < 1 || channel > NUM_CHANS)
		return -1;
	return s->controls[logged][channel - 1];
}

void AIL_set_controller_value(HDRIVER driver, HSEQUENCE sequence, uint16_t channel,
	uint16_t controller_num, uint16_t value)
{
	Sequence *s = GetSequence(sequence);

	MT32_CALL(driver, );
	if (!s || channel < 1 || channel > NUM_CHANS)
		return;
	plat_sound_lock();
	XmidiControl(s, (uint8_t) (channel - 1), (uint8_t) controller_num, (uint8_t) value);
	plat_sound_unlock();
}

uint16_t AIL_channel_notes(HDRIVER driver, HSEQUENCE sequence, uint16_t channel)
{
	Sequence *s = GetSequence(sequence);
	uint16_t count = 0;

	MT32_CALL(driver, 0);
	if (!s)
		return 0xffff;
	plat_sound_lock();
	for (int16_t i = 0; i < MAX_NOTES; i++) {
		if (s->noteChan[i] == (int8_t) (channel - 1))
			count++;
	}
	plat_sound_unlock();
	return count;
}

uint16_t AIL_beat_count(HDRIVER driver, HSEQUENCE sequence)
{
	Sequence *s = GetSequence(sequence);
	uint16_t beat;
	int16_t bar;

	MT32_CALL(driver, 0);
	if (!s)
		return 0xffff;
	plat_sound_lock();
	AdvancedCount(s, &beat, &bar);
	plat_sound_unlock();
	return beat;
}

uint16_t AIL_measure_count(HDRIVER driver, HSEQUENCE sequence)
{
	Sequence *s = GetSequence(sequence);
	uint16_t beat;
	int16_t bar;

	MT32_CALL(driver, 0);
	if (!s)
		return 0xffff;
	plat_sound_lock();
	AdvancedCount(s, &beat, &bar);
	plat_sound_unlock();
	return (uint16_t) bar;
}

/* Jumps to a marker in the RBRN chunk, ending any loops. */
void AIL_branch_index(HDRIVER driver, HSEQUENCE sequence, uint16_t marker_number)
{
	Sequence *s = GetSequence(sequence);

	MT32_CALL(driver, );
	if (!s)
		return;
	plat_sound_lock();
	if (s->rbrn && memcmp(s->rbrn, "RBRN", 4) == 0) {
		const uint8_t *entry = s->rbrn + 10;

		for (uint16_t count = ReadWord(s->rbrn + 8); count > 0; count--, entry += 6) {
			if (entry[0] == (uint8_t) marker_number) {
				s->next = s->evnt + 8 + (ReadWord(entry + 2) | (uint32_t) ReadWord(entry + 4) << 16);
				s->intervalCount = 0;
				FlushNoteQueue(s);
				for (int16_t i = 0; i < FOR_NEST; i++)
					s->forCount[i] = -1;
				break;
			}
		}
	}
	plat_sound_unlock();
}

void AIL_send_channel_voice_message(HDRIVER driver, uint16_t status, uint16_t data_1,
	uint16_t data_2)
{
	MT32_CALL(driver, );
	plat_sound_lock();
	SendMidi((uint8_t) status, (uint8_t) data_1, (uint8_t) data_2);
	plat_sound_unlock();
}

void AIL_send_sysex_message(HDRIVER driver, uint16_t addr_a, uint16_t addr_b, uint16_t addr_c,
	void *data, uint16_t size, uint16_t delay)
{
	MT32_CALL(driver, );
	plat_sound_lock();
	SendSysex((uint8_t) addr_a, (uint8_t) addr_b, (uint8_t) addr_c, (const uint8_t *) data,
		size < SYSEX_SIZE ? size : SYSEX_SIZE, delay);
	plat_sound_unlock();
}

void AIL_write_display(HDRIVER driver, const char *string)
{
	MT32_CALL(driver, );
	plat_sound_lock();
	WriteDisplay(string);
	plat_sound_unlock();
}

uint16_t AIL_lock_channel(HDRIVER driver)
{
	uint16_t channel;

	MT32_CALL(driver, 0);
	plat_sound_lock();
	channel = LockChannel();
	plat_sound_unlock();
	return channel;
}

/* Channels are 1-based. */
void AIL_map_sequence_channel(HDRIVER driver, HSEQUENCE sequence, uint16_t sequence_channel,
	uint16_t physical_channel)
{
	Sequence *s = GetSequence(sequence);

	MT32_CALL(driver, );
	if (!s || sequence_channel < 1 || sequence_channel > NUM_CHANS)
		return;
	plat_sound_lock();
	s->chanMap[sequence_channel - 1] = (uint8_t) (physical_channel - 1);
	plat_sound_unlock();
}

uint16_t AIL_true_sequence_channel(HDRIVER driver, HSEQUENCE sequence,
	uint16_t sequence_channel)
{
	Sequence *s = GetSequence(sequence);

	MT32_CALL(driver, 0);
	if (!s)
		return 0xffff;
	if (sequence_channel < 1 || sequence_channel > NUM_CHANS)
		return 0;
	return (uint16_t) (s->chanMap[sequence_channel - 1] + 1);
}

void AIL_release_channel(HDRIVER driver, uint16_t channel)
{
	MT32_CALL(driver, );
	plat_sound_lock();
	ReleaseChannel(channel);
	plat_sound_unlock();
}

extern "C" void ResetAilmt32Globals(void)
{
	Mt32ShortWaits = 0;
	memset(&Description, 0, sizeof Description);
	InitOK = 0;
	InService = 0;
	MainWaiting = 0;
	StallUs = 0;
	Resuming = 0;
	ResumeSequence = 0;
	memset(Sequences, 0, sizeof Sequences);
	SequenceCount = 0;
	memset(GlobalControls, 0, sizeof GlobalControls);
	memset(GlobalProgram, 0, sizeof GlobalProgram);
	memset(GlobalPitchLow, 0, sizeof GlobalPitchLow);
	memset(GlobalPitchHigh, 0, sizeof GlobalPitchHigh);
	memset(ActiveNotes, 0, sizeof ActiveNotes);
	memset(LockStatus, 0, sizeof LockStatus);
	memset(PatchBank, 0, sizeof PatchBank);
	memset(SysexQueue, 0, sizeof SysexQueue);
	memset(AddrM, 0, sizeof AddrM);
	memset(AddrK, 0, sizeof AddrK);
	memset(AddrL, 0, sizeof AddrL);
	memset(QueuePtr, 0, sizeof QueuePtr);
	NoteEvent = 0;
	memset(TimbHist, 0, sizeof TimbHist);
	memset(TimbBank, 0, sizeof TimbBank);
	memset(TimbNum, 0, sizeof TimbNum);
	memset(TimbAttribs, 0, sizeof TimbAttribs);
	memset(ChanTimbs, 0, sizeof ChanTimbs);
	memset(MidiBank, 0, sizeof MidiBank);
	memset(MidiProgram, 0, sizeof MidiProgram);
}
