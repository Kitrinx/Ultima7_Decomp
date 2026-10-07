#ifndef ENDSTATS_H
#define ENDSTATS_H

struct GameDate {
	uint16_t year;
	uint16_t month;
	uint16_t day;
	GameDate();
	int16_t load(char *name);
	int16_t save(char *name);
};

#endif
