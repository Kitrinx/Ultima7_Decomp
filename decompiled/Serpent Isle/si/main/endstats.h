#ifndef ENDSTATS_H
#define ENDSTATS_H

struct GameDate {
	unsigned year;
	unsigned month;
	unsigned day;
	GameDate();
	int load(char *name);
	int save(char *name);
};

#endif
