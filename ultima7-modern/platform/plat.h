/* The platform layer: everything the game needs from the host.
 *
 *   game/
 *     |  plat_* calls only; no OS headers, interrupts, ports or BIOS
 *   plat.h
 *     |
 *   backend (one per target, chosen when linking): matoya, ...
 *
 * Threads: each program runs on one thread (called the main thread here, whatever the host
 * calls it) except the sound tick, which the audio backend calls on its own clock.
 * Code shared with the sound tick is bracketed by plat_sound_lock()/plat_sound_unlock(),
 * where the DOS game disabled interrupts.
 */
#ifndef PLAT_H
#define PLAT_H

#include <stdbool.h>
#include <stdint.h>

/* The game packs its own structs; these keep the host's layout on both sides. */
#pragma pack(push)
#pragma pack()

#ifdef __cplusplus
extern "C" {
#endif

/* ---- System ---- */

/* The backend owns the process's main(). It opens the window and audio once, then runs the start
 * menu ("startmenu", whose exit code is 1 Black Gate, 2 Serpent Isle, 0 quit) and the chosen
 * game's launcher (Ultima7Main or SerpentMain) in turn until the player quits. With --game it
 * runs that game's launcher alone; with a program named, just that one program. */
int16_t Ultima7Main(int16_t argc, char **argv);
int16_t SerpentMain(int16_t argc, char **argv);
/* The game being played: 0 Black Gate, 1 Serpent Isle. Kept across resets. */
int16_t plat_played_game(void);
/* Switches the played game: its folder, its saves folder and the window title. */
void plat_set_played_game(int16_t game);
/* The npc_pace setting: 1 when townsfolk run their routines at half speed (the default), 0 for
 * the original pace. */
int16_t plat_relaxed_npc_pace(void);

#define PLAT_GAME_FPS_MIN 2
#define PLAT_GAME_FPS_MAX 10

/* Settings by name, kept in settings.json and across resets; names not listed below read from
 * the file alone, 0 when absent (tests use no_start_items, full_rate_animation and
 * original_lighting). A command-line switch overrides one for the run until it is set here.
 *   aspect 0 4:3, 1 16:10, 2 16:9    npc_pace 1 relaxed, 0 original    runes 1 translated
 *   fullscreen 0    volume 10 (tenths)    sound 0 best available, 1 Adlib, 2 MT32, 3 OPL3
 *   filter 0 fill, 1 pixel-exact    cheats 0    frame_limit 2–10 FPS (default 10) */
int16_t plat_setting(const char *name);
void plat_set_setting(const char *name, int16_t value);
/* Writes settings.json. */
void plat_save_settings(void);
/* Calls the entry point of the program with this DOS name ("si", "mainmenu", ...). */
int16_t ProgramMain(const char *name, int16_t argc, char **argv);
/* SI.EXE's entry point. */
int16_t GameMain(int16_t argc, char **argv);
/* Provided by the game: puts all its globals back to their start-up values. */
void ResetEnvironment(void);
/* Runs the named program in this process, as SERPENT.COM ran each EXE, and waits for its
 * exit code. The backend first clears what it holds for the game and calls ResetEnvironment,
 * so the program starts as if the process were new. */
int16_t plat_run_program(const char *name, int16_t argc, char **argv);
/* Shows the message and ends the process. */
void plat_fatal(const char *message);
/* Ends the program with the DOS exit code the launcher used to see. Its stack is not
 * unwound. */
void plat_exit(int16_t code);
/* Text the DOS game printed to the console. */
void plat_log(const char *text);
/* Text on the screen, as the DOS game's printf (through DOS to the BIOS) and conio's cprintf
 * put it there. In graphics mode the screen holds 40 by 25 characters of 8x8; after textmode it
 * is 80 by 25 colour text. Positions count from 1 inside conio's window, as gotoxy's did. */
void plat_console_goto(int16_t x, int16_t y);
int16_t plat_console_where_x(void);
int16_t plat_console_where_y(void);
/* printf: the BIOS teletype. Handles \r, \n (down a row only), \b and \a. */
void plat_console_write(const char *text);
/* cprintf and putch: in conio's text attribute, wrapping and scrolling inside its window. */
void plat_console_cputs(const char *text);
void plat_console_putch(uint8_t ch);
void plat_console_window(int16_t left, int16_t top, int16_t right, int16_t bottom);
void plat_console_text_color(int16_t color);
void plat_console_text_background(int16_t color);
void plat_console_clrscr(void);
/* textmode(C80): 80 by 25 colour text. */
void plat_console_text_mode(void);
/* Back to mode 13h without clearing the screen (int 10h AX=0093h). */
void plat_console_graphics_mode(void);

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

/* ---- Video: 200 lines of 320 to 432 pixels, one byte a pixel ---- */

#define PLAT_SCREEN_WIDTH 320
#define PLAT_SCREEN_MAX_WIDTH 432
#define PLAT_SCREEN_HEIGHT 200

/* The screen width the aspect setting asks for: 320, 384 or 432. */
int16_t plat_video_aspect_width(void);
/* The width of the frames presented from now on. A new program starts at 320; frames narrower
 * than the aspect setting's width are shown with black sides. */
void plat_video_set_width(int16_t width);
/* Shows a whole frame: 200 rows of the set width. */
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

/* ---- Mouse: x in 0..2W-1 (W the frame's width), y in 0..199, as the DOS mouse driver reported ---- */

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
 * Names are the game's DOS names ("STATIC\\SHAPES.VGA"), with either separator and any letter
 * case. Writes go to the played game's saves folder; reads look there, then in the game's folder.
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

/* Opens a file of either game's folder for reading (0 Black Gate, 1 Serpent Isle), whichever is
 * played; -1 on failure. */
int16_t plat_game_file_open(int16_t game, const char *name);

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

/* A ROM-backed MT-32, available even when Adlib is selected. */
int16_t plat_mt32_available(void);
/* MIDI to the selected MT-32: a channel message (1-3 bytes) or a whole SysEx message. */
int16_t plat_midi_available(void);
void plat_midi_send(const uint8_t *message, int16_t length);

/* Enhanced FM, selected for this program. */
bool plat_opl3_enabled(void);
/* An OPL3's ISA ports: address 0x388, data 0x389 (second bank 0x38A/0x38B).
 * Writes are queued to the audio thread; reads return the chip's status and timers. */
bool plat_opl_available(void);
void plat_opl_write(uint16_t port, uint8_t value);
uint8_t plat_opl_read(uint16_t port);

/* MT32 pan runs right to left, with 64 centered. */
static inline uint8_t plat_opl_pan_value(int16_t pan)
{
	pan = pan < 0 ? 0 : pan > 127 ? 127 : pan;
	return (uint8_t) (pan <= 64 ? 255 - (pan * 127 + 32) / 64 :
		((127 - pan) * 128 + 31) / 63);
}

/* Digitised speech: unsigned 8-bit mono at the given rate. Never blocks. */
void plat_pcm_start(uint32_t rate);
/* Queues samples; returns how many were taken. */
int32_t plat_pcm_queue(const uint8_t *samples, int32_t count);
/* Plays a FLEX entry containing a 48 kHz stereo PCM WAV. False leaves playback intact. */
bool plat_pcm_play_hq(const char *archive, int16_t entry);
/* Frames queued and not yet played. */
int32_t plat_pcm_pending(void);
void plat_pcm_stop(void);

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif
