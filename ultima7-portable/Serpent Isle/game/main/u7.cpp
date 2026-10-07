/* Serpent Isle SI.EXE, resident segment 1 (file offsets 0x00994f to 0x0099bf, 112 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "plat.h"
#include "u7ibuf.h"
#include "init.h"
#include "main.h"
#include "preload.h"
#include "u7manage.h"

int16_t GameMain(int16_t argc, char **argv)
{
	FlatModeFlags = 0;
	SetDataDirectories();
	ConfigureSound("serpent.cfg", ".\\gamedat\\options.cfg");
	ParseCommandLine(argc, argv);
	InitEnvironment();
	InitGameSystems();
	if (CheatStart != 0) {
		AvatarDontMove = 0;
	}
	if (AvatarDontMove != 0) {
		RemapShapeRecord(721, 239);
	}
	MainGameLoop();
	ShutDown();
	plat_exit(5);
	return 5;
}
