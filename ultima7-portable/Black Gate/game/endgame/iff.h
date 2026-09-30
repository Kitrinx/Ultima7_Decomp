#ifndef ENDGAME_IFF_H
#define ENDGAME_IFF_H

namespace Endgame {

/* "FORM", its size and its type: where a form's chunks begin. */
#define FORM_HEADER     12

/* A chunk's header as read from the file, with where the chunk starts and ends. */
struct ChunkHeader {
	int32_t start;
	char id[5];
	int32_t size;
	int32_t end;
};

/* A FORM or CAT entered, kept on a stack so it can be left again. */
struct IffChunk {
	IffChunk *prev;
	int32_t start;
	char id[5];
	int32_t size;
	int32_t end;
};

/* An IFF file: FORM chunks entered and left like directories, chunks found by id. */
struct IffFile {
	int16_t handle;
	int32_t offset;
	int16_t depth;
	IffChunk *form;
	ChunkHeader chunk;
	IffFile() { handle = -1; form = 0; depth = 0; }
	~IffFile() { close(); }
	int16_t open(const char *name);
	void close();
	int16_t findChunk(const char *id);
	void read(int32_t size, void *buffer);
	int16_t readWord();
	int32_t readChunk(void *buffer);
	void readId();
	void readSize();
	void readHeader();
	void skipChunk() { offset = chunk.end; }
	void rewindForm();
	void enterForm();
	void leaveForm();
	int16_t isId(const char *id) { return strcmp(chunk.id, id) == 0; }
	int16_t atFormEnd() { return offset >= form->end; }
};

/* Finds the chunk of type id carrying the eight-character name, and leaves the file just past
 * the name. */
void FindNamedChunk(IffFile *file, const char *id, const char *name);

}

#endif
