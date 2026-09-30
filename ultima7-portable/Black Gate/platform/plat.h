/* The platform layer: everything the game needs from the host.
 *
 *   game/
 *     |  plat_* calls only; no OS headers, interrupts, ports or BIOS
 *   plat.h
 *     |
 *   backend (one per target, chosen when linking): matoya, ...
 *
 * Threads: the game runs on one thread (called the main thread here, whatever the host
 * calls it) except the sound tick, which the audio backend calls on its own clock.
 * Code shared with the sound tick is bracketed by plat_sound_lock()/plat_sound_unlock(),
 * where the DOS game disabled interrupts.
 */
#ifndef PLAT_H
#define PLAT_H

#include <stdint.h>

/* The game packs its own structs; these keep the host's layout on both sides. */
#pragma pack(push)
#pragma pack()

#ifdef __cplusplus
extern "C" {
#endif

/* ---- System ---- */

/* The game's entry point. The backend owns the process's main(): it opens the window and
 * audio, then calls this on the game thread with the game's own arguments. */
int16_t GameMain(int16_t argc, char **argv);
/* Shows the message and ends the program. */
void plat_fatal(const char *message);
/* Ends the program with the DOS exit code the launcher used to see. */
void plat_exit(int16_t code);
/* Text the DOS game printed to the console. */
void plat_log(const char *text);

/* ---- Time ---- */

#define PLAT_TICKS_PER_SECOND 60

/* 60 Hz ticks since start; the game's TickCount. */
uint32_t plat_ticks(void);
uint32_t plat_milliseconds(void);
void plat_sleep(uint32_t milliseconds);
/* Handles window and input events and runs due game timers. */
void plat_pump(void);
/* For busy-waits: pump, show the last presented buffer again (the game may have drawn
 * into it), sleep briefly. */
void plat_yield(void);

typedef void (*plat_timer_fn)(void);
/* A 60 Hz callback run on the main thread from plat_pump, catching up on missed ticks. */
void plat_game_timer_add(plat_timer_fn fn);
void plat_game_timer_remove(plat_timer_fn fn);

/* The 60 Hz sound tick, run by the audio backend in step with the sound it renders. */
void plat_sound_tick_set(plat_timer_fn fn);
/* Recursive; also held by the backend around every sound tick. */
void plat_sound_lock(void);
void plat_sound_unlock(void);

/* ---- Video: 320x200, one byte a pixel ---- */

#define PLAT_SCREEN_WIDTH 320
#define PLAT_SCREEN_HEIGHT 200

/* Shows a whole frame. pixels is 320x200, 320 bytes a row. */
void plat_video_present(const uint8_t *pixels);
/* Palette entries as 6-bit VGA red, green, blue triples; applied with the next present. */
void plat_video_set_palette(int16_t first, int16_t count, const uint8_t *rgb);
/* Waits for the next 70 Hz display refresh, as the game's retrace wait did. */
void plat_video_wait_retrace(void);

/* ---- Keyboard ---- */

/* Nonzero when a key is waiting. */
int16_t plat_key_available(void);
/* The next key as getch returned it: ASCII, or 0 followed by the scan code on the next call. */
int16_t plat_key_get(void);
/* BIOS shift flags: 1 right shift, 2 left shift, 4 ctrl, 8 alt. */
uint16_t plat_key_modifiers(void);

/* ---- Mouse: x in 0..639, y in 0..199, as the DOS mouse driver reported ---- */

#define PLAT_MOUSE_MOVED 0x01
#define PLAT_MOUSE_LEFT_DOWN 0x02
#define PLAT_MOUSE_LEFT_UP 0x04
#define PLAT_MOUSE_RIGHT_DOWN 0x08
#define PLAT_MOUSE_RIGHT_UP 0x10

/* buttons: 1 left, 2 right. */
void plat_mouse_state(int16_t *x, int16_t *y, uint16_t *buttons);
/* Called from plat_pump for each mouse event, with a PLAT_MOUSE_* mask. */
typedef void (*plat_mouse_fn)(uint16_t events, uint16_t buttons, int16_t x, int16_t y);
void plat_mouse_set_handler(plat_mouse_fn fn);
void plat_mouse_move_to(int16_t x, int16_t y);

/* ---- Pointer ---- */

/* Nonzero when the backend draws the pointer; the game then skips its own save-under drawing. */
int16_t plat_cursor_is_hardware(void);
/* 8-bit pixels, index 255 transparent, in the current palette. */
void plat_cursor_set_shape(const uint8_t *pixels, int16_t width, int16_t height,
	int16_t hot_x, int16_t hot_y);
void plat_cursor_show(int16_t visible);

/* ---- Files ----
 * Names are the game's DOS names ("STATIC\\SHAPES.VGA"), resolved against the data directory
 * with either separator and any letter case.
 */

#define PLAT_FILE_READ 1
#define PLAT_FILE_WRITE 2

#define PLAT_SEEK_SET 0
#define PLAT_SEEK_CUR 1
#define PLAT_SEEK_END 2

/* Handles are 1 and up; -1 on failure. */
int16_t plat_file_open(const char *name, int16_t mode);
int16_t plat_file_create(const char *name);
void plat_file_close(int16_t file);
/* Bytes moved, or -1. */
int32_t plat_file_read(int16_t file, void *buffer, int32_t count);
int32_t plat_file_write(int16_t file, const void *buffer, int32_t count);
/* The new position, or -1. */
int32_t plat_file_seek(int16_t file, int32_t offset, int16_t origin);
int32_t plat_file_length(int16_t file);
/* Cuts the file at the current position. Nonzero on success. */
int16_t plat_file_truncate(int16_t file);

/* These return nonzero on success. */
int16_t plat_file_exists(const char *name);
int16_t plat_dir_exists(const char *name);
int16_t plat_dir_create(const char *name);
int16_t plat_dir_remove(const char *name);
int16_t plat_file_remove(const char *name);
int16_t plat_file_rename(const char *from, const char *to);
uint32_t plat_disk_free(void);

/* Directory listing with a DOS wildcard ("GAMEDAT\\*.*", "u7ifix*."). */
typedef struct plat_find {
	char name[13];
	uint32_t size;
	void *state;
} plat_find;
/* Nonzero when an entry was found. plat_find_close ends a listing early. */
int16_t plat_find_first(const char *pattern, plat_find *find);
int16_t plat_find_next(plat_find *find);
void plat_find_close(plat_find *find);

/* ---- Sound ---- */

/* MIDI to the MT-32: a channel message (1-3 bytes) or a whole SysEx message. */
int16_t plat_midi_available(void);
void plat_midi_send(const uint8_t *message, int16_t length);

/* Digitised speech: unsigned 8-bit mono at the given rate. Never blocks. */
void plat_pcm_start(uint32_t rate);
/* Queues samples; returns how many were taken. */
int32_t plat_pcm_queue(const uint8_t *samples, int32_t count);
/* Samples queued and not yet played. */
int32_t plat_pcm_pending(void);
void plat_pcm_stop(void);

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif
