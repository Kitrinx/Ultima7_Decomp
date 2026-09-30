#ifndef FLAGS_H
#define FLAGS_H

#include "datanode.h"

/* one bit per usecode flag, most significant bit first */
class GameFlagSet : public DataNode {
	char *bits;
	int16_t count;
	int16_t size;
	char *file;
	int32_t sum;
public:
	GameFlagSet(char *);
	~GameFlagSet();
	void checksum();
	void verify(char *where);
	void init();
	char *name();
	void load(char *dir);
	void save(char *dir);
	void refresh(char *dir);
	uint8_t get(int16_t);
	void set(int16_t, int8_t);
	int16_t find(char *, char *, int16_t);
	void *read(char *, int16_t *);
	int16_t pick();
	void edit();
	void show();
};

extern GameFlagSet GameFlags;

void VerifyFlags(char *where);

extern char *const FlagInitFileName;

#endif
