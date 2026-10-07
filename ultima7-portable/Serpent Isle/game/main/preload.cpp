/* Serpent Isle SI.EXE, overlay segment 235 (file offsets 0x0663e0 to 0x066f02, 2850 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -Y -b- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
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
	uint8_t music, speech, effects;
};

struct SoundConfig {
	int8_t device;
	int16_t musicPort, speechPort, irq, dma;
	uint8_t speechEnabled, optionEnabled;
	SoundConfig();
	void readConfig(char *name);
	int16_t isRoland();
	int16_t isAdlib();
	int16_t isSoundBlaster();
	int16_t hasMusic();
};

extern uint16_t _ovrbuffer;
void RestorePolymorphs(void);
void CheckMoonshadeRavaged(void);

static char * FormatToFar(char *, char *, ...);

inline int8_t UpperCase(int8_t c)
{
	return c >= 'a' && c <= 'z' ? c + ('A' - 'a') : c;
}

struct WorldMask { WorldMask(); };

char *const U7MapFileName = "U7MAP";
char *const U7IregFileFormat = "U7IREG%02x";
char *const U7ChunksFileName = "U7CHUNKS";
char *const ShpDimsFileName = "shpdims.dat";
char OptionDelimiters[] = "\r\n= ";
char MusicKeyword[] = "MUSIC";
char SpeechKeyword[] = "SPEECH";
char SfxKeyword[] = "SFX";
char OnKeyword[] = "ON";
char OffKeyword[] = "OFF";
const char CheatPassword[] = "manimal";
uint8_t CheatStart = 0;
Speech SpeechPlayer(3072, 10667, 0x220, 7, 3);
SoundConfig SoundSetup;

extern "C" void InitGameSystems()
{
	SaveLoadActive = 1;
	SetDebugHook(ShowScreenCoords);
	LogMemoryUsage(GetGameText(3, 181));    /* Sets */
	InitNPCSets();
	LogMemoryUsage(GetGameText(3, 182));    /* Voice */
	SpeechPlayer.start();
	LogMemoryUsage(GetGameText(3, 183));    /* Flex zoomer */
	InitIfixCaches();
	LogMemoryUsage(GetGameText(3, 184));    /* World Physics */
	InitWorldPhysics();
	ReplaceString(&IregPathFormat, BuildPath(GamedatPath, U7IregFileFormat, 0));
	ReplaceString(&MapPath, BuildPath(StaticPath, U7MapFileName, 0));
	LogMemoryUsage(GetGameText(3, 185));    /* Sprite Manager */
	SpriteManager_init(&gSpriteManager, 8);
	LogMemoryUsage(GetGameText(3, 186));    /* Shape Manager */
	OpenShapeManager();
	if (!DebugOutputEnabled) {
		HideMarkerShapes();
	}
	LogMemoryUsage(GetGameText(3, 187));    /* The Red Screen */
	RedScreenPicture.show();
	LogMemoryUsage(GetGameText(3, 188));    /* Action Queue */
	ActionQueue.reset();
	LogMemoryUsage(GetGameText(3, 189));    /* Collision buf */
	AllocateCollisionBuffer();
	GameFlags.init();
	LogMemoryUsage(GetGameText(3, 190));    /* Map */
	InitMap();
	LogMemoryUsage(GetGameText(3, 191));    /* Z-Buffer */
	WorldMaskObject = new WorldMask;
	LogMemoryUsage(GetGameText(3, 192));    /* Shape Dimensions */
	LoadShpDims(BuildPath(StaticPath, ShpDimsFileName, 0));
	LogMemoryUsage(GetGameText(3, 193));    /* Occlusion */
	InitOcclusionTable();
	OcclusionTable.load(StaticPath);
	OpenChunkFile(BuildPath(StaticPath, U7ChunksFileName, 0));
	LogMemoryUsage(GetGameText(3, 194));    /* Chunk Cache */
	InitChunkCache();
	LogMemoryUsage(GetGameText(3, 195));    /* Weapon Tables */
	LoadItemDataFiles();
	LogMemoryUsage(GetGameText(3, 196));    /* Initializing SPOT... */
	InitSchedules();
	if (HasGameArgs()) {
		SaveGameFiles.deleteGameDirectory();
	}
	LogMemoryUsage(GetGameText(3, 197));    /* Game data */
	SaveGameFiles.restoreGame(-1);
	LogMemoryUsage(GetGameText(3, 198));    /* Initializing Camera */
	InitCamera(1662);
	LogMemoryUsage(GetGameText(3, 199));    /* Loading polymorphs */
	RestorePolymorphs();
	CheckMoonshadeRavaged();
	LogMemoryUsage(GetGameText(3, 200));    /* Initialization Complete */
	YellowTextPrinter.setFont(0);
	SaveLoadActive = 0;
}

extern "C" void ParseCommandLine(int16_t argc, char **argv)
{
	int16_t i;
	uint8_t launched = 0;
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
					ShapePoolSize = (int32_t)atoi(value) << 10;
					break;
				case 'B':
					PlainErrors = 1;
					AssertFail("Ovr=%uK", (uint16_t)(_ovrbuffer << 4) >> 10);
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
				case 'P':   /* passed by the SERPENT launcher */
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
		FatalError("Run SERPENT to play Serpent Isle.");
	}
}

uint8_t GetAudioOptions(uint8_t *music, uint8_t *speech, uint8_t *effects)
{
	AudioOptions options;
	options.music = options.speech = options.effects = 0;
	*music = 0;
	*speech = 0;
	*effects = 0;
	if (!(uint8_t)SoundSetup.hasMusic() && !SoundSetup.speechEnabled) {
		return 0;
	}
	*music = *speech = *effects = AUDIO_ON;
	if (!ReadAudioOptions(".\\gamedat\\options.cfg", &options)) {
		return 0;
	}
	*music = options.music;
	*speech = options.speech;
	*effects = options.effects;
	if (*music != 0)
		EnableMusic(*music == AUDIO_ON);
	if (*effects != 0)
		EnableSfx(*effects == AUDIO_ON);
	if (*speech != 0) {
		if (*speech == AUDIO_ON)
			SpeechOn = 1;
		else
			SpeechOn = 0;
	}
	return 1;
}

static char * FormatToFar(char *destination, char *format, ...)
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

uint8_t SetAudioState(uint8_t music, uint8_t speech, uint8_t effects)
{
	if (!(uint8_t)SoundSetup.hasMusic() && !SoundSetup.speechEnabled) {
		return 0;
	}
	DataFile options;
	if (options.open(".\\gamedat\\options.cfg", 0) != 1)
		return 0;
	char line[80];
	if (music != 0) {
		options.write(MusicKeyword, (uint32_t)strlen(MusicKeyword));
		char *state = music == AUDIO_ON ? OnKeyword : OffKeyword;
		FormatToFar(line, " %s\n\r", state);
		options.write(line, (uint32_t)strlen(line));
		EnableMusic(music == AUDIO_ON);
	}
	if (speech != 0) {
		options.write(SpeechKeyword, (uint32_t)strlen(SpeechKeyword));
		char *state = speech == AUDIO_ON ? OnKeyword : OffKeyword;
		FormatToFar(line, " %s\n\r", state);
		options.write(line, (uint32_t)strlen(line));
		if (speech == AUDIO_ON) {
			SpeechOn = 1;
		} else {
			SpeechOn = 0;
		}
	}
	if (effects != 0) {
		options.write(SfxKeyword, (uint32_t)strlen(SfxKeyword));
		char *state = effects == AUDIO_ON ? OnKeyword : OffKeyword;
		FormatToFar(line, " %s\n\r", state);
		options.write(line, (uint32_t)strlen(line));
		EnableSfx(effects == AUDIO_ON);
	}
	options.close();
	return 1;
}

extern "C" void ConfigureSound(char *configuration, char *preferences)
{
	SoundSetup.readConfig(configuration);
	MusicDevice = 0;
	if ((uint8_t)SoundSetup.isAdlib() != 0 || (uint8_t)SoundSetup.isSoundBlaster() != 0) {
		MusicDevice = 2;
	}
	if ((uint8_t)SoundSetup.isRoland() != 0) {
		MusicDevice = 1;
	}
	if (SoundSetup.speechEnabled) {
		SpeechPlayer.irq = SoundSetup.irq;
		SpeechPlayer.port = SoundSetup.speechPort;
		SpeechPlayer.dma = SoundSetup.dma;
		SpeechCardConfigured = 1;
		SpeechOn = 1;
		AlternateSpeechDriver = SoundSetup.optionEnabled;
	} else {
		SpeechOn = 0;
	}
	if ((uint8_t)SoundSetup.hasMusic() != 0) {
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

uint8_t ReadAudioOptions(char *filename, AudioOptions *settings)
{
	DataFile options;
	if (options.open(filename, 1) != 1)
		return 0;
	char *save;
	char *setting;
	int32_t length;
	char line[80];
	char *value;
	settings->music = AUDIO_ON;
	settings->speech = AUDIO_ON;
	settings->effects = AUDIO_ON;
	for (;;) {
		length = options.readUntil('\r', line, INT32_C(79));
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

extern "C" void ResetPreloadGlobals(void)
{
	memcpy(OptionDelimiters, "\r\n= ", sizeof(OptionDelimiters));
	memcpy(MusicKeyword, "MUSIC", sizeof(MusicKeyword));
	memcpy(SpeechKeyword, "SPEECH", sizeof(SpeechKeyword));
	memcpy(SfxKeyword, "SFX", sizeof(SfxKeyword));
	memcpy(OnKeyword, "ON", sizeof(OnKeyword));
	memcpy(OffKeyword, "OFF", sizeof(OffKeyword));
	CheatStart = 0;
	memset((void *)&SpeechPlayer, 0, sizeof(SpeechPlayer));
	memset((void *)&SoundSetup, 0, sizeof(SoundSetup));
}

extern "C" void ConstructPreloadGlobals(void)
{
	new (&SpeechPlayer) Speech(3072, 10667, 0x220, 7, 3);
	new (&SoundSetup) SoundConfig();
}
