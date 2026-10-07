/* Black Gate ENDGAME.EXE: the Miles AIL 2.0 XMIDI driver for the Roland MT-32 (MT32MPU.ADV), which
 * the ending loaded through AIL, rewritten natively. It sends to the platform's MT-32.
 *
 *   main thread: open, register (timbre uploads), start, stop, status, close
 *   sound tick (60 Hz): Serve twice, the driver's 120 Hz service
 *
 * An XMIDI event stream is MIDI without running status or note-offs: a byte below 0x80 waits
 * that many 120 Hz intervals, and a note-on carries its length as a variable-length number.
 * Controllers 32-46 and 58-63 write MT-32 memory; 110-120 steer the player and never reach the
 * synth. Channel locking (110, 111), indirect values (115), callbacks (119), beat counts and
 * tempo and volume ramps are not implemented; the ending uses none of them.
 */

#include "u7port.h"
#include "plat.h"
#include "ailmt32.h"

/* No waits after MT-32 memory writes (a launcher switch, not in the original): Munt takes them
 * at once. */
extern "C" uint8_t Mt32ShortWaits = 0;

namespace Endgame {

#define NUM_CHANS       16
#define MIN_CHAN        1       /* the MT-32's parts listen on MIDI channels 2-10 */
#define MAX_CHAN        9
#define MAX_NOTES       32
#define FOR_NEST        4
#define NSEQS           8
#define NUM_TIMBS       64
#define SYSEX_SIZE      32
#define SYSEX_Q_CNT     3
#define DEF_SYNTH_VOL   90      /* keeps the MT-32 from distorting */

/* One vertical retrace, the unit the driver waited in after writing MT-32 memory. */
#define RETRACE_US      14286
#define SERVICE_US      8333

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

/* Timbre cache slot attributes. */
#define TIMB_IN_USE     0x80
#define TIMB_PROTECTED  0x40

struct Sequence {
	uint8_t registered;
	int16_t status;
	uint8_t interrupted;        /* a wait broke off its events; the rest follow the wait */
	const uint8_t *timb;        /* the TIMB chunk, or 0 */
	const uint8_t *evnt;        /* the EVNT chunk */
	const uint8_t *next;
	int16_t intervalCount;
	int16_t noteCount;
	int16_t volPercent;
	int16_t forCount[FOR_NEST];
	const uint8_t *forStart[FOR_NEST];
	int8_t sustain[NUM_CHANS];
	int8_t noteChan[MAX_NOTES];
	uint8_t noteNum[MAX_NOTES];
	int32_t noteTime[MAX_NOTES];
};

static uint8_t Opened;
static uint8_t InService;
static uint8_t MainWaiting;
static int32_t StallUs;
static Sequence Sequences[NSEQS];

/* The MT-32 side: timbre cache, patch banks, and the controller SysEx queues. */
static uint8_t PatchBank[128];
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
static int16_t MidiProgram[NUM_CHANS];

/* ---- Output ---- */

/* The driver busy-waited for retraces with interrupts off, so neither the program nor the music
 * moved meanwhile. In the service only the music stops. */
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

/* A Roland data set to address a, b, c, then waits. */
static void SendSysex(uint8_t a, uint8_t b, uint8_t c, const uint8_t *data, int16_t size, uint16_t wait)
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
	for (int16_t i = 0; i < size; i++) {
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
	uint16_t low = *l + addend, mid = *k, high = *m;

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

static void WritePatch(uint8_t patch, uint8_t index, const uint8_t *value, int16_t size)
{
	uint8_t m = 5, k = 0, l = 0;

	AddAddress((uint16_t) (patch * 8 + index), &m, &k, &l);
	SendSysex(m, k, l, value, size, 2);
}

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

/* A controller from the sequence, as the MT-32 side takes it. */
static void Controller(uint8_t chan, uint8_t con, uint8_t value)
{
	uint8_t message[3];

	if (con >= SYSEX_FIRST && con <= SYSEX_LAST) {
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
		/* A final byte, or a full queue: send it. The final byte's address stays the last one
		 * written, so later final bytes rewrite it. */
		length = QueuePtr[q] + 1;
		SendSysex(AddrM[q], AddrK[q], AddrL[q], SysexQueue[q], length, 0);
		if (op == 3)
			Wait(4);
		else
			length--;
		AddAddress(length, &AddrM[q], &AddrK[q], &AddrL[q]);
		QueuePtr[q] = 0;
		return;
	}
	switch (con) {
	case PATCH_REVERB:
	case PATCH_BENDER:
		if (MidiProgram[chan] >= 0) {
			WritePatch((uint8_t) MidiProgram[chan], con == PATCH_REVERB ? 6 : 4, &value, 1);
			message[0] = 0xc0 | chan;
			message[1] = (uint8_t) MidiProgram[chan];
			SendBytes(message, 2);
		}
		return;
	case REVERB_MODE:
		WriteSystem(1, value);
		return;
	case REVERB_TIME:
		WriteSystem(2, value);
		return;
	case REVERB_LEVEL:
		WriteSystem(3, value);
		return;
	case PATCH_BANK_SEL:
		MidiBank[chan] = value;
		return;
	case RHYTHM_KEY_TIMB:
		if (ChanTimbs[chan] >= 0)
			WriteRhythmSetup(value, (uint8_t) ChanTimbs[chan]);
		return;
	case TIMBRE_PROTECT:
		if (ChanTimbs[chan] >= 0) {
			if (value >= 64)
				TimbAttribs[ChanTimbs[chan]] |= TIMB_PROTECTED;
			else
				TimbAttribs[ChanTimbs[chan]] &= ~TIMB_PROTECTED;
		}
		return;
	}
	if (con >= CHAN_LOCK && con <= SEQ_INDEX)
		return;
	message[0] = 0xb0 | chan;
	message[1] = con;
	message[2] = value;
	SendBytes(message, 3);
}

/* A channel message on its way to the synth. Notes count toward the timbre cache's
 * least-recently-used order; a program change first points the patch at the channel's bank. */
static void SendMessage(uint8_t status, uint8_t d1, uint8_t d2)
{
	uint8_t chan = status & 0x0f;
	uint8_t message[3];

	switch (status & 0xf0) {
	case 0xb0:
		Controller(chan, d1, d2);
		return;
	case 0xc0:
		MidiProgram[chan] = d1;
		if (MidiBank[chan] != PatchBank[d1])
			SetupPatch(d1, MidiBank[chan]);
		ChanTimbs[chan] = (int8_t) IndexTimbre(MidiBank[chan], d1);
		break;
	case 0x90:
		NoteEvent++;
		if (ChanTimbs[chan] >= 0)
			TimbHist[ChanTimbs[chan]] = NoteEvent;
		break;
	}
	message[0] = status;
	message[1] = d1;
	message[2] = d2;
	SendBytes(message, (status & 0xf0) == 0xc0 || (status & 0xf0) == 0xd0 ? 2 : 3);
}

/* ---- Sequences ---- */

static uint32_t ReadBigLong(const uint8_t *p)
{
	return (uint32_t) p[0] << 24 | (uint32_t) p[1] << 16 | (uint32_t) p[2] << 8 | p[3];
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

/* The first chunk of FORM XMID number n, in a lone FORM XMID or inside a CAT; 0 if absent. */
static const uint8_t *FindSequence(const uint8_t *xmidi, int32_t size, int16_t n)
{
	const uint8_t *p = xmidi, *end = xmidi + size;

	while (p + 12 <= end && memcmp(p + 8, "XMID", 4) != 0) {
		if (memcmp(p, "FORM", 4) != 0 && memcmp(p, "CAT ", 4) != 0)
			return 0;
		p += 8 + ReadBigLong(p + 4);
	}
	if (p + 12 > end)
		return 0;
	if (memcmp(p, "FORM", 4) == 0)
		return n == 0 ? p : 0;
	if (memcmp(p, "CAT ", 4) != 0)
		return 0;
	end = p + 8 + ReadBigLong(p + 4);
	for (p += 12; p + 12 <= end; p += 8 + ReadBigLong(p + 4)) {
		if (memcmp(p + 8, "XMID", 4) == 0 && n-- == 0)
			return p;
	}
	return 0;
}

static void Rewind(Sequence *s)
{
	for (int16_t i = 0; i < FOR_NEST; i++)
		s->forCount[i] = -1;
	for (int16_t i = 0; i < NUM_CHANS; i++)
		s->sustain[i] = -1;
	for (int16_t i = 0; i < MAX_NOTES; i++)
		s->noteChan[i] = -1;
	s->intervalCount = 0;
	s->interrupted = 0;
	s->noteCount = 0;
	s->volPercent = DEF_SYNTH_VOL;
}

static void NoteOff(Sequence *s, int16_t slot)
{
	uint8_t chan = (uint8_t) s->noteChan[slot];

	s->noteChan[slot] = -1;
	SendMessage(0x80 | chan, s->noteNum[slot], 0);
}

static void FlushNotes(Sequence *s)
{
	for (int16_t i = 0; i < MAX_NOTES; i++) {
		if (s->noteChan[i] != -1)
			NoteOff(s, i);
	}
	s->noteCount = 0;
}

/* Lets go of what the sequence held: its sustain pedals. */
static void ResetSequence(Sequence *s)
{
	for (uint8_t chan = 0; chan < NUM_CHANS; chan++) {
		if (s->sustain[chan] >= 64)
			SendMessage(0xb0 | chan, SUSTAIN, 0);
	}
}

/* A note with its length; a full queue loses its first note's note-off. */
static int16_t NoteOn(Sequence *s, const uint8_t *event)
{
	const uint8_t *p = event + 3;
	uint32_t duration = ReadVarLength(&p);
	int16_t slot;

	for (slot = 0; slot < MAX_NOTES && s->noteChan[slot] != -1; slot++)
		;
	if (slot == MAX_NOTES)
		slot = 0;
	else
		s->noteCount++;
	s->noteChan[slot] = event[0] & 0x0f;
	s->noteNum[slot] = event[1];
	s->noteTime[slot] = (int32_t) duration - 1;
	SendMessage(0x90 | (event[0] & 0x0f), event[1], event[2]);
	return (int16_t) (p - event);
}

/* A controller the player itself handles, or passes on. */
static void SequenceController(Sequence *s, uint8_t chan, uint8_t con, uint8_t value)
{
	int16_t i;

	switch (con) {
	case SUSTAIN:
		s->sustain[chan] = (int8_t) value;
		break;
	case PART_VOLUME:
		if (s->volPercent != 100) {
			uint16_t scaled = value * s->volPercent / 100;
			value = scaled < 127 ? (uint8_t) scaled : 127;
		}
		break;
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
		if (value < 64)
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
	case CHAN_LOCK:
	case CHAN_PROTECT:
	case INDIRECT_C_PFX:
	case CLEAR_BEAT_BAR:
	case CALLBACK_TRIG:
		return;
	}
	SendMessage(0xb0 | chan, con, value);
}

/* Plays one sequence's events up to its next interval. */
static void PlayEvents(Sequence *s)
{
	while (s->status == SEQ_PLAYING) {
		const uint8_t *event = s->next;
		uint8_t status = event[0] & 0xf0, chan = event[0] & 0x0f;
		const uint8_t *p;
		int16_t size;

		if (StallUs > 0) {
			s->interrupted = 1;
			return;
		}
		if (event[0] < 0x80) {
			s->intervalCount = event[0];
			s->next++;
			return;
		}
		switch (status) {
		case 0xf0:
			if (chan == 0x0f) {
				p = event + 2;
				size = (int16_t) ReadVarLength(&p);
				size += (int16_t) (p - event);
				if (event[1] == 0x2f) {
					ResetSequence(s);
					s->status = SEQ_DONE;
				}
			} else {
				p = event + 1;
				size = (int16_t) ReadVarLength(&p);
				/* F0 goes out with its data; F7 sends the data alone */
				if (event[0] == 0xf0)
					SendBytes(event, 1);
				SendBytes(p, size);
				size += (int16_t) (p - event);
			}
			break;
		case 0xb0:
			s->next += 3;
			SequenceController(s, chan, event[1], event[2]);
			continue;
		case 0xc0:
		case 0xd0:
			SendMessage(event[0], event[1], 0);
			size = 2;
			break;
		case 0xa0:
		case 0xe0:
			SendMessage(event[0], event[1], event[2]);
			size = 3;
			break;
		default:
			size = NoteOn(s, event);
			break;
		}
		s->next += size;
	}
}

/* The 120 Hz service: expired notes off, then any events that are due. A wait holds everything
 * back until it has passed. */
static void Serve(void)
{
	if (MainWaiting)
		return;
	if (StallUs > 0) {
		StallUs -= SERVICE_US;
		return;
	}
	InService = 1;
	for (int16_t n = 0; n < NSEQS && StallUs <= 0; n++) {
		Sequence *s = &Sequences[n];

		if (!s->registered || s->status != SEQ_PLAYING)
			continue;
		if (s->interrupted) {
			s->interrupted = 0;
			PlayEvents(s);
			continue;
		}
		if (s->noteCount) {
			for (int16_t i = 0; i < MAX_NOTES && s->noteCount; i++) {
				if (s->noteChan[i] != -1 && --s->noteTime[i] < 0) {
					NoteOff(s, i);
					s->noteCount--;
				}
			}
		}
		if (--s->intervalCount <= 0)
			PlayEvents(s);
	}
	InService = 0;
}

static void SoundTick(void)
{
	Serve();
	Serve();
}

/* ---- Driver ---- */

static const uint8_t PartChannels[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
static const uint8_t PartReserve[9] = { 3, 4, 3, 4, 3, 4, 3, 4, 4 };
static const uint8_t InitReverb[3] = { 0, 3, 2 };
static const uint8_t LoggedControls[9] = { PART_VOLUME, MODULATION, PANPOT, EXPRESSION, SUSTAIN,
	PATCH_BANK_SEL, CHAN_LOCK, CHAN_PROTECT, VOICE_PROTECT };
static const uint8_t ControlDefaults[9] = { 127, 0, 64, 127, 0, 0, 0, 0, 0 };
static const int16_t ProgramDefaults[9] = { 68, 48, 95, 78, 41, 3, 110, 122, -1 };

/* Clears the MT-32 and puts its parts on channels 2-10. */
static void ResetSynth(void)
{
	SendSysex(0x7f, 0, 0, PartChannels, 1, 12);
	SendSysex(0x10, 0, 0x0d, PartChannels, 9, 4);
}

int16_t Mt32Open(void)
{
	uint8_t chan;

	if (Opened)
		return 1;
	if (!plat_midi_available())
		return 0;
	plat_sound_lock();
	memset(Sequences, 0, sizeof Sequences);
	StallUs = 0;
	InService = 0;
	MainWaiting = 0;
	ResetSynth();
	SendSysex(0x10, 0, 0x04, PartReserve, 9, 4);
	SendSysex(0x10, 0, 0x01, InitReverb, 3, 4);
	memset(QueuePtr, 0, sizeof QueuePtr);
	NoteEvent = 0;
	memset(TimbAttribs, 0, sizeof TimbAttribs);
	for (chan = 0; chan < NUM_CHANS; chan++) {
		ChanTimbs[chan] = -1;
		MidiProgram[chan] = -1;
		MidiBank[chan] = 0;
	}
	memset(PatchBank, 0, sizeof PatchBank);
	for (int16_t i = 0; i < 9; i++) {
		for (chan = MIN_CHAN; chan <= MAX_CHAN; chan++)
			SendMessage(0xb0 | chan, LoggedControls[i], ControlDefaults[i]);
	}
	Wait(10);
	for (chan = MIN_CHAN; chan <= MAX_CHAN; chan++) {
		SendMessage(0xe0 | chan, 0x00, 0x40);
		if (ProgramDefaults[chan - MIN_CHAN] >= 0)
			SendMessage(0xc0 | chan, (uint8_t) ProgramDefaults[chan - MIN_CHAN], 0);
	}
	Wait(10);
	Opened = 1;
	plat_sound_tick_set(SoundTick);
	plat_sound_unlock();
	return 1;
}

/* Stops everything, clears the MT-32 again and leaves a message on its display. */
void Mt32Close(const char *signOff)
{
	char text[20];

	if (!Opened)
		return;
	plat_sound_lock();
	plat_sound_tick_set(0);
	for (int16_t n = 0; n < NSEQS; n++) {
		if (Sequences[n].registered && Sequences[n].status == SEQ_PLAYING) {
			FlushNotes(&Sequences[n]);
			ResetSequence(&Sequences[n]);
		}
		Sequences[n].registered = 0;
	}
	ResetSynth();
	if (signOff) {
		memset(text, ' ', sizeof text);
		for (int16_t i = 0; i < 20 && signOff[i]; i++)
			text[i] = signOff[i];
		SendSysex(0x20, 0, 0, (const uint8_t *) text, 20, 4);
	}
	Opened = 0;
	plat_sound_unlock();
}

int16_t Mt32RegisterSequence(const uint8_t *xmidi, int32_t size, int16_t number)
{
	const uint8_t *form = FindSequence(xmidi, size, number);
	const uint8_t *chunk, *end;
	int16_t handle;
	Sequence *s;

	if (!Opened || form == 0)
		return -1;
	plat_sound_lock();
	for (handle = 0; handle < NSEQS && Sequences[handle].registered; handle++)
		;
	if (handle == NSEQS) {
		plat_sound_unlock();
		return -1;
	}
	s = &Sequences[handle];
	s->timb = 0;
	s->evnt = 0;
	end = form + 8 + ReadBigLong(form + 4);
	for (chunk = form + 12; chunk + 8 <= end; chunk += 8 + ReadBigLong(chunk + 4)) {
		if (memcmp(chunk, "TIMB", 4) == 0)
			s->timb = chunk;
		else if (memcmp(chunk, "EVNT", 4) == 0) {
			s->evnt = chunk;
			break;
		}
	}
	if (s->evnt == 0) {
		plat_sound_unlock();
		return -1;
	}
	s->registered = 1;
	s->status = SEQ_STOPPED;
	Rewind(s);
	plat_sound_unlock();
	return handle;
}

void Mt32StartSequence(int16_t handle)
{
	Sequence *s;

	if (handle < 0 || handle >= NSEQS || !Sequences[handle].registered)
		return;
	plat_sound_lock();
	s = &Sequences[handle];
	if (s->status == SEQ_PLAYING) {
		FlushNotes(s);
		ResetSequence(s);
	}
	Rewind(s);
	s->next = s->evnt + 8;
	s->status = SEQ_PLAYING;
	plat_sound_unlock();
}

void Mt32StopSequence(int16_t handle)
{
	Sequence *s;

	if (handle < 0 || handle >= NSEQS || !Sequences[handle].registered)
		return;
	plat_sound_lock();
	s = &Sequences[handle];
	if (s->status == SEQ_PLAYING) {
		FlushNotes(s);
		ResetSequence(s);
		s->status = SEQ_STOPPED;
		s->interrupted = 0;
	}
	plat_sound_unlock();
}

int16_t Mt32SequenceStatus(int16_t handle)
{
	int16_t status;

	if (handle < 0 || handle >= NSEQS)
		return -1;
	plat_sound_lock();
	status = Sequences[handle].status;
	plat_sound_unlock();
	return status;
}

/* Banks 0 (built in) and 127 (rhythm) are never asked for. */
int16_t Mt32TimbreRequest(int16_t handle)
{
	const uint8_t *entry;
	int16_t count, request = -1;

	if (handle < 0 || handle >= NSEQS || !Sequences[handle].registered || !Sequences[handle].timb)
		return -1;
	plat_sound_lock();
	entry = Sequences[handle].timb + 8;
	count = (int16_t) (entry[0] | entry[1] << 8);
	for (entry += 2; count > 0; count--, entry += 2) {
		uint8_t patch = entry[0], bank = entry[1];

		if (bank != 0 && bank != 127 && IndexTimbre(bank, patch) < 0) {
			request = (int16_t) (bank << 8 | patch);
			break;
		}
	}
	plat_sound_unlock();
	return request;
}

int16_t Mt32TimbreStatus(uint8_t bank, uint8_t patch)
{
	int16_t slot;

	if (bank == 0 || bank == 127)
		return 1;
	plat_sound_lock();
	slot = IndexTimbre(bank, patch);
	plat_sound_unlock();
	return slot + 1;
}

/* Takes a free cache slot, or the least recently played unprotected one, and uploads the timbre
 * there in five pieces: common part and four partials. */
void Mt32InstallTimbre(uint8_t bank, uint8_t patch, const uint8_t *timbre)
{
	int16_t slot;
	uint8_t high;

	if (bank == 127)
		return;
	plat_sound_lock();
	if (bank != 0 && IndexTimbre(bank, patch) < 0) {
		if (timbre == 0) {
			plat_sound_unlock();
			return;
		}
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
			if (slot < 0) {
				plat_sound_unlock();
				return;
			}
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
	plat_sound_unlock();
}

}

extern "C" void ResetEndgameAilmt32Globals(void)
{
	Mt32ShortWaits = 0;
	Endgame::Opened = 0;
	Endgame::InService = 0;
	Endgame::MainWaiting = 0;
	Endgame::StallUs = 0;
	memset(Endgame::Sequences, 0, sizeof Endgame::Sequences);
	memset(Endgame::PatchBank, 0, sizeof Endgame::PatchBank);
	memset(Endgame::SysexQueue, 0, sizeof Endgame::SysexQueue);
	memset(Endgame::AddrM, 0, sizeof Endgame::AddrM);
	memset(Endgame::AddrK, 0, sizeof Endgame::AddrK);
	memset(Endgame::AddrL, 0, sizeof Endgame::AddrL);
	memset(Endgame::QueuePtr, 0, sizeof Endgame::QueuePtr);
	Endgame::NoteEvent = 0;
	memset(Endgame::TimbHist, 0, sizeof Endgame::TimbHist);
	memset(Endgame::TimbBank, 0, sizeof Endgame::TimbBank);
	memset(Endgame::TimbNum, 0, sizeof Endgame::TimbNum);
	memset(Endgame::TimbAttribs, 0, sizeof Endgame::TimbAttribs);
	memset(Endgame::ChanTimbs, 0, sizeof Endgame::ChanTimbs);
	memset(Endgame::MidiBank, 0, sizeof Endgame::MidiBank);
	memset(Endgame::MidiProgram, 0, sizeof Endgame::MidiProgram);
}
