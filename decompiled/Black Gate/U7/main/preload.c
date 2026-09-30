/* Black Gate U7.EXE, overlay segment 251 (file offsets 0x076c60 to 0x07762b, 2507 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -Y -b- rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dosio.h"
#include "easyfile.h"
#include "chkfile.h"
#include "vstring.h"
#include "voice.h"
#include "debug.h"
#include "midiplay.h"
#include "u7manage.h"
#include "u7sound.h"
#include "sprite.h"
#include "redscrn.h"
#include "savegame.h"
#include "actqueue.h"
#include "chunk.h"
#include "collide.h"
#include "cullmask.h"
#include "initwp.h"
#include "itemcmd.h"
#include "loadreg.h"
#include "sysusage.h"
#include "vitem.h"
#include "voolook.h"
#include "colbuf.h"
#include "sche_ov1.h"
#include "itable.h"
#include "init.h"
#include "camera.h"
#include "dbgfont.h"
#include "cheat.h"
#include "flags.h"
#include "preload.h"
#undef atoi

struct AudioOptions {
	unsigned char music, speech, effects;
};

struct SoundConfig {
	char device;
	int musicPort, speechPort, irq, dma;
	unsigned char speechEnabled;
	SoundConfig();
	void readConfig(char *name);
	int isRoland();
	int isAdlib();
	int isSoundBlaster();
	int hasMusic();
};

extern unsigned _ovrbuffer;

static char far *far FormatToFar(char far *, char *, ...);

inline char UpperCase(char c)
{
	return c >= 'a' && c <= 'z' ? c + ('A' - 'a') : c;
}

struct WorldMask { WorldMask(); };

int CursorX = 0, CursorY = 0;
unsigned char CursorDrawn = 0, CursorTracking = 0;
char *U7MapFileName = "U7MAP";
char *U7IregFileFormat = "U7IREG%02x";
char *U7ChunksFileName = "U7CHUNKS";
char *ShpDimsFileName = "shpdims.dat";
char OptionDelimiters[] = "\r\n= ";
char MusicKeyword[] = "MUSIC";
char SpeechKeyword[] = "SPEECH";
char SfxKeyword[] = "SFX";
char InterruptKeyword[] = "INTERRUPT";
char PortKeyword[] = "PORT";
char AdlibKeyword[] = "ADLIB";
char RolandKeyword[] = "ROLAND";
char OnKeyword[] = "ON";
char OffKeyword[] = "OFF";
char CheatPassword[] = "ABCD\xff";
unsigned char CheatStart = 0;
Speech SpeechPlayer(3072, 10667, 0x220, 7, 3);
SoundConfig SoundSetup;

extern "C" void far InitGameSystems()
{
	SaveLoadActive = 1;
	SetDebugHook(ShowScreenCoords);
	LogMemoryUsage("Sets");
	InitNPCSets();
	LogMemoryUsage("Voice");
	SpeechPlayer.start();
	LogMemoryUsage("Flex zoomer");
	InitIfixCaches();
	LogMemoryUsage("World Physics");
	InitWorldPhysics();
	ReplaceString(&IregPathFormat, BuildPath(GamedatPath, U7IregFileFormat, 0));
	ReplaceString(&MapPath, BuildPath(StaticPath, U7MapFileName, 0));
	LogMemoryUsage("Sprite Manager");
	SpriteManager_init(&gSpriteManager, 8);
	LogMemoryUsage("Shape Manager");
	OpenShapeManager();
	if (!DebugOutputEnabled) {
		HideMarkerShapes();
	}
	LogMemoryUsage("The Red Screen");
	RedScreenPicture.show();
	LogMemoryUsage("Action Queue");
	ActionQueue.reset();
	LogMemoryUsage("Collision buf");
	AllocateCollisionBuffer();
	GameFlags.init();
	LogMemoryUsage("Map");
	InitMap();
	LogMemoryUsage("Z-Buffer");
	WorldMaskObject = new WorldMask;
	LogMemoryUsage("Shape Dimensions");
	LoadShpDims(BuildPath(StaticPath, ShpDimsFileName, 0));
	LogMemoryUsage("Occlusion");
	InitOcclusionTable();
	OcclusionTable.load(StaticPath);
	OpenChunkFile(BuildPath(StaticPath, U7ChunksFileName, 0));
	LogMemoryUsage("Chunk Cache");
	InitChunkCache();
	LogMemoryUsage("Weapon Tables");
	LoadItemDataFiles();
	if (HasGameArgs() != 0) {
		SaveGameFiles.deleteGameDirectory();
	}
	LogMemoryUsage("Game data");
	SaveGameFiles.restoreGame(-1);
	LogMemoryUsage("Initializing SPOT...");
	InitSchedules();
	LogMemoryUsage("Initializing Camera");
	InitCamera(1598);
	LogMemoryUsage("Initialization Complete");
	YellowTextPrinter.setFont(0);
	SaveLoadActive = 0;
}

extern "C" void far ParseCommandLine(int argc, char **argv)
{
	int i;
	unsigned char launched = 0;
	CheatsEnabled = 0;
	for (i = 1; i < argc; ++i) {
		char *argument = argv[i];
		if (stricmp(argument, CheatPassword) == 0) {
			CheatsEnabled = 1;
		} else {
			/* a run of option letters; a number is read from the second character */
			char *value = argument + 1;
			while (*argument) {
				switch (UpperCase(*argument)) {
				case '?':
					PlainErrors = 1;
					FatalError("[%s]\nVersion %s", GameTitle, GameVersion);
					break;
				case 'V':
					SpeechOn = 1;
					break;
				case 'C':
					ShapePoolSize = (long)atoi(value) << 10;
					break;
				case 'B':
					PlainErrors = 1;
					AssertFail("Ovr=%uK", (_ovrbuffer << 4) >> 10);
					break;
				case 'A':
					MusicDevice = 2;
					if (*value >= '0' && *value <= '9') {
						AdlibPort = atoi(value);
					}
					break;
				case 'R':
					MusicDevice = 1;
					if (*value >= '0' && *value <= '9') {
						RolandArgument = atoi(value);
					}
					break;
				case 'P':   /* passed by the ULTIMA7 launcher */
				case 'p':
					launched = 1;
					break;
				case 'S':
				case 's':
					if (CheatsEnabled) {
						CheatStart = 1;
						launched = 1;
						HackMoverEnabled = 1;
						PowerAvatar = 1;
						DebugOutputEnabled = 1;
					}
					break;
				}
				++argument;
			}
		}
	}
	if (!launched) {
		PlainErrors = 1;
		FatalError("Run ULTIMA7 to play Ultima VII.");
	}
}

unsigned char far GetAudioOptions(unsigned char *music, unsigned char *speech, unsigned char *effects)
{
	AudioOptions options;
	options.music = options.speech = options.effects = 0;
	*music = 0;
	*speech = 0;
	*effects = 0;
	if (!(unsigned char)SoundSetup.hasMusic() && !SoundSetup.speechEnabled) {
		return 0;
	}
	*music = *speech = *effects = AUDIO_ON;
	if (!ReadAudioOptions(".\\gamedat\\options.cfg", &options)) {
		return 0;
	}
	*music = options.music;
	*speech = options.speech;
	*effects = options.effects;
	return 1;
}

static char far *far FormatToFar(char far *destination, char *format, ...)
{
	va_list arguments;
	if (format != WorkString) {
		va_start(arguments, format);
		vsprintf(WorkString, format, arguments);
		va_end(arguments);
	}
	_fstrcpy(destination, WorkString);
	return destination;
}

unsigned char far SetAudioState(unsigned char music, unsigned char speech, unsigned char effects)
{
	if (!(unsigned char)SoundSetup.hasMusic() && !SoundSetup.speechEnabled) {
		return 0;
	}
	DataFile options;
	if (options.open(".\\gamedat\\options.cfg", 0) != 1)
		return 0;
	char line[80];
	if (music != 0) {
		options.write(MusicKeyword, (unsigned long)strlen(MusicKeyword));
		char *state = music == AUDIO_ON ? OnKeyword : OffKeyword;
		FormatToFar(line, " %s\n\r", state);
		options.write(line, (unsigned long)strlen(line));
		EnableMusic(music == AUDIO_ON);
	}
	if (speech != 0) {
		options.write(SpeechKeyword, (unsigned long)strlen(SpeechKeyword));
		char *state = speech == AUDIO_ON ? OnKeyword : OffKeyword;
		FormatToFar(line, " %s\n\r", state);
		options.write(line, (unsigned long)strlen(line));
		if (speech == AUDIO_ON) {
			SpeechOn = 1;
		} else {
			SpeechOn = 0;
		}
	}
	if (effects != 0) {
		options.write(SfxKeyword, (unsigned long)strlen(SfxKeyword));
		char *state = effects == AUDIO_ON ? OnKeyword : OffKeyword;
		FormatToFar(line, " %s\n\r", state);
		options.write(line, (unsigned long)strlen(line));
		EnableSfx(effects == AUDIO_ON);
	}
	options.close();
	return 1;
}

extern "C" void far ConfigureSound(char *configuration, char *preferences)
{
	SoundSetup.readConfig(configuration);
	MusicDevice = 0;
	if ((unsigned char)SoundSetup.isAdlib() != 0 || (unsigned char)SoundSetup.isSoundBlaster() != 0) {
		MusicDevice = 2;
	}
	if ((unsigned char)SoundSetup.isRoland() != 0) {
		MusicDevice = 1;
	}
	if (SoundSetup.speechEnabled) {
		SpeechPlayer.irq = SoundSetup.irq;
		SpeechPlayer.port = SoundSetup.speechPort;
		SpeechPlayer.dma = SoundSetup.dma;
		SpeechCardConfigured = 1;
		SpeechOn = 1;
	} else {
		SpeechOn = 0;
	}
	if ((unsigned char)SoundSetup.hasMusic() != 0) {
		EnableMusic(1);
		EnableSfx(1);
	}
	AudioOptions options;
	options.music = options.speech = options.effects = 0;
	if (ReadAudioOptions(preferences, &options) != 0) {
		if (options.music == AUDIO_OFF) {
			EnableMusic(0);
		}
		if (options.effects == AUDIO_OFF) {
			EnableSfx(0);
		}
		if (options.speech == AUDIO_ON && SoundSetup.speechEnabled) {
			SpeechOn = 1;
		} else {
			SpeechOn = 0;
		}
	}
}

unsigned char far ReadAudioOptions(char *filename, AudioOptions *settings)
{
	DataFile options;
	if (options.open(filename, 1) != 1)
		return 0;
	char *save;
	char *setting;
	long length;
	char line[80];
	char *value;
	settings->music = AUDIO_ON;
	settings->speech = AUDIO_ON;
	settings->effects = AUDIO_ON;
	for (;;) {
		length = options.readUntil('\r', line, 79L);
		if (length == 0)
			break;
		line[length] = 0;
		setting = NextToken(line, OptionDelimiters, &save);
		if (stricmp(setting, MusicKeyword) == 0) {
			settings->music = AUDIO_ON;
			while ((value = NextToken(0, OptionDelimiters, &save)) != 0) {
				if (stricmp(value, OnKeyword) == 0) {
					settings->music = AUDIO_ON;
				} else if (stricmp(value, OffKeyword) == 0) {
					settings->music = AUDIO_OFF;
				}
			}
		}
		if (stricmp(setting, SfxKeyword) == 0) {
			settings->effects = AUDIO_ON;
			if ((value = NextToken(0, OptionDelimiters, &save)) != 0) {
				if (stricmp(value, OnKeyword) == 0) {
					settings->effects = AUDIO_ON;
				} else if (stricmp(value, OffKeyword) == 0) {
					settings->effects = AUDIO_OFF;
				}
			}
		}
		if (stricmp(setting, SpeechKeyword) == 0) {
			settings->speech = AUDIO_ON;
			while ((value = NextToken(0, OptionDelimiters, &save)) != 0) {
				if (stricmp(value, OnKeyword) == 0) {
					settings->speech = AUDIO_ON;
				} else if (stricmp(value, OffKeyword) == 0) {
					settings->speech = AUDIO_OFF;
				}
			}
		}
	}
	options.close();
	return 1;
}
