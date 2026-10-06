#ifndef OPTIONS_H
#define OPTIONS_H

/* Settings of the audio switches. */
#define OPTION_OFF      1
#define OPTION_ON       2

/* The music, speech and sound effect switches from options.cfg. */
struct AudioOptions {
	char music;
	char speech;
	char sfx;
	AudioOptions();
	unsigned char load();
	unsigned char load(char *name);
	unsigned char musicOn();
	unsigned char speechOn();
	unsigned char sfxOn();
};

extern AudioOptions Options;

#endif
