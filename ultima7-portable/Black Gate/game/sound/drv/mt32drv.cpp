/* The Roland MT-32 music driver, native.
 *
 * Channels arrive counted from 1 and go out as MIDI channel minus one. A note stops as a note-on
 * with velocity 0. Memory writes are Roland data-set messages, 256 bytes at most each.
 */

#include "u7port.h"
#include "plat.h"
#include "midiplay.h"
#include "mt32drv.h"

#define SYSEX_CHUNK     256
#define TIMBRE_SIZE     246

static SoundDriver Mt32Info = { 0, 0 };

/* The patch written for each bank timbre; byte 1 is the memory timbre it plays. */
static const uint8_t BankPatchStart[8] = { 2, 0, 24, 50, 24, 0, 1, 0 };
static uint8_t BankPatch[8];
static const char Banner[] = "Origin Sound System!";
static const char Title[] = "<<<  MQ Driver  >>> ";
static const uint8_t ResetValue = 0;

/* The game thread and the sound tick both send; the lock keeps each message whole. */
static void SendMidi(const uint8_t *message, int16_t length)
{
	plat_sound_lock();
	plat_midi_send(message, length);
	plat_sound_unlock();
}

static void SendChannel(uint8_t status, int16_t channel, int16_t first, int16_t second, int16_t length)
{
	uint8_t message[3];

	message[0] = status + ((channel - 1) & 0xf);
	message[1] = first & 0x7f;
	message[2] = second & 0x7f;
	SendMidi(message, length);
}

SoundDriver *Mt32Describe(void)
{
	return &Mt32Info;
}

/* Timbre data to memory slot, then a patch pointing program at it. Slot and program count from 1. */
static void LoadBankTimbre(uint8_t *timbre, int16_t slot, int16_t program)
{
	int32_t address;

	slot = (slot - 1) & 0x3f;
	program = (program - 1) & 0x7f;
	BankPatch[1] = slot;
	Mt32Sysex(INT32_C(0x80000) + (slot << 9), TIMBRE_SIZE, timbre);
	address = INT32_C(0x50000) + ((program >> 4) << 8) + ((program & 0xf) << 3);
	Mt32Sysex(address, 8, BankPatch);
}

/* Resets the synth and loads the timbre bank: a count, then per timbre its data and its program. */
int16_t Mt32Init(void *timbres)
{
	uint8_t *p;
	int16_t count, slot, i;

	if (!plat_midi_available())
		return 0;
	Mt32Sysex(INT32_C(0x7f007f), 1, (void *)&ResetValue);
	Mt32Sysex(INT32_C(0x200000), 20, (void *)Banner);
	if (timbres != 0) {
		p = (uint8_t *)timbres;
		count = *p++;
		slot = 1;
		for (i = 0; i < count; i++) {
			LoadBankTimbre(p, slot++, p[TIMBRE_SIZE]);
			p += TIMBRE_SIZE + 1;
		}
		/* the effects' timbres go after the bank, one slot further on */
		Mt32Info.first = slot;
	}
	Mt32Sysex(INT32_C(0x200000), 20, (void *)Title);
	return 1;
}

void Mt32Shutdown(void)
{
	int16_t i;

	for (i = 2; i <= 10; i++)
		Mt32Control(i, 0x7b, 0);
}

/* Nothing to do each tick. */
void Mt32Timer(void)
{
}

void Mt32Note(int16_t channel, int16_t note, int16_t velocity)
{
	SendChannel(0x90, channel, note, velocity, 3);
}

void Mt32Control(int16_t channel, int16_t control, int16_t value)
{
	SendChannel(0xb0, channel, control, value, 3);
}

void Mt32Pitch(int16_t channel, int16_t low, int16_t high)
{
	SendChannel(0xe0, channel, low, high, 3);
}

void Mt32Program(int16_t channel, int16_t program)
{
	SendChannel(0xc0, channel, program, 0, 2);
}

/* The synth needs no port. */
void Mt32Port(int16_t)
{
}

/* Writes length bytes at address, whose three bytes each hold 7 bits of the synth's address. */
void Mt32Sysex(int32_t address, int16_t length, void *data)
{
	uint8_t message[SYSEX_CHUNK + 10];
	uint8_t *p = (uint8_t *)data;
	uint16_t left = length, count, sum, i;
	uint32_t flat;
	int16_t n;

	while (left != 0) {
		count = left > SYSEX_CHUNK ? SYSEX_CHUNK : left;
		left -= count;
		n = 0;
		message[n++] = 0xf0;
		message[n++] = 0x41;            /* Roland */
		message[n++] = 0x10;            /* device 17 */
		message[n++] = 0x16;            /* MT-32 */
		message[n++] = 0x12;            /* data set */
		message[n++] = (address >> 16) & 0x7f;
		message[n++] = (address >> 8) & 0x7f;
		message[n++] = address & 0x7f;
		sum = message[5] + message[6] + message[7];
		for (i = 0; i < count; i++) {
			message[n] = *p++;
			sum += message[n++];
		}
		message[n++] = -sum & 0x7f;
		message[n++] = 0xf7;
		SendMidi(message, n);
		flat = ((address >> 16) & 0x7f) << 14 | ((address >> 8) & 0x7f) << 7 | (address & 0x7f);
		flat += count;
		address = (flat >> 14) << 16 | ((flat >> 7) & 0x7f) << 8 | (flat & 0x7f);
	}
}

extern "C" void ResetMt32drvGlobals(void)
{
	memset(&Mt32Info, 0, sizeof Mt32Info);
	memcpy(BankPatch, BankPatchStart, sizeof BankPatch);
}
