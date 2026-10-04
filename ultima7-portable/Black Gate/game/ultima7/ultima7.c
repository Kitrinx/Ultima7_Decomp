/* Black Gate ULTIMA7.COM, the launcher. It runs the game's programs one after another, and
 * each program's exit code picks the next:
 *
 *   mainmenu m -> intro -> mainmenu v -> u7 -> endgame -> mainmenu c -> mainmenu n -> quit
 *
 * Each program starts afresh in this process, as each was its own EXE.
 */

#include "u7port.h"
#include "plat.h"
#include "programs.h"

enum {
	DO_QUIT = 1,
	DO_MENU_V = 2,
	DO_GAME = 3,
	DO_ENDGAME = 4,
	DO_MENU_N = 5,      /* last program: thank the player and stop */
	DO_INTRO = 6,
	DO_MENU_C = 7,
	DO_SURPRISE = 8,
	DO_MENU_L = 9,
	DO_MENU_M = 10
};

static const struct {
	const char *name;
	int16_t (*main)(int16_t argc, char **argv);
} Programs[] = {
	{"u7", GameMain},
	{"mainmenu", MainMenuMain},
	{"intro", IntroMain},
	{"endgame", EndgameMain},
};

/* given to intro and endgame */
static const char Password[] = "ereiamjh";

/* U7.EXE's command-line options, as switches. A value goes after '=' (or, when it is required,
 * as the next argument) and is appended to the option letter, as in "r3". A switch with no
 * option letter is a setting of this port and sets its flag instead. */
enum { NO_VALUE, OPTIONAL_VALUE, REQUIRED_VALUE };

/* Host alignment: the game's packing would leave these pointers unaligned. */
#pragma pack(push)
#pragma pack()
extern uint8_t QuietWeapons;

static const struct {
	const char *name;
	const char *option;
	int8_t value;
	uint8_t *flag;
} Switches[] = {
	{"--cheat", "ABCD\xff", NO_VALUE},     /* the password, typed with Alt-255 */
	{"--cheat-start", "s", NO_VALUE},
	{"--speech", "v", NO_VALUE},
	{"--adlib", "a", OPTIONAL_VALUE},
	{"--roland", "r", OPTIONAL_VALUE},
	{"--shape-pool", "c", REQUIRED_VALUE},
	{"--overlay-size", "b", NO_VALUE},
	{"--version", "?", NO_VALUE},
	{"--quiet-weapons", 0, NO_VALUE, &QuietWeapons},
};
#pragma pack(pop)

/* Turns the switches in argv into the options U7.EXE read; other arguments pass unchanged.
 * Returns the new count, or -1 after reporting a switch it doesn't know. */
static int16_t TranslateSwitches(int16_t argc, char **argv, char **out)
{
	int16_t count = 0;
	int16_t i, k;

	for (i = 0; i < argc; ++i) {
		const char *argument = argv[i];
		const char *value = 0;
		size_t length;

		if (i == 0 || strncmp(argument, "--", 2) != 0) {
			out[count++] = argv[i];
			continue;
		}
		length = strcspn(argument, "=");
		for (k = 0; k < (int16_t) (sizeof Switches / sizeof Switches[0]); ++k) {
			if (strlen(Switches[k].name) == length && strncmp(argument, Switches[k].name, length) == 0)
				break;
		}
		if (k == (int16_t) (sizeof Switches / sizeof Switches[0])) {
			plat_log("Unknown switch: ");
			plat_log(argument);
			plat_log("\nRun with --help to list them.\n");
			return -1;
		}
		if (argument[length] == '=')
			value = argument + length + 1;
		else if (Switches[k].value == REQUIRED_VALUE && i + 1 < argc)
			value = argv[++i];
		if (Switches[k].value == REQUIRED_VALUE && value == 0) {
			plat_log(Switches[k].name);
			plat_log(" needs a value.\n");
			return -1;
		}
		if (Switches[k].flag != 0) {
			*Switches[k].flag = 1;
		} else if (value == 0 || Switches[k].value == NO_VALUE) {
			out[count++] = (char *) Switches[k].option;
		} else {
			char *option = (char *) malloc(strlen(Switches[k].option) + strlen(value) + 1);

			strcpy(option, Switches[k].option);
			strcat(option, value);
			out[count++] = option;
		}
	}
	out[count] = 0;
	return count;
}

int16_t ProgramMain(const char *name, int16_t argc, char **argv)
{
	int16_t i;

	if (stricmp(name, "u7") == 0) {
		char **options = (char **) calloc(argc + 1, sizeof *options);

		argc = TranslateSwitches(argc, argv, options);
		if (argc < 0)
			plat_exit(1);
		argv = options;
	}
	for (i = 0; i < (int16_t) (sizeof Programs / sizeof Programs[0]); ++i) {
		if (stricmp(name, Programs[i].name) == 0)
			return Programs[i].main(argc, argv);
	}
	plat_fatal("An error had occurred loading a program.");
	return 1;
}

/* Runs a program with one argument. Returns its exit code, or -1 if it can't be run. */
static int16_t RunProgram(const char *name, const char *argument)
{
	int16_t i;

	for (i = 0; i < (int16_t) (sizeof Programs / sizeof Programs[0]); ++i) {
		if (stricmp(name, Programs[i].name) == 0)
			return plat_run_program(name, 1, (char **) &argument);
	}
	return -1;
}

int16_t Ultima7Main(int16_t argc, char **argv)
{
	int16_t next = DO_MENU_M;
	int16_t gameArgc = 0;
	char **gameArgs;
	int16_t i;

	/* the game gets our own arguments, with "p" added; bad switches stop us before it starts */
	gameArgs = (char **) calloc(argc + 1, sizeof *gameArgs);
	if (TranslateSwitches(argc, argv, gameArgs) < 0)
		return 1;
	for (i = 1; i < argc; ++i)
		gameArgs[gameArgc++] = argv[i];
	gameArgs[gameArgc++] = "p";

	for (;;) {
		switch (next) {
		case DO_QUIT:
			return 0;
		case DO_INTRO:
			next = RunProgram("intro", Password);
			break;
		case DO_MENU_V:
			next = RunProgram("mainmenu", "v");
			break;
		case DO_GAME:
			next = plat_run_program("u7", gameArgc, gameArgs);
			break;
		case DO_ENDGAME:
			next = RunProgram("endgame", Password);
			break;
		case DO_MENU_N:
			if (RunProgram("mainmenu", "n") < 0)
				next = -1;
			else
				next = 0;
			break;
		case DO_MENU_C:
			next = RunProgram("mainmenu", "c");
			break;
		case DO_SURPRISE:
			next = RunProgram("surprise", "u1");
			break;
		case DO_MENU_M:
			next = RunProgram("mainmenu", "m");
			break;
		case DO_MENU_L:
			next = RunProgram("mainmenu", "l");
			break;
		default:
			plat_log("Thank you for playing Ultima VII, The Black Gate!\n\n");
			return 0;
		}
		if (next < 0) {
			plat_log("An error had occurred loading a program.\n\n");
			return 0;
		}
	}
}
