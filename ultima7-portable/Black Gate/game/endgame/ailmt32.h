#ifndef ENDGAME_AILMT32_H
#define ENDGAME_AILMT32_H

namespace Endgame {

/* Sequence states, as the driver reported them. */
#define SEQ_STOPPED     0
#define SEQ_PLAYING     1
#define SEQ_DONE        2

/* The Miles XMIDI driver for the Roland MT-32: it plays XMIDI sequences at 120 Hz and keeps the
 * MT-32's 64 memory timbres as a cache. One exists at a time; it runs on the platform's sound
 * tick from open to close. Sequence handles are 0 and up, -1 for none. */
int16_t Mt32Open(void);
void Mt32Close(const char *signOff);
int16_t Mt32RegisterSequence(const uint8_t *xmidi, int32_t size, int16_t number);
void Mt32StartSequence(int16_t handle);
void Mt32StopSequence(int16_t handle);
int16_t Mt32SequenceStatus(int16_t handle);
/* The next timbre the sequence needs and the cache lacks, as bank << 8 | patch; -1 if none. */
int16_t Mt32TimbreRequest(int16_t handle);
int16_t Mt32TimbreStatus(uint8_t bank, uint8_t patch);
/* timbre: a 16-bit size, then the MT-32 timbre's 246 bytes. */
void Mt32InstallTimbre(uint8_t bank, uint8_t patch, const uint8_t *timbre);

}

#endif
