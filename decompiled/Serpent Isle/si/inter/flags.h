#ifndef FLAGS_H
#define FLAGS_H

#include "datanode.h"

/* one bit per usecode flag, most significant bit first */
class GameFlagSet : public DataNode {
	char far *bits;
	int count;
	int size;
	char *file;
	long sum;
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
	unsigned char get(int);
	void set(int, char);
	int find(char far *, char *, int);
	void far *read(char *, int *);
	int pick();
	void edit();
	void show();
};

extern GameFlagSet GameFlags;

void VerifyFlags(char *where);

extern char *FlagInitFileName;

#endif
