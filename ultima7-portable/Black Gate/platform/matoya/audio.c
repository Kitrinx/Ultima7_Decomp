/* Sound: the 60 Hz sound tick, MT-32 music through Munt, and 8-bit speech.
 *
 * One audio thread renders 800-frame blocks at 48 kHz, exactly one tick each:
 *
 *   sound tick (under the sound lock) -> queued MIDI into Munt -> Munt renders
 *   -> speech mixed in -> queued to the output, kept 3-4 blocks ahead
 *
 * MIDI and speech arrive from any thread through locked queues, so only this thread
 * touches Munt.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mt32emu/c_interface/c_interface.h>

#include "backend.h"

#ifndef U7_DEFAULT_ROM_DIR
#define U7_DEFAULT_ROM_DIR "."
#endif

#define RATE 48000
#define BLOCK_FRAMES (RATE / PLAT_TICKS_PER_SECOND)
#define QUEUE_AHEAD_MS 60
#define MIDI_BUFFER_SIZE (256 * 1024)
#define PCM_BUFFER_SIZE 65536
#define STOP_WAIT_MS 300

/* Sound lock: recursive per thread. */
static MTY_Mutex *sound_mutex;
static _Thread_local int sound_depth;
static plat_timer_fn sound_tick;

static mt32emu_context synth;
static MTY_Audio *output;
static FILE *dump;
static MTY_Thread *thread;
static MTY_Atomic32 running;
static MTY_Atomic32 finished;

/* MIDI bytes waiting for the audio thread. */
static MTY_Mutex *midi_mutex;
static uint8_t midi_buffer[MIDI_BUFFER_SIZE];
static uint32_t midi_count;
static bool midi_overflow_logged;

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

static void play_queued_midi(void)
{
	static uint8_t taken[MIDI_BUFFER_SIZE];
	uint32_t count;

	MTY_MutexLock(midi_mutex);
	count = midi_count;
	memcpy(taken, midi_buffer, count);
	midi_count = 0;
	MTY_MutexUnlock(midi_mutex);

	/* The stream parser handles SysEx and running status alike. */
	if (count > 0)
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

static mt32emu_context open_synth(void)
{
	const char *dir = getenv("U7_MT32_ROMS");
	mt32emu_report_handler_i no_reports = {0};
	char control[1024], pcm[1024];
	mt32emu_context context;

	if (dir == NULL || dir[0] == '\0')
		dir = U7_DEFAULT_ROM_DIR;
	if (!find_roms(dir, control, pcm, sizeof control)) {
		fprintf(stderr, "u7: no MT-32 ROMs in %s; music is off\n", dir);
		return NULL;
	}
	context = mt32emu_create_context(no_reports, NULL);
	if (mt32emu_add_rom_file(context, control) != MT32EMU_RC_ADDED_CONTROL_ROM
		|| mt32emu_add_rom_file(context, pcm) != MT32EMU_RC_ADDED_PCM_ROM) {
		fprintf(stderr, "u7: MT-32 ROMs in %s could not be loaded; music is off\n", dir);
		mt32emu_free_context(context);
		return NULL;
	}
	/* Accurate mode renders natively at 48 kHz, so no resampling is needed. */
	mt32emu_set_analog_output_mode(context, MT32EMU_AOM_ACCURATE);
	mt32emu_set_stereo_output_samplerate(context, RATE);
	if (mt32emu_open_synth(context) != MT32EMU_RC_OK) {
		fprintf(stderr, "u7: the MT-32 synth could not start; music is off\n");
		mt32emu_free_context(context);
		return NULL;
	}
	return context;
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

/* ---- Audio thread ---- */

static void render_block(int16_t *block)
{
	plat_sound_lock();
	if (sound_tick != NULL)
		sound_tick();
	plat_sound_unlock();

	if (synth != NULL) {
		play_queued_midi();
		mt32emu_render_bit16s(synth, block, BLOCK_FRAMES);
	} else {
		memset(block, 0, BLOCK_FRAMES * 2 * sizeof *block);
	}
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
	MTY_Atomic32Set(&finished, 1);
	return NULL;
}

void audio_start(void)
{
	const char *dump_path = getenv("U7_AUDIO_DUMP");

	sound_mutex = MTY_MutexCreate();
	midi_mutex = MTY_MutexCreate();
	pcm_mutex = MTY_MutexCreate();
	sb_filter_init();
	synth = open_synth();
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

void audio_stop(void)
{
	if (thread == NULL)
		return;
	MTY_Atomic32Set(&running, 0);
	/* A thread stuck in the sound tick (it may have ended the program) is left alone. */
	for (int waited = 0; !MTY_Atomic32Get(&finished) && waited < STOP_WAIT_MS; waited++)
		MTY_Sleep(1);
	if (!MTY_Atomic32Get(&finished))
		return;
	MTY_ThreadDestroy(&thread);
	if (dump != NULL)
		fclose(dump);
	MTY_AudioDestroy(&output);
	if (synth != NULL) {
		mt32emu_close_synth(synth);
		mt32emu_free_context(synth);
		synth = NULL;
	}
}
