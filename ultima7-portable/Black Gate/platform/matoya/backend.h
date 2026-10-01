/* Shared state between the parts of the libmatoya backend.
 *
 *   app thread (process main)   game thread (ProgramMain)   audio thread
 *   window, events, drawing     plat_* calls, game timers   sound tick, Munt, speech
 *        |  events_push ------------> plat_pump                   ^
 *        |  video_draw <------------ plat_video_present           |
 *                                    plat_midi_send / plat_pcm ---+
 *
 * A launcher thread runs Ultima7Main (or the one program named) and starts each program on
 * a new game thread, after the *_reset calls below clear what the last one left.
 */
#ifndef BACKEND_H
#define BACKEND_H

#include <stdbool.h>
#include <stdint.h>

#include <matoya.h>

#include "plat.h"

/* main.c */
int64_t backend_elapsed_us(void);
/* Asks the app thread to close the window and end the process with this code. */
void backend_request_exit(int code);

/* events.c */
void events_init(void);
void events_reset(void);
void events_push(const MTY_Event *event, MTY_Size view);
bool events_take_warp(MTY_Size view, uint32_t *x, uint32_t *y);
/* The getch word for a key press (scan code high, ASCII low), or 0 when BIOS gave none. */
uint16_t keys_bios_word(MTY_Key key, MTY_Mod mod);
/* The unshifted key that types c, or MTY_KEY_NONE. */
MTY_Key keys_for_char(char c);

/* The window's client size, as the app thread last saw it. */
MTY_Size backend_view_size(void);

/* testinput.c */
bool test_input_active(void);
/* Queues the U7_TEST_INPUT steps that are due, as if the user had made them. Game thread. */
void test_input_poll(void);

/* video.c */
void video_init(void);
void video_reset(void);
/* The game image inside a window of this size, in window pixels. */
void video_view_rect(MTY_Size view, int32_t *x, int32_t *y, int32_t *w, int32_t *h);
/* Integer scale matoya should use for this window, or 0 to fit. */
float video_scale(MTY_Size view);
void video_draw(MTY_App *app, MTY_Window window);
/* The game's screen memory: the buffer it last presented, or NULL before its first frame. */
uint8_t *video_screen(void);
/* Shows the last presented frame again when the palette changed or time has passed. */
void video_refresh(bool force);
/* Saves the last presented frame as a 320x200 PPM. */
bool video_write_shot(const char *path);

/* files.c */
void files_set_root(const char *dir);
/* The game's folder, as set. */
const char *files_root(void);
/* Whether the data folder holds the game's files; if not, says what is wrong in message. */
bool files_check_data(char *message, size_t size);
void files_reset(void);

/* console.c */
/* A new program's text cursor starts at the top left, as after a mode set. */
void console_reset(void);

/* nullpage.c */
/* Lets the game read and write through null pointers as DOS did; see nullpage.c. */
void nullpage_install(void);
void nullpage_reset(void);

/* audio.c */
void audio_start(void);
void audio_stop(void);
/* Stops the sound tick, flushes queued MIDI and speech, and restarts the synth as powered on. */
void audio_reset(void);
/* Releases the sound lock if this thread holds it. */
void audio_drop_lock(void);

#ifdef U7_RESET_CHECK
/* resetcheck.c: records the game's globals at start-up and after the first reset, and reports
 * what differs after each reset. */
void reset_check_startup(void);
void reset_check(void);
#endif

#endif
