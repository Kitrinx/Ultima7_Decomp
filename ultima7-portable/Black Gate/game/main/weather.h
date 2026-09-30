#ifndef WEATHER_H
#define WEATHER_H

struct Particle;
struct Weather;
struct WeatherSaver;

#define WEATHER_PARTICLES   125

/* One falling drop or flake, drawn along a diagonal from the top of the screen. */
struct Particle {
	int16_t x;
	uint8_t y, endY, splash, frame;
	Particle() { endY = 0; }
};

/* A shower of particles; phase steps it through building up, holding, dying down and ending. */
struct Weather {
	Particle particles[WEATHER_PARTICLES];
	int16_t targetCount, maxCount, activeCount;
	uint8_t speed;
	int16_t firstFrame, firstSplashFrame, frameCount, splashFrames;
	uint8_t splashes, direction, bursts;
	int16_t delay;
	uint8_t phase;
	Weather();
};

struct WeatherSet : Weather {
	uint8_t type;
	WeatherSet();
};

extern WeatherSet CurrentWeather;
extern WeatherSaver WeatherFile;
extern char WeatherFileName[];
uint8_t Particle_isFalling(Particle *p);
uint8_t Particle_isOnScreen(Particle *p);
uint8_t Particle_willShow(Particle *p);
void Particle_start(Particle *p, uint8_t endY);
void Particle_draw(Particle *p, uint8_t type, uint8_t paused);
void Particle_drawSplash(Particle *p, uint8_t type);
uint8_t Weather_addParticle(Weather *p);
void Weather_clearParticles(Weather *p);
int16_t Weather_findFreeParticle(Weather *p);
void Weather_update(Weather *p, uint8_t type, uint8_t advance);
void Weather_start(Weather *p, int16_t storm, int16_t maximum, uint8_t speed, int16_t firstFrame, int16_t frameCount,
	uint8_t splashes, int16_t firstSplashFrame, int16_t splashFrames);

void SetWeather(WeatherSet *, uint8_t);
void UpdateWeather(WeatherSet *p, uint8_t advance);
void Weather_stop(Weather *p);

#endif
