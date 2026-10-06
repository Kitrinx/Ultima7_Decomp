#ifndef WEATHER_H
#define WEATHER_H

struct Particle;
struct Weather;
struct WeatherSaver;

#define WEATHER_PARTICLES   125

/* One falling drop or flake, drawn along a diagonal from the top of the screen. */
struct Particle {
	int x;
	unsigned char y, endY, splash, frame;
	Particle() { endY = 0; }
};

/* A shower of particles; phase steps it through building up, holding, dying down and ending. */
struct Weather {
	Particle particles[WEATHER_PARTICLES];
	int targetCount, maxCount, activeCount;
	unsigned char speed;
	int firstFrame, firstSplashFrame, frameCount, splashFrames;
	unsigned char splashes, direction, bursts;
	int delay;
	unsigned char phase;
	Weather();
};

struct WeatherSet : Weather {
	unsigned char type;
	WeatherSet();
};

extern WeatherSet CurrentWeather;
extern WeatherSaver WeatherFile;
extern char WeatherFileName[];
unsigned char Particle_isFalling(Particle *p);
unsigned char Particle_isOnScreen(Particle *p);
unsigned char Particle_willShow(Particle *p);
void Particle_start(Particle *p, unsigned char endY);
void Particle_draw(Particle *p, unsigned char type, unsigned char paused);
void Particle_drawSplash(Particle *p, unsigned char type);
unsigned char Weather_addParticle(Weather *p);
void Weather_clearParticles(Weather *p);
int Weather_findFreeParticle(Weather *p);
void Weather_update(Weather *p, unsigned char type, unsigned char advance);
void Weather_start(Weather *p, int storm, int maximum, unsigned char speed, int firstFrame, int frameCount,
	unsigned char splashes, int firstSplashFrame, int splashFrames);

void SetWeather(WeatherSet *, unsigned char);
void UpdateWeather(WeatherSet *p, unsigned char advance);
void Weather_stop(Weather *p);

#endif
