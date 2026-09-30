#ifndef ROUTINE_H
#define ROUTINE_H

/* A usecode function's code, loaded by number. */

struct DataFile;

/* the functions loaded with the one run, and where each starts */
#define LINKED_ROUTINE_COUNT    35

struct RoutineEntry {
	int16_t id;
	uint16_t offset;
};

extern RoutineEntry LinkedRoutines[LINKED_ROUTINE_COUNT];
void AllocateLinkdep1(uint16_t bytes);
void AllocateLinkdep2(uint16_t bytes);
void ReadLinkdep1(DataFile *input, uint16_t bytes);
void ReadLinkdep2(DataFile *input, uint16_t bytes);
void LoadLinkdep();
void LookupLinkdep1(uint16_t id, uint16_t *first, uint16_t *count, uint16_t *offset);
int32_t GetUsecodeOffset(uint16_t index);

struct UsecodeRoutine {
	int16_t handle;
	uint16_t length, position;
	UsecodeRoutine();
	~UsecodeRoutine();
	void skip(int32_t n) { position += n; }
	void append(DataFile *, uint16_t);
	int16_t available(uint16_t);
	uint8_t load(uint16_t);
	uint16_t resolve(uint16_t, uint16_t, uint8_t);
	uint8_t readByte();
	int16_t readWord();
	int32_t text(uint16_t, uint16_t);
};

void InitUsecodeIndex();

extern const char Linkdep1FileName[];
extern const char Linkdep2FileName[];
extern const char UsecodeFileName[];
extern int32_t Linkdep1Block;
extern int32_t Linkdep2Block;
extern uint16_t Linkdep1Count;
extern uint16_t Linkdep2Size;
extern int32_t UnusedRoutineGlobal;

#endif
