/* Serpent Isle SI.EXE, overlay segment 254 (file offsets 0x06e570 to 0x06ed36, 1990 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
#include "dosio.h"
#include "easyfile.h"
#include "u7manage.h"
#include "bltshape.h"
#include "random.h"
#include "u7npc.h"
#include "collide.h"
#include "datanode.h"
#include "u7sound.h"
#include "sprite.h"
#include "mapview.h"
#include "weather.h"

extern objref AvatarRef;

/* The WEATHER.DAT file in a saved game. */
struct WeatherSaver : DataNode {
	char *name();
	void load(char *dir);
	void save(char *dir);
};

WeatherSet CurrentWeather;
WeatherSaver WeatherFile;
char WeatherFileName[] = "WEATHER.DAT";

char *WeatherSaver::name() { return WeatherFileName; }

void WeatherSaver::save(char *path)
{
	int16_t fd;
	fd = CreateFileOrFail(BuildPath(path, WeatherFileName, 0));
	DosWrite(fd, -INT32_C(1), (int32_t)sizeof(CurrentWeather), &CurrentWeather);
	DosClose(fd);
}

void WeatherSaver::load(char *path)
{
	int16_t fd;
	fd = OpenFileOrFail(BuildPath(path, WeatherFileName, 0));
	DosRead(fd, -INT32_C(1), (int32_t)sizeof(CurrentWeather), &CurrentWeather);
	DosClose(fd);
}

uint8_t Particle_isFalling(Particle *p) { return p->y < p->endY ? 1 : 0; }

uint8_t Particle_isOnScreen(Particle *p) { return p->x + p->y < 319 ? 1 : 0; }

uint8_t Particle_willShow(Particle *p) { return p->endY + p->x > 0 ? 1 : 0; }

void Particle_start(Particle *p, uint8_t endY)
{
	p->endY = endY;
	p->y = 0;
	p->splash = 0;
	p->frame = 0;
}

void Particle_draw(Particle *p, uint8_t type, uint8_t paused)
{
	int16_t frame;
	if (CurrentWeather.frameCount != 0) {
		frame = p->frame + CurrentWeather.firstFrame;
		if (type == 3 || (!InDungeon && CeilingZ >= 15))
			ShapeManager_drawInViewport(&gShapeManager, p->x + p->y, p->y, FIRST_SPRITE_SHAPE, frame, 0, 0);
		if (!paused) {
			p->frame++;
			if (p->frame == CurrentWeather.frameCount)
				p->frame = 0;
		}
	}
}

void Particle_drawSplash(Particle *p, uint8_t type)
{
	int16_t frame;
	frame = CurrentWeather.firstSplashFrame + CurrentWeather.splashFrames - p->splash;
	if (type == 3 || (!InDungeon && CeilingZ >= 15))
		ShapeManager_drawInViewport(&gShapeManager, p->x + p->y, p->y, FIRST_SPRITE_SHAPE, frame, 0, 0);
	p->splash--;
}

Weather::Weather() { phase = 0; }

uint8_t Weather_addParticle(Weather *p)
{
	int16_t index;
	int16_t x;
	index = Weather_findFreeParticle(p);
	if (index != -1) {
		x = GenerateRandomIntegerInRange(518) - 199;
		p->particles[index].x = x;
		Particle_start(&p->particles[index], GenerateRandomIntegerInRange(198) + 1);
		if (Particle_willShow(&p->particles[index])) {
			p->activeCount++;
			p->particles[index].y += RollRandom(p->particles[index].endY);
			return 1;
		}
		p->particles[index].endY = 0;
	}
	return 0;
}

void Weather_clearParticles(Weather *p)
{
	int16_t i;
	for (i = 0; i < WEATHER_PARTICLES; i++)
		Particle_start(&p->particles[i], 0);
	p->activeCount = 0;
}

int16_t Weather_findFreeParticle(Weather *p)
{
	int16_t i;
	for (i = 0; i < WEATHER_PARTICLES; i++)
		if (!p->particles[i].endY)
			return i;
	return -1;
}

void Weather_update(Weather *p, uint8_t type, uint8_t advance)
{
	int16_t i;
	int16_t newParticles;
	int16_t chance;
	if (!advance) {
		for (i = 0; i < WEATHER_PARTICLES; i++) {
			if (p->particles[i].endY != 0)
				Particle_draw(&p->particles[i], type, 1);
		}
		return;
	}
	newParticles = GenerateRandomIntegerInRange(12) + 1;
	for (i = 0; i < WEATHER_PARTICLES; i++) {
		if (p->particles[i].endY != 0) {
			if (!Particle_isFalling(&p->particles[i])) {
				if (p->splashes != 0 && p->particles[i].splash != 0)
					Particle_drawSplash(&p->particles[i], type);
				else {
					p->particles[i].endY = 0;
					p->activeCount--;
				}
			} else {
				Particle_draw(&p->particles[i], type, 0);
				p->particles[i].y += p->speed;
				if (!Particle_isFalling(&p->particles[i]))
					p->particles[i].splash = p->splashFrames;
				if (!Particle_isOnScreen(&p->particles[i])) {
					p->particles[i].endY = 0;
					p->activeCount--;
				}
			}
		} else {
			if (p->activeCount < p->targetCount && newParticles != 0)
				if (Weather_addParticle(p))
					newParticles--;
		}
	}
	chance = 75;
	switch (p->phase) {
	case 1:
		if (p->bursts < 3 && p->delay != 0)
			chance = p->delay;
		else
			p->phase = ((int16_t)p->phase + 1) % 6;
		break;
	case 2:
		if (p->targetCount >= p->maxCount)
			p->phase = ((int16_t)p->phase + 1) % 6;
		else
			p->targetCount++;
		break;
	case 4:
		chance = 9999;
		if (p->targetCount <= 0)
			p->phase = ((int16_t)p->phase + 1) % 6;
		else
			p->targetCount--;
		break;
	case 5:
		if (p->activeCount <= 0) {
			SetWeather(&CurrentWeather, 0);
			p->phase = ((int16_t)p->phase + 1) % 6;
		}
		break;
	}
	if (p->delay != 0) {
		if ((uint8_t)(GenerateRandomIntegerInRange(chance) == 0)) {
			SpriteManager_playEdgeSprite(&gSpriteManager, p->direction, RollRandom(3) + 1, 1026,
				GenerateRandomIntegerInRange(3), -2, 5);
			p->bursts++;
		}
	}
}

void Weather_start(Weather *p, int16_t storm, int16_t maximum, uint8_t speed, int16_t firstFrame, int16_t frameCount,
	uint8_t splashes, int16_t firstSplashFrame, int16_t splashFrames)
{
	Weather_clearParticles(p);
	p->targetCount = 1;
	p->direction = ((uint8_t)((uint8_t)(GetNpcBufferForIbo(&AvatarRef)->status & 7) + 3) +
		RollRandom(3)) % 8;
	p->bursts = 0;
	p->delay = (storm * 25) / 2;
	p->maxCount = maximum;
	p->speed = speed;
	p->firstFrame = firstFrame;
	p->frameCount = frameCount;
	p->splashes = splashes;
	p->firstSplashFrame = firstSplashFrame;
	p->splashFrames = splashFrames;
	p->phase = storm ? 1 : 2;
}

void UpdateWeather(WeatherSet *p, uint8_t advance)
{
	Weather_update(p, p->type, advance);
}

void Weather_stop(Weather *p) { p->phase = 4; }

void SetWeather(WeatherSet *p, uint8_t type)
{
	p->type = type;
	switch (p->type) {
	case 0:
		Weather_stop(p);
		if (CurrentMusic == 69 || CurrentMusic == 68)
			PlayMusic(255);
		break;
	case 1:
		Weather_start(p, 2, 125, 1, 13, 8, 1, 21, 7);
		PlayMusic(69);
		break;
	case 2:
		Weather_start(p, 2, 125, 6, 3, 4, 0, 0, 0);
		PlayMusic(68);
		break;
	case 3:
		Weather_start(p, 0, 125, 10, 0, 0, 1, 21, 7);
		break;
	case 5:
	case 6:
		Weather_start(p, 2, 1, 6, 0, 0, 0, 0, 0);
		break;
	}
}

WeatherSet::WeatherSet() { type = 0; }

extern "C" void ResetWeatherGlobals(void)
{
	memset((void *)&CurrentWeather, 0, sizeof(CurrentWeather));
	memset((void *)&WeatherFile, 0, sizeof(WeatherFile));
	memcpy(WeatherFileName, "WEATHER.DAT", sizeof(WeatherFileName));
}

extern "C" void ConstructWeatherGlobals(void)
{
	new (&CurrentWeather) WeatherSet();
	new (&WeatherFile) WeatherSaver();
}
