#ifndef AIL_H
#define AIL_H

/* The Miles Audio Interface Library 2.14 calls the game makes, over native drivers: the MT-32
 * XMIDI driver (MT32MPU.ADV) and the Sound Blaster digital driver (SBDIG.ADV, SBPDIG.ADV).
 * AIL_register_driver takes the loaded .ADV image and picks the native driver by the device it
 * names. Timers are the platform's; the drivers run on its sound tick.
 */

#define SEQ_STOPPED     0
#define SEQ_PLAYING     1
#define SEQ_DONE        2

#define DAC_STOPPED     0
#define DAC_PAUSED      1
#define DAC_PLAYING     2
#define DAC_DONE        3

#define XMIDI_DRVR      3
#define DSP_DRVR        2

typedef int16_t HDRIVER;
typedef int16_t HSEQUENCE;

typedef struct {
	uint16_t min_API_version;
	uint16_t drvr_type;
	char data_suffix[4];
	const char *dev_name_table;
	int16_t default_IO;
	int16_t default_IRQ;
	int16_t default_DMA;
	int16_t default_DRQ;
	int16_t service_rate;
	uint16_t display_size;
} drvr_desc;

/* pack_type 0 is 8-bit unsigned mono; sample_rate is a Sound Blaster time constant. */
typedef struct {
	uint16_t pack_type;
	uint16_t sample_rate;
	void *data;
	uint32_t len;
} sound_buff;

#ifdef __cplusplus
extern "C" {
#endif

void AIL_startup(void);
void AIL_shutdown(const char *signoff_msg);

HDRIVER AIL_register_driver(void *driver_base_addr);
void AIL_release_driver_handle(HDRIVER driver);
drvr_desc *AIL_describe_driver(HDRIVER driver);
uint16_t AIL_detect_device(HDRIVER driver, uint16_t IO_addr, uint16_t IRQ, uint16_t DMA,
	uint16_t DRQ);
void AIL_init_driver(HDRIVER driver, uint16_t IO_addr, uint16_t IRQ, uint16_t DMA, uint16_t DRQ);
void AIL_shutdown_driver(HDRIVER driver, const char *signoff_msg);

void AIL_register_sound_buffer(HDRIVER driver, uint16_t buffer_num, sound_buff *buff);
uint16_t AIL_sound_buffer_status(HDRIVER driver, uint16_t buffer_num);
void AIL_play_VOC_file(HDRIVER driver, void *VOC_file, int16_t block_marker);
uint16_t AIL_VOC_playback_status(HDRIVER driver);
void AIL_start_digital_playback(HDRIVER driver);
void AIL_stop_digital_playback(HDRIVER driver);

uint16_t AIL_state_table_size(HDRIVER driver);
HSEQUENCE AIL_register_sequence(HDRIVER driver, void *FORM_XMID, uint16_t sequence_num,
	void *state_table, void *controller_table);
void AIL_release_sequence_handle(HDRIVER driver, HSEQUENCE sequence);

uint16_t AIL_default_timbre_cache_size(HDRIVER driver);
void AIL_define_timbre_cache(HDRIVER driver, void *cache_addr, uint16_t cache_size);
uint16_t AIL_timbre_request(HDRIVER driver, HSEQUENCE sequence);
uint16_t AIL_timbre_status(HDRIVER driver, int16_t bank, int16_t patch);
void AIL_install_timbre(HDRIVER driver, int16_t bank, int16_t patch, void *src_addr);
void AIL_protect_timbre(HDRIVER driver, int16_t bank, int16_t patch);
void AIL_unprotect_timbre(HDRIVER driver, int16_t bank, int16_t patch);

void AIL_start_sequence(HDRIVER driver, HSEQUENCE sequence);
void AIL_stop_sequence(HDRIVER driver, HSEQUENCE sequence);
void AIL_resume_sequence(HDRIVER driver, HSEQUENCE sequence);
uint16_t AIL_sequence_status(HDRIVER driver, HSEQUENCE sequence);
uint16_t AIL_relative_volume(HDRIVER driver, HSEQUENCE sequence);
uint16_t AIL_relative_tempo(HDRIVER driver, HSEQUENCE sequence);
void AIL_set_relative_volume(HDRIVER driver, HSEQUENCE sequence, uint16_t percent,
	uint16_t milliseconds);
void AIL_set_relative_tempo(HDRIVER driver, HSEQUENCE sequence, uint16_t percent,
	uint16_t milliseconds);
int16_t AIL_controller_value(HDRIVER driver, HSEQUENCE sequence, uint16_t channel,
	uint16_t controller_num);
void AIL_set_controller_value(HDRIVER driver, HSEQUENCE sequence, uint16_t channel,
	uint16_t controller_num, uint16_t value);
uint16_t AIL_channel_notes(HDRIVER driver, HSEQUENCE sequence, uint16_t channel);
uint16_t AIL_beat_count(HDRIVER driver, HSEQUENCE sequence);
uint16_t AIL_measure_count(HDRIVER driver, HSEQUENCE sequence);
void AIL_branch_index(HDRIVER driver, HSEQUENCE sequence, uint16_t marker_number);

void AIL_send_channel_voice_message(HDRIVER driver, uint16_t status, uint16_t data_1,
	uint16_t data_2);
void AIL_send_sysex_message(HDRIVER driver, uint16_t addr_a, uint16_t addr_b, uint16_t addr_c,
	void *data, uint16_t size, uint16_t delay);
void AIL_write_display(HDRIVER driver, const char *string);

uint16_t AIL_lock_channel(HDRIVER driver);
void AIL_map_sequence_channel(HDRIVER driver, HSEQUENCE sequence, uint16_t sequence_channel,
	uint16_t physical_channel);
uint16_t AIL_true_sequence_channel(HDRIVER driver, HSEQUENCE sequence,
	uint16_t sequence_channel);
void AIL_release_channel(HDRIVER driver, uint16_t channel);

#ifdef __cplusplus
}
#endif

#endif
