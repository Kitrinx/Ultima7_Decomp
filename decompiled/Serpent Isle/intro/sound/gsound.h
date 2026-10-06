#ifndef GSOUND_H
#define GSOUND_H

/* The sound setup from u7.cfg: a music device with its port, and a digital device. */
struct SoundConfig {
	char device;            /* 's' Sound Blaster, 'a' Adlib, 'r' Roland, 'p' none */
	int musicPort;
	int speechPort;
	int irq;
	int dma;
	char speechEnabled;
	char optionEnabled;
	SoundConfig();
	void reset();
	void readConfig(char *name);
	void close();
	unsigned char isRoland();
	unsigned char isAdlib();
	unsigned char isSoundBlaster();
	unsigned char hasMusic();
	char getSpeechEnabled() { return speechEnabled; }
	char getOptionEnabled() { return optionEnabled; }
	int getSpeechPort() { return speechPort; }
	int getIrq() { return irq; }
	int getDma() { return dma; }
};

int ParseHexSuffix(char *s);

extern SoundConfig Config;

#endif
