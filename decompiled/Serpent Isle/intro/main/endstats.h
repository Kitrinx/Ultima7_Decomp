#ifndef ENDSTATS_H
#define ENDSTATS_H

/* A calendar date; it can be kept in a file of its own. */
struct GameDate {
	int year;
	int month;
	int day;
	GameDate();
	int load(char *name);
	int save(char *name);
};

#endif
