/* Serpent Isle INTRO.EXE, resident segment 15 (file offsets 0x00c5f3 to 0x00c895, 674 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- -d rebuilds it byte for byte as C++.
 */

#include <string.h>
#include "file.h"
#include "options.h"

extern "C" char *far NextToken(char *text, char *separators, char **cursor);

static char Separators[] = "\r\n= ";
static char MusicKey[] = "MUSIC";
static char SpeechKey[] = "SPEECH";
static char SfxKey[] = "SFX";
static char InterruptKey[] = "INTERRUPT";
static char PortKey[] = "PORT";
static char AdlibKey[] = "ADLIB";
static char RolandKey[] = "ROLAND";
static char OnValue[] = "ON";
static char OffValue[] = "OFF";

AudioOptions::AudioOptions()
{
	music = speech = sfx = 0;
}

unsigned char AudioOptions::load()
{
	return load("static\\options.cfg");
}

/* Lines read "KEY = ON" or "KEY = OFF"; a switch named without a value is on. */
unsigned char AudioOptions::load(char *name)
{
	music = speech = sfx = OPTION_ON;
	LineFile f;
	char line[80];

	if (!DosFileExists(name))
		return 0;
	if (f.open(name) != 1)
		return 0;

	char *save;
	char *key;
	int n;
	char *value;

	while (f.hasLine()) {
		n = f.readLine(line, 79);
		if (n == 0)
			break;
		line[n] = 0;
		key = NextToken(line, Separators, &save);
		if (stricmp(key, MusicKey) == 0) {
			music = OPTION_ON;
			while ((value = NextToken(0, Separators, &save)) != 0) {
				if (stricmp(value, OnValue) == 0)
					music = OPTION_ON;
				else if (stricmp(value, OffValue) == 0)
					music = OPTION_OFF;
			}
		}
		if (stricmp(key, SfxKey) == 0) {
			sfx = OPTION_ON;
			if ((value = NextToken(0, Separators, &save)) != 0) {
				if (stricmp(value, OnValue) == 0)
					sfx = OPTION_ON;
				else if (stricmp(value, OffValue) == 0)
					sfx = OPTION_OFF;
			}
		}
		if (stricmp(key, SpeechKey) == 0) {
			speech = OPTION_ON;
			while ((value = NextToken(0, Separators, &save)) != 0) {
				if (stricmp(value, OnValue) == 0)
					speech = OPTION_ON;
				else if (stricmp(value, OffValue) == 0)
					speech = OPTION_OFF;
			}
		}
	}
	return 1;
}

unsigned char AudioOptions::musicOn()
{
	return music == OPTION_ON;
}

unsigned char AudioOptions::speechOn()
{
	return speech == OPTION_ON;
}

unsigned char AudioOptions::sfxOn()
{
	return sfx == OPTION_ON;
}
