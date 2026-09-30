#ifndef ROUTINE_H
#define ROUTINE_H

/* A usecode function's code, loaded by number. */

struct DataFile;

/* the functions loaded with the one run, and where each starts */
#define LINKED_ROUTINE_COUNT    35

struct RoutineEntry {
	int id;
	unsigned offset;
};

extern RoutineEntry LinkedRoutines[LINKED_ROUTINE_COUNT];
void far AllocateLinkdep1(unsigned bytes);
void far AllocateLinkdep2(unsigned bytes);
void far ReadLinkdep1(DataFile *input, unsigned bytes);
void far ReadLinkdep2(DataFile *input, unsigned bytes);
void far LoadLinkdep();
void far LookupLinkdep1(unsigned id, unsigned *first, unsigned *count, unsigned *offset);
long far GetUsecodeOffset(unsigned index);

struct UsecodeRoutine {
	int handle;
	unsigned length, position;
	UsecodeRoutine();
	~UsecodeRoutine();
	void skip(long n) { position += n; }
	void append(DataFile *, unsigned);
	int available(unsigned);
	unsigned char load(unsigned);
	unsigned resolve(unsigned, unsigned, unsigned char);
	unsigned char readByte();
	int readWord();
	long text(unsigned, unsigned);
};

void far InitUsecodeIndex();

extern char Linkdep1FileName[];
extern char Linkdep2FileName[];
extern char UsecodeFileName[];
extern long Linkdep1Block;
extern long Linkdep2Block;
extern unsigned Linkdep1Count;
extern unsigned Linkdep2Size;
extern long UnusedRoutineGlobal;

#endif
