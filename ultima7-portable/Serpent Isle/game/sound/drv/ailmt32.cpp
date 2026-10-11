/* The synthesizer half of the Miles AIL 2.14 XMIDI driver for the Roland MT-32 (MT32MPU.ADV,
 * MT32.INC), rewritten natively: 64 memory timbres as a cache, patch banks and the controller
 * SysEx queues, sent to the platform's MT-32.
 */

#include "u7port.h"
#include "plat.h"
#include "ailxmidi.h"

#define NUM_CHANS       16
#define NUM_TIMBS       64
#define SYSEX_SIZE      32
#define SYSEX_Q_CNT     3
#define DEF_SYNTH_VOL   90      /* keeps the MT-32 from distorting */
#define QUANT_RATE      120

/* Controllers the MT-32 side acts on. */
#define SYSEX_FIRST     32      /* three queues of start MSB, KSB, LSB, data byte, final byte */
#define SYSEX_LAST      46
#define RHYTHM_KEY_TIMB 58
#define PATCH_REVERB    59
#define PATCH_BENDER    60
#define REVERB_MODE     61
#define REVERB_TIME     62
#define REVERB_LEVEL    63
#define CHAN_LOCK       110
#define TIMBRE_PROTECT  113
#define PATCH_BANK_SEL  114
#define SEQ_INDEX       120

#define TIMB_IN_USE     0x80
#define TIMB_PROTECTED  0x40

static const uint8_t InitReverb[3] = { 0, 3, 2 };
static const uint8_t PartChannels[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
static const uint8_t PartReserve[9] = { 3, 4, 3, 4, 3, 4, 3, 4, 4 };
static const char DeviceNames[] = "Roland MT-32 or compatible with Roland MPU MIDI Interface\0"
	"Roland LAPC-1\0";

static drvr_desc Description;

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
	XmidiWait(wait);
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
		XmidiWait(4);
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

/* An XMIDI SysEx event: F0 goes out with its data; F7 sends the data alone. */
static void SysexEvent(uint8_t type, const uint8_t *data, uint16_t length)
{
	if (type == 0xf0)
		SendBytes(&type, 1);
	SendBytes(data, (int16_t) length);
}

static void Shutdown(const char *signOff)
{
	WriteDisplay(signOff);
}

static uint16_t ReadWord(const uint8_t *p)
{
	return (uint16_t) (p[0] | p[1] << 8);
}

/* Banks 0 (built in) and 127 (rhythm) are never asked for. */
static uint16_t TimbreRequest(const uint8_t *timb)
{
	if (timb && memcmp(timb, "TIMB", 4) == 0) {
		const uint8_t *entry = timb + 10;

		for (uint16_t count = ReadWord(timb + 8); count > 0; count--, entry += 2) {
			uint8_t patch = entry[0], bank = entry[1];

			if (bank != 0 && bank != 127 && IndexTimbre(bank, patch) == -1)
				return ReadWord(entry);
		}
	}
	return 0xffff;
}

static uint16_t TimbreStatus(uint8_t bank, uint8_t patch)
{
	if (bank == 0 || bank == 127)
		return (uint16_t) ((bank << 8 | patch) + 1);
	return (uint16_t) (IndexTimbre(bank, patch) + 1);
}

/* Bank and patch 0xff stand for every timbre. */
static void ProtectTimbre(uint8_t bank, uint8_t patch, uint8_t on)
{
	int16_t first = 0, end = NUM_TIMBS;

	if (bank != 0xff || patch != 0xff) {
		first = IndexTimbre(bank, patch);
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

/* The MT-32 keeps its timbres in its own memory. */
static void DefineCache(uint8_t *base, uint16_t size)
{
}

static void SysexMessage(uint8_t a, uint8_t b, uint8_t c, const uint8_t *data, uint16_t size,
	uint16_t wait)
{
	SendSysex(a, b, c, data, size < SYSEX_SIZE ? size : SYSEX_SIZE, wait);
}

const XmidiSynth Mt32Synth = {
	AIL_KIND_MT32, DEF_SYNTH_VOL, 1, SendMidi, SysexEvent, 0, ResetSynth, InitSynth, Shutdown,
	TimbreRequest, TimbreStatus, InstallTimbre, ProtectTimbre, 0, DefineCache, SysexMessage,
	WriteDisplay
};

extern "C" void ResetAilmt32Globals(void)
{
	memset(&Description, 0, sizeof Description);
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
