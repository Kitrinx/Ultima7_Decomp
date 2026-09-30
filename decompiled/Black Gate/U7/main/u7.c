/* Black Gate U7.EXE, resident segment 1 (file offsets 0x00928c to 0x0092e5, 89 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder chosen by subsystem.
 */

#include <stdlib.h>
#include "u7ibuf.h"
#include "init.h"
#include "main.h"

extern unsigned char CheatStart;
extern void SetDataDirectories(void);
extern void ConfigureSound(char *configuration, char *preferences);
extern void ParseCommandLine(int argc, char **argv);
extern void InitGameSystems(void);

void main(int argc, char **argv)
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
	exit(5);
}
