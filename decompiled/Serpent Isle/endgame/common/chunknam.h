#ifndef CHUNKNAM_H
#define CHUNKNAM_H

struct IffFile;

/* An eight-character name stored in an IFF file. */
struct ChunkName {
	char text[9];
	ChunkName(IffFile *f) { read(f); }
	void clear() { text[0] = 0; }
	void read(IffFile *f);
	operator char *() { return text; }
};

inline void ClearName(char far *name) { name[0] = 0; }
void ReadChunkName(char far *name, IffFile *f);

/* A file name made of directory, name and extension. */
struct PathName {
	char *text;
	void clear() { text = 0; }
	void build(char *dir, char *name, char *ext);
};

#endif
