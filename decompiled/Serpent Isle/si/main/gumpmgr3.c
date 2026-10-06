/* Serpent Isle SI.EXE, overlay segment 328 (file offsets 0x094c30 to 0x094ede, 686 bytes).
 * Borland C++ 2.0 -mm -O -P -Y rebuilds it byte for byte as C++.
 */

#include <dos.h>
#include <mem.h>
#include "u7manage.h"
#include "item.h"
#include "npcref.h"
#include "coord.h"
#include "colbuf.h"
#include "gumps.h"
#include "spellbk.h"
#include "dosio.h"
#include "easyfile.h"
#include "gumpmgr.h"
#include "worldgmp.h"
#include "itable.h"
#include "datanode.h"
#include "camera.h"
#include "cheat.h"
#include "oops.h"
#include "mouse.h"
#include "bogus.h"
#include "combat.h"
#include "crime.h"
#include "palctrl.h"
#include "u7point.h"
#include "use.h"
#include "u7npc.h"
#include "u7event.h"
#include "debug.h"
#include "cast.h"
#include "itemcmd.h"
#include "itemovr1.h"
#include "look.h"
#include "text.h"
#include "palfade.h"
#include "partymov.h"
#include "sortitem.h"
#include "sounds.h"
#include "spell.h"
#include "trigger.h"
#include "usehook.h"
#include "voice.h"
#include "type.h"
#include "preload.h"
#include "systimer.h"
#include "iteminfo.h"
#include "collide.h"
#include "u7map.h"
#include "slider.h"

/* the save, load and options dialog */
struct SaveGump : Control {
	SaveGump();
	~SaveGump();
	void moveTo(int, int);
	unsigned char handle(MouseState *);
	void draw(View *);
};

/* a dialog that asks for a number */
struct SliderGump : Control {
	ProportionalTextPrinter text;
	SizedSprite accept;
	RepeatButton down, up;
	int thumbShape, backgroundShape, endShape;
	int minimum, maximum, step;
	int left, right, x, y, value;
	SliderGump(int, int, int, int, int, int);
	void moveTo(int, int);
	void draw(View *);
	unsigned char handle(MouseState *);
};

struct GumpFile : DataNode {
	char *name();
	void load(char *dir);
	void save(char *dir);
};

extern View Viewport;

char *GumpMgrFileName = "GUMPMGR.DAT";
GumpFile GumpMgrFile;

char *GumpFile::name()
{
	return GumpMgrFileName;
}

void GumpFile::save(char *path)
{
	int fd;
	int i;

	fd = CreateFileOrFail(BuildPath(path, GumpMgrFileName, 0));
	for (i = 0; i < 8; i++)
		DosWrite(fd, -1L, (long) sizeof(Point), PaperdollPositions[i]);
	DosClose(fd);
}

void GumpFile::load(char *path)
{
	int fd;
	int i;

	fd = DosOpen(BuildPath(path, GumpMgrFileName, 0));
	if (fd != -1)
		for (i = 0; i < 8; i++)
			DosRead(fd, -1L, (long) sizeof(Point), PaperdollPositions[i]);
	DosClose(fd);
}

char RunSaveDialog(unsigned char *quit)
{
	MouseState state;
	unsigned char handled = 0;
	unsigned char result = 0;

	OpenSaveDialog = new SaveGump;
	if (!OpenSaveDialog) {
		ReportOutOfNearMemory();
		return 0;
	}
	SetFixedPalette(0);
	RedrawDialogs();
	while (!handled) {
		UpdateAndCopyMouseState(&state);
		result = OpenSaveDialog->handle(&state);
		handled = 1;
		if (result == GUMP_RESTORED)
			DialogState = DIALOG_DONE;
		else if (result == GUMP_QUIT) {
			DialogState = DIALOG_DONE;
			*quit = 1;
		} else if (result != GUMP_CLOSE)
			handled = 0;
	}
	delete OpenSaveDialog;
	OpenSaveDialog = 0;
	return 1;
}

int RunSliderGump(int low, int high, int increment, int initial, int x, int y)
{
	unsigned char result = 0;
	int value;
	MouseState state;
	SliderGump box(low, high, increment, initial, x, y);

	box.show();
	box.paint(&Viewport);
	SetFixedPalette(0);
	CopyFrameBuffer();
	GameInput.enableKeyboardMouse();
	while (result != GUMP_CLOSE) {
		UpdateAndCopyMouseState(&state);
		if (state.clicked()) {
			result = box.handle(&state);
			if (result == GUMP_REDRAW) {
				box.paint(&Viewport);
				CopyFrameBuffer();
			}
		}
		CyclePalette();
	}
	value = box.value;
	RedrawDialogs();
	return value;
}
