/* Black Gate U7.EXE, resident segment 1 (file offsets 0x00928c to 0x0092e5, 89 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
#include "plat.h"
#include "u7ibuf.h"
#include "init.h"
#include "main.h"

extern uint8_t CheatStart;
extern void SetDataDirectories(void);
extern void ConfigureSound(char *configuration, char *preferences);
extern void ParseCommandLine(int16_t argc, char **argv);
extern void InitGameSystems(void);

int16_t GameMain(int16_t argc, char **argv)
{
	FlatModeFlags = 0;
	SetDataDirectories();
	ConfigureSound("u7.cfg", ".\\gamedat\\options.cfg");
	ParseCommandLine(argc, argv);
	InitEnvironment();
	InitGameSystems();
	if (CheatStart != 0) {
		AvatarDontMove = 0;
	}
	MainGameLoop();
	ShutDown();
	plat_exit(5);
	return 5;
}
