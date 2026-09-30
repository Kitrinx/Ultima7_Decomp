#ifndef ENDGAME_SPEECH_H
#define ENDGAME_SPEECH_H

namespace Endgame {

/* A Creative Voice file in memory, streamed block by block to the speech output from a marker on.
 * It is fed each clock tick, as the sound card's interrupt fed it. */
struct SpeechStream {
	uint8_t *source;
	int32_t length;
	uint8_t active;
	uint8_t type;
	int32_t offset;
	int32_t blockEnd;
	int32_t blockSize;
	int32_t left;
	uint16_t repeats;
	SpeechStream(uint8_t *voc, int32_t size);
	~SpeechStream();
	void readHeader();
	uint8_t nextBlock();
	void play(int16_t marker);
	void service();
	uint8_t isDone();
	void stopPlayback();
};

}

#endif
