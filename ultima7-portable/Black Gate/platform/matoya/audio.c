/* Sound: the 60 Hz sound tick, MT-32 music through Munt, AdLib through Nuked OPL3, and
 * 8-bit speech.
 *
 * One audio thread renders 800-frame blocks at 48 kHz, exactly one tick each:
 *
 *   sound tick (under the sound lock) -> queued MIDI into Munt -> Munt renders
 *   -> queued OPL writes into the OPL3, which renders -> speech mixed in
 *   -> queued to the output, kept 3-4 blocks ahead
 *
 * MIDI, OPL writes and speech arrive from any thread through locked queues, so only this
 * thread touches Munt and the OPL3.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mt32emu/c_interface/c_interface.h>

#include "opl3.h"

#include "backend.h"

/* Without a folder built in, the ROMs are looked for beside the game's files. */
#ifndef U7_DEFAULT_ROM_DIR
#define U7_DEFAULT_ROM_DIR files_root()
#endif

#define RATE 48000
#define BLOCK_FRAMES (RATE / PLAT_TICKS_PER_SECOND)
#define QUEUE_AHEAD_MS 60
#define MIDI_BUFFER_SIZE (256 * 1024)
#define PCM_BUFFER_SIZE 65536
#define OPL_QUEUE_SIZE 16384

/* Sound lock: recursive per thread. */
static MTY_Mutex *sound_mutex;
static _Thread_local int sound_depth;
static plat_timer_fn sound_tick;

static mt32emu_context synth;
static MTY_Audio *output;
static FILE *dump;
static MTY_Thread *thread;
static MTY_Atomic32 running;
static _Thread_local bool on_audio_thread;
/* 'a' for --adlib, 'r' for --roland, 0 to use the MT-32 when its ROMs are found. */
static char music_choice;

/* MIDI bytes waiting for the audio thread. */
static MTY_Mutex *midi_mutex;
static uint8_t midi_buffer[MIDI_BUFFER_SIZE];
static uint32_t midi_count;
static bool midi_overflow_logged;
static bool synth_reset_pending;

/* Speech samples waiting to play, unsigned 8-bit mono at pcm_rate. */
static MTY_Mutex *pcm_mutex;
static uint8_t pcm_buffer[PCM_BUFFER_SIZE];
static uint32_t pcm_head;
static uint32_t pcm_count;
static uint32_t pcm_rate;
static double pcm_phase;

void plat_sound_lock(void)
{
	/* Static constructors may get here before main() creates the mutex. */
	if (sound_depth++ == 0 && sound_mutex != NULL)
		MTY_MutexLock(sound_mutex);
}

void plat_sound_unlock(void)
{
	if (--sound_depth == 0 && sound_mutex != NULL)
		MTY_MutexUnlock(sound_mutex);
}

void audio_drop_lock(void)
{
	if (sound_depth > 0) {
		sound_depth = 0;
		MTY_MutexUnlock(sound_mutex);
	}
}

void plat_sound_tick_set(plat_timer_fn fn)
{
	plat_sound_lock();
	sound_tick = fn;
	plat_sound_unlock();
}

/* ---- MIDI ---- */

int16_t plat_midi_available(void)
{
	return synth != NULL;
}

void plat_midi_send(const uint8_t *message, int16_t length)
{
	if (synth == NULL || length <= 0)
		return;
	MTY_MutexLock(midi_mutex);
	if (midi_count + (uint32_t) length <= MIDI_BUFFER_SIZE) {
		memcpy(midi_buffer + midi_count, message, (size_t) length);
		midi_count += (uint32_t) length;
	} else if (!midi_overflow_logged) {
		midi_overflow_logged = true;
		plat_log("u7: MIDI queue full, messages dropped\n");
	}
	MTY_MutexUnlock(midi_mutex);
}

/* Back to power-on state, as a new process found it: no notes, no uploaded timbres. */
static void reopen_synth(void)
{
	mt32emu_close_synth(synth);
	if (mt32emu_open_synth(synth) != MT32EMU_RC_OK) {
		fprintf(stderr, "u7: the MT-32 synth could not restart; music is off\n");
		mt32emu_free_context(synth);
		synth = NULL;
	}
}

static void play_queued_midi(void)
{
	static uint8_t taken[MIDI_BUFFER_SIZE];
	uint32_t count;
	bool reset;

	MTY_MutexLock(midi_mutex);
	count = midi_count;
	memcpy(taken, midi_buffer, count);
	midi_count = 0;
	reset = synth_reset_pending;
	synth_reset_pending = false;
	MTY_MutexUnlock(midi_mutex);

	if (reset)
		reopen_synth();
	/* The stream parser handles SysEx and running status alike. */
	if (synth != NULL && count > 0)
		mt32emu_parse_stream(synth, taken, count);
}

/* Control and PCM ROMs of a real MT-32; the CM-32L's and split halves are skipped. */
static bool is_mt32_control(const mt32emu_rom_info *info)
{
	const char *id = info->control_rom_id;
	size_t n = id ? strlen(id) : 0;

	return n > 0 && strncmp(id, "ctrl_mt32_", 10) == 0
		&& !(n > 2 && id[n - 2] == '_' && (id[n - 1] == 'a' || id[n - 1] == 'b'));
}

static bool is_mt32_pcm(const mt32emu_rom_info *info)
{
	return info->pcm_rom_id != NULL && strcmp(info->pcm_rom_id, "pcm_mt32") == 0;
}

static bool find_roms(const char *dir, char *control, char *pcm, size_t size)
{
	MTY_FileList *files = MTY_GetFileList(dir, NULL);

	control[0] = pcm[0] = '\0';
	for (uint32_t i = 0; files != NULL && i < files->len; i++) {
		mt32emu_rom_info info = {0};

		if (files->files[i].dir
			|| mt32emu_identify_rom_file(&info, files->files[i].path, NULL) != MT32EMU_RC_OK)
			continue;
		if (control[0] == '\0' && is_mt32_control(&info))
			snprintf(control, size, "%s", files->files[i].path);
		else if (pcm[0] == '\0' && is_mt32_pcm(&info))
			snprintf(pcm, size, "%s", files->files[i].path);
	}
	MTY_FreeFileList(&files);
	return control[0] != '\0' && pcm[0] != '\0';
}

static mt32emu_report_handler_version MT32EMU_C_CALL report_version(mt32emu_report_handler_i i)
{
	(void) i;
	return MT32EMU_REPORT_HANDLER_VERSION_0;
}

/* Munt's debug notes (such as drum keys the MT-32 has no sound for, which the scores do play)
 * are dropped; other reports keep Munt's defaults. */
static void MT32EMU_C_CALL ignore_debug(void *instance, const char *fmt, va_list list)
{
	(void) instance;
	(void) fmt;
	(void) list;
}

static const mt32emu_report_handler_i_v0 quiet_reports = {report_version, ignore_debug};

static mt32emu_context open_synth(void)
{
	const char *dir = getenv("U7_MT32_ROMS");
	mt32emu_report_handler_i reports = {&quiet_reports};
	char control[1024], pcm[1024];
	mt32emu_context context;

	if (dir == NULL || dir[0] == '\0')
		dir = U7_DEFAULT_ROM_DIR;
	if (!find_roms(dir, control, pcm, sizeof control)) {
		fprintf(stderr, "u7: no MT-32 ROMs in %s\n", dir);
		return NULL;
	}
	context = mt32emu_create_context(reports, NULL);
	if (mt32emu_add_rom_file(context, control) != MT32EMU_RC_ADDED_CONTROL_ROM
		|| mt32emu_add_rom_file(context, pcm) != MT32EMU_RC_ADDED_PCM_ROM) {
		fprintf(stderr, "u7: MT-32 ROMs in %s could not be loaded\n", dir);
		mt32emu_free_context(context);
		return NULL;
	}
	/* Accurate mode renders natively at 48 kHz, so no resampling is needed. */
	mt32emu_set_analog_output_mode(context, MT32EMU_AOM_ACCURATE);
	mt32emu_set_stereo_output_samplerate(context, RATE);
	if (mt32emu_open_synth(context) != MT32EMU_RC_OK) {
		fprintf(stderr, "u7: the MT-32 synth could not start\n");
		mt32emu_free_context(context);
		return NULL;
	}
	return context;
}

/* ---- AdLib ----
 *
 * Port state and the timers live here, on the caller's thread, so a status read sees its own
 * writes at once; the register writes go to the audio thread, which owns the chip. */

typedef struct opl_write {
	int64_t us;
	uint16_t reg;
	uint8_t value;
	bool in_tick;
} opl_write;

static opl3_chip opl;
static MTY_Mutex *opl_mutex;
static opl_write opl_queue[OPL_QUEUE_SIZE];
static uint32_t opl_count;
static bool opl_overflow_logged;
static bool opl_reset_pending;
/* The chip renders only once a program has written to it. */
static bool opl_in_use;
static int64_t opl_last_block_us;

static uint16_t opl_address;
static bool opl_new_mode;
/* ISA time spent in port accesses, which native code doesn't take. */
static int64_t opl_io_us;

/* Timer 1 counts 80 us steps, timer 2 320 us steps, from the preset up to 256; each overflow
 * sets the timer's flag unless masked, and reloads. */
static struct {
	uint8_t preset;
	bool running;
	bool masked;
	bool flag;
	int64_t start_us;
	int64_t period_us;
	int64_t overflows_seen;
} opl_timer[2];

bool plat_opl_available(void)
{
	return opl_mutex != NULL;
}

static int64_t opl_clock_us(void)
{
	return backend_elapsed_us() + opl_io_us;
}

static void opl_update_timers(int64_t now)
{
	for (int i = 0; i < 2; i++) {
		int64_t overflows;

		if (!opl_timer[i].running || now < opl_timer[i].start_us)
			continue;
		overflows = (now - opl_timer[i].start_us) / opl_timer[i].period_us;
		if (overflows > opl_timer[i].overflows_seen) {
			opl_timer[i].overflows_seen = overflows;
			if (!opl_timer[i].masked)
				opl_timer[i].flag = true;
		}
	}
}

/* Register 4: bit 7 clears both flags (and nothing else); otherwise bits 6 and 5 mask timers
 * 1 and 2, and bits 0 and 1 start or stop them. */
static void opl_timer_control(uint8_t value, int64_t now)
{
	opl_update_timers(now);
	if (value & 0x80) {
		opl_timer[0].flag = opl_timer[1].flag = false;
		return;
	}
	for (int i = 0; i < 2; i++) {
		bool start = (value & (1 << i)) != 0;

		opl_timer[i].masked = (value & (i == 0 ? 0x40 : 0x20)) != 0;
		if (opl_timer[i].masked)
			opl_timer[i].flag = false;
		if (start && !opl_timer[i].running) {
			opl_timer[i].start_us = now;
			opl_timer[i].period_us = (256 - opl_timer[i].preset) * (i == 0 ? 80 : 320);
			opl_timer[i].overflows_seen = 0;
		}
		opl_timer[i].running = start;
	}
}

void plat_opl_write(uint16_t port, uint8_t value)
{
	int64_t now;

	MTY_MutexLock(opl_mutex);
	opl_io_us++;
	now = opl_clock_us();
	switch (port & 3) {
	case 0:
		opl_address = value;
		break;
	case 2:
		/* The second bank answers only in OPL3 mode, apart from the mode register. */
		opl_address = (opl_new_mode || value == 0x05) ? 0x100 | value : value;
		break;
	default:
		if (opl_address == 0x02 || opl_address == 0x03)
			opl_timer[opl_address - 2].preset = value;
		else if (opl_address == 0x04)
			opl_timer_control(value, now);
		else if (opl_address == 0x105)
			opl_new_mode = (value & 1) != 0;
		if (opl_count < OPL_QUEUE_SIZE) {
			opl_queue[opl_count++] = (opl_write) {backend_elapsed_us(), opl_address, value,
				on_audio_thread};
		} else if (!opl_overflow_logged) {
			opl_overflow_logged = true;
			plat_log("u7: OPL queue full, writes dropped\n");
		}
		break;
	}
	MTY_MutexUnlock(opl_mutex);
}

/* An OPL3's status: bit 7 when either flag is up, bit 6 timer 1, bit 5 timer 2; the low bits
 * an OPL2 set read as zero. */
uint8_t plat_opl_read(uint16_t port)
{
	uint8_t status = 0;

	(void) port;
	MTY_MutexLock(opl_mutex);
	opl_io_us++;
	opl_update_timers(opl_clock_us());
	if (opl_timer[0].flag)
		status |= 0x40;
	if (opl_timer[1].flag)
		status |= 0x20;
	if (status != 0)
		status |= 0x80;
	MTY_MutexUnlock(opl_mutex);
	return status;
}

/* ---- Speech ---- */

void plat_pcm_start(uint32_t rate)
{
	MTY_MutexLock(pcm_mutex);
	pcm_rate = rate;
	pcm_head = 0;
	pcm_count = 0;
	pcm_phase = 0.0;
	MTY_MutexUnlock(pcm_mutex);
}

int32_t plat_pcm_queue(const uint8_t *samples, int32_t count)
{
	int32_t taken = 0;

	MTY_MutexLock(pcm_mutex);
	while (taken < count && pcm_count < PCM_BUFFER_SIZE) {
		pcm_buffer[(pcm_head + pcm_count) % PCM_BUFFER_SIZE] = samples[taken++];
		pcm_count++;
	}
	MTY_MutexUnlock(pcm_mutex);
	return taken;
}

int32_t plat_pcm_pending(void)
{
	int32_t pending;

	MTY_MutexLock(pcm_mutex);
	pending = (int32_t) pcm_count;
	MTY_MutexUnlock(pcm_mutex);
	return pending;
}

void plat_pcm_stop(void)
{
	plat_pcm_start(0);
}

static int16_t clamp_sample(int32_t v)
{
	return (int16_t) (v < -32768 ? -32768 : v > 32767 ? 32767 : v);
}

/* Speech played through a Sound Blaster 2.0: its DAC held each sample until the next, and
 * its output went through a 2-pole low-pass at about 4.8 kHz. */
#define SB_FILTER_HZ 4800.0

static double lp_b0, lp_b1, lp_b2, lp_a1, lp_a2;
static double lp_x1, lp_x2, lp_y1, lp_y2;

/* Butterworth biquad (Q = 1/sqrt 2). */
static void sb_filter_init(void)
{
	double w = 2.0 * 3.14159265358979323846 * SB_FILTER_HZ / RATE;
	double alpha = sin(w) / (2.0 * 0.70710678118654752440);
	double a0 = 1.0 + alpha;

	lp_b0 = (1.0 - cos(w)) / 2.0 / a0;
	lp_b1 = (1.0 - cos(w)) / a0;
	lp_b2 = lp_b0;
	lp_a1 = -2.0 * cos(w) / a0;
	lp_a2 = (1.0 - alpha) / a0;
}

static double sb_filter(double x)
{
	double y = lp_b0 * x + lp_b1 * lp_x1 + lp_b2 * lp_x2 - lp_a1 * lp_y1 - lp_a2 * lp_y2;

	/* Settle to exact zero in silence rather than decaying forever. */
	if (y > -1e-3 && y < 1e-3)
		y = 0.0;
	lp_x2 = lp_x1;
	lp_x1 = x;
	lp_y2 = lp_y1;
	lp_y1 = y;
	return y;
}

/* Adds speech to the block, each sample held for its share of 48 kHz, then filtered. Running
 * out is silence; the filter keeps running so a clip's end rings out as it did. */
static void mix_speech(int16_t *block)
{
	MTY_MutexLock(pcm_mutex);
	for (int i = 0; i < BLOCK_FRAMES; i++) {
		double x = 0.0;
		int32_t v;

		if (pcm_rate > 0 && pcm_count > 0) {
			x = (pcm_buffer[pcm_head] - 128) * 256.0;
			pcm_phase += (double) pcm_rate / RATE;
			while (pcm_phase >= 1.0 && pcm_count > 0) {
				pcm_phase -= 1.0;
				pcm_head = (pcm_head + 1) % PCM_BUFFER_SIZE;
				pcm_count--;
			}
		}
		v = (int32_t) lrint(sb_filter(x));
		block[2 * i] = clamp_sample(block[2 * i] + v);
		block[2 * i + 1] = clamp_sample(block[2 * i + 1] + v);
	}
	MTY_MutexUnlock(pcm_mutex);
}

/* FM through a Sound Blaster 16. The YAC512 DAC holds each of the chip's 49716 Hz samples until
 * the next, and the board passes that staircase on without further filtering in the audio
 * band, so the hold's droop (about 1.3 dB at 16 kHz) is the whole response. Each 48 kHz
 * sample is the staircase band-limited to 22 kHz: a Kaiser-windowed sinc integrated over one
 * hold, taken from a table of kernels at FM_PHASES points between two chip samples. */
#define OPL_RATE 49716
#define FM_TAPS 64
#define FM_PHASES 256
#define FM_CUTOFF_HZ 22000.0
#define FM_KAISER_BETA 8.0
#define FM_HISTORY 128

static float fm_kernel[FM_PHASES + 1][FM_TAPS];
static int16_t fm_history[FM_HISTORY][2];
/* Chip samples made since reset, and the output's place among them: fm_index plus
 * fm_step_rest / RATE. */
static int64_t fm_made;
static int64_t fm_index;
static int64_t fm_step_rest;

static double bessel_i0(double x)
{
	double sum = 1.0, term = 1.0;

	for (int k = 1; k < 32; k++) {
		term *= (x / (2.0 * k)) * (x / (2.0 * k));
		sum += term;
	}
	return sum;
}

/* The band-limiting kernel at v chip samples from its centre. */
static double fm_lowpass(double v)
{
	const double pi = 3.14159265358979323846;
	double fc = FM_CUTOFF_HZ / OPL_RATE;
	double half = FM_TAPS / 2 - 1;
	double r = v / half;
	double sinc = v == 0.0 ? 1.0 : sin(2.0 * pi * fc * v) / (2.0 * pi * fc * v);

	if (r <= -1.0 || r >= 1.0)
		return 0.0;
	return 2.0 * fc * sinc * bessel_i0(FM_KAISER_BETA * sqrt(1.0 - r * r))
		/ bessel_i0(FM_KAISER_BETA);
}

static void opl_filter_init(void)
{
	for (int phase = 0; phase <= FM_PHASES; phase++) {
		double sum = 0.0;

		for (int k = 0; k < FM_TAPS; k++) {
			/* The held sample covers [u - 1, u] of the kernel; Simpson's rule over it. */
			double u = (double) phase / FM_PHASES + k - FM_TAPS / 2, area = 0.0;

			for (int s = 0; s <= 16; s++)
				area += (s == 0 || s == 16 ? 1 : s % 2 ? 4 : 2) * fm_lowpass(u - 1.0 + s / 16.0);
			fm_kernel[phase][k] = (float) (area / 48.0);
			sum += fm_kernel[phase][k];
		}
		for (int k = 0; k < FM_TAPS; k++)
			fm_kernel[phase][k] = (float) (fm_kernel[phase][k] / sum);
	}
}

static void opl_filter_reset(void)
{
	memset(fm_history, 0, sizeof fm_history);
	fm_made = 0;
	fm_index = 0;
	fm_step_rest = 0;
}

/* The next 48 kHz frame, running the chip as far ahead as the kernel reaches. */
static void opl_filter(double *left, double *right)
{
	double at = (double) fm_step_rest / RATE * FM_PHASES, l = 0.0, r = 0.0;
	int phase = (int) at;
	double blend = at - phase;

	while (fm_made <= fm_index + FM_TAPS / 2) {
		OPL3_Generate(&opl, fm_history[fm_made % FM_HISTORY]);
		fm_made++;
	}
	for (int k = 0; k < FM_TAPS; k++) {
		int64_t n = fm_index + FM_TAPS / 2 - k;
		double c = fm_kernel[phase][k] + blend * (fm_kernel[phase + 1][k] - fm_kernel[phase][k]);

		if (n < 0)
			continue;
		l += c * fm_history[n % FM_HISTORY][0];
		r += c * fm_history[n % FM_HISTORY][1];
	}
	*left = l;
	*right = r;
	fm_step_rest += OPL_RATE;
	fm_index += fm_step_rest / RATE;
	fm_step_rest %= RATE;
}

/* Frame in this block where a write made at us lands: its place between the last block and
 * this one. */
static uint32_t opl_offset(int64_t us, int64_t now)
{
	int64_t span = now - opl_last_block_us;
	int64_t offset;

	if (span <= 0 || us <= opl_last_block_us)
		return 0;
	offset = (us - opl_last_block_us) * BLOCK_FRAMES / span;
	return (uint32_t) (offset >= BLOCK_FRAMES ? BLOCK_FRAMES - 1 : offset);
}

/* Plays the queued writes into the chip at their places in the block, in the order they were
 * made, and adds what it renders. The sound tick runs as the block is made, so its writes land
 * at the block's end, after the game's: a note's timbre is loaded before the tick keys it. */
static void mix_opl(int16_t *block)
{
	static opl_write taken[OPL_QUEUE_SIZE];
	static opl_write ordered[OPL_QUEUE_SIZE];
	static uint32_t at[OPL_QUEUE_SIZE];
	int64_t now = backend_elapsed_us();
	uint32_t count, n = 0, next = 0;
	bool reset;

	MTY_MutexLock(opl_mutex);
	count = opl_count;
	memcpy(taken, opl_queue, count * sizeof *taken);
	opl_count = 0;
	reset = opl_reset_pending;
	opl_reset_pending = false;
	MTY_MutexUnlock(opl_mutex);

	if (reset) {
		OPL3_Reset(&opl, OPL_RATE);
		opl_filter_reset();
		opl_in_use = false;
	}
	for (uint32_t i = 0; i < count; i++) {
		at[n] = taken[i].in_tick ? BLOCK_FRAMES - 1 : opl_offset(taken[i].us, now);
		if (n > 0 && at[n] < at[n - 1])
			at[n] = at[n - 1];
		ordered[n++] = taken[i];
	}
	if (count > 0)
		opl_in_use = true;
	for (uint32_t i = 0; opl_in_use && i < BLOCK_FRAMES; i++) {
		double left, right;

		while (next < n && at[next] <= i) {
			OPL3_WriteRegBuffered(&opl, ordered[next].reg, ordered[next].value);
			next++;
		}
		opl_filter(&left, &right);
		block[2 * i] = clamp_sample(block[2 * i] + (int32_t) lrint(left));
		block[2 * i + 1] = clamp_sample(block[2 * i + 1] + (int32_t) lrint(right));
	}
	opl_last_block_us = now;
}

/* ---- Between programs ---- */

void audio_reset(void)
{
	plat_sound_tick_set(NULL);

	MTY_MutexLock(midi_mutex);
	midi_count = 0;
	midi_overflow_logged = false;
	synth_reset_pending = true;
	MTY_MutexUnlock(midi_mutex);

	MTY_MutexLock(pcm_mutex);
	pcm_rate = 0;
	pcm_head = 0;
	pcm_count = 0;
	pcm_phase = 0.0;
	lp_x1 = lp_x2 = lp_y1 = lp_y2 = 0.0;
	MTY_MutexUnlock(pcm_mutex);

	MTY_MutexLock(opl_mutex);
	opl_count = 0;
	opl_overflow_logged = false;
	opl_reset_pending = true;
	opl_address = 0;
	opl_new_mode = false;
	memset(opl_timer, 0, sizeof opl_timer);
	MTY_MutexUnlock(opl_mutex);
}

/* ---- Audio thread ---- */

static void render_block(int16_t *block)
{
	plat_sound_lock();
	if (sound_tick != NULL)
		sound_tick();
	plat_sound_unlock();

	if (synth != NULL)
		play_queued_midi();
	if (synth != NULL) {
		mt32emu_render_bit16s(synth, block, BLOCK_FRAMES);
	} else {
		memset(block, 0, BLOCK_FRAMES * 2 * sizeof *block);
	}
	mix_opl(block);
	mix_speech(block);
	if (dump != NULL)
		fwrite(block, sizeof *block, BLOCK_FRAMES * 2, dump);
}

/* Without an output device, blocks are paced by the clock so the tick still runs. */
static bool ahead_of_output(int64_t blocks, int64_t start_us)
{
	if (output != NULL)
		return MTY_AudioGetQueued(output) >= QUEUE_AHEAD_MS;
	return blocks >= (backend_elapsed_us() - start_us) * PLAT_TICKS_PER_SECOND / 1000000;
}

static void *audio_thread(void *opaque)
{
	static int16_t block[BLOCK_FRAMES * 2];
	int64_t start_us = backend_elapsed_us();
	int64_t blocks = 0;

	(void) opaque;
	on_audio_thread = true;
	while (MTY_Atomic32Get(&running)) {
		if (ahead_of_output(blocks, start_us)) {
			MTY_Sleep(2);
			continue;
		}
		render_block(block);
		if (output != NULL)
			MTY_AudioQueue(output, block, BLOCK_FRAMES);
		blocks++;
	}
	return NULL;
}

void audio_choose_music(char device)
{
	music_choice = device;
}

void audio_start(void)
{
	const char *dump_path = getenv("U7_AUDIO_DUMP");

	sound_mutex = MTY_MutexCreate();
	midi_mutex = MTY_MutexCreate();
	pcm_mutex = MTY_MutexCreate();
	opl_mutex = MTY_MutexCreate();
	sb_filter_init();
	opl_filter_init();
	OPL3_Reset(&opl, OPL_RATE);
	if (music_choice != 'a')
		synth = open_synth();
	if (synth == NULL)
		fprintf(stderr, music_choice == 'r' ? "u7: --roland needs the MT-32 ROMs; playing AdLib\n"
			: "u7: playing AdLib music\n");
	/* Playback starts once two blocks are queued. Scripted test runs stay silent. */
	if (getenv("U7_TEST_INPUT") == NULL)
		output = MTY_AudioCreate(RATE, 34, 250, 2, NULL, true);
	else
		fprintf(stderr, "u7: test input; sound is off\n");
	if (output == NULL && getenv("U7_TEST_INPUT") == NULL)
		fprintf(stderr, "u7: no audio output; sound is off\n");
	/* Raw 16-bit stereo at 48 kHz, for checking what was rendered. */
	if (dump_path != NULL && dump_path[0] != '\0')
		dump = fopen(dump_path, "wb");
	MTY_Atomic32Set(&running, 1);
	thread = MTY_ThreadCreate(audio_thread, NULL);
}

