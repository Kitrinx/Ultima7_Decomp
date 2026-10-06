/* Serpent Isle MAINMENU.EXE, resident segment 21 (file offsets 0x00f8b4 to 0x00fca7, 1011 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include "chkfile.h"
#include "install.h"

InstallParams InstallInfo;

InstallOption::InstallOption()
{
	kind = enabled = 0;
	offset = size = 0;
	next = 0;
}

InstallParams::InstallParams()
{
	optionCount = 0;
	options = 0;
	setDefaults();
}

InstallParams::~InstallParams()
{
	clear();
}

void InstallParams::setDefaults()
{
	dosMemory = 530000L;
	speechMemory = 6144;
	adlibMemory = 31000;
	rolandMemory = 23000;
	extendedMemory = 1024000L;
	fullDiskSpace = 20480;
	diskSpace = 4096;
	unused = 0;
	clear();
}

void InstallParams::clear()
{
	InstallOption *o, *next;

	for (o = options; o; o = next) {
		next = o->next;
		delete o;
	}
	optionCount = 0;
	options = 0;
}

/* Reads install.prm; false if it is missing or short. */
int InstallParams::load(char *name)
{
	clear();
	DataFile file;
	InstallOption *o;
	int ok;
	long got;

	if (file.open(name, FILE_OPEN) != 1) {
		return 0;
	}
	ok = 0;
	got = file.read(this, sizeof(InstallParams));
	options = 0;
	if (got == sizeof(InstallParams)) {
		int n = optionCount;
		optionCount = 0;
		add(n);
		ok = 1;
		for (o = options; o; o = o->next) {
			got = file.read(o, OPTION_RECORD_SIZE);
			if (got != OPTION_RECORD_SIZE) {
				ok = 0;
				break;
			}
		}
	}
	file.close();
	return ok;
}

int InstallParams::save(char *)
{
	return 0;
}

void InstallParams::show()
{
}

InstallOption *InstallParams::option(int n)
{
	InstallOption *o;

	for (o = options; n--; ) {
		o = o->next;
		if (!o)
			break;
	}
	return o;
}

int InstallParams::append(InstallOption *option)
{
	InstallOption *node = new InstallOption;
	InstallOption *p;

	if (node == 0)
		return 0;
	optionCount++;
	*node = *option;
	for (p = options; p && p->next; p = p->next)
		;
	if (p)
		p->next = node;
	else
		options = node;
	node->next = 0;
	return 1;
}

/* Appends extra blank options; false if memory ran out first. */
int InstallParams::add(int extra)
{
	InstallOption *last, *o;

	for (last = options; last && last->next; last = last->next)
		;
	while (extra--) {
		o = new InstallOption;
		if (!o)
			break;
		optionCount++;
		if (last)
			last->next = o;
		else
			options = o;
		last = o;
		o->next = 0;
	}
	return extra == -1;
}

int InstallParams::count()
{
	return optionCount;
}

/* The highest offset any option starts at. */
long InstallParams::lastOffset()
{
	InstallOption *o;
	long last = 0;

	for (o = options; o; o = o->next)
		if (o->offset > last)
			last = o->offset;
	return last;
}

long InstallParams::endOffset()
{
	InstallOption *o;
	long base = fullDiskSpace + diskSpace;
	long most = base;
	long total = 0;
	long end;

	for (o = options; o; o = o->next) {
		end = o->offset + o->size + total;
		if (end > most)
			most = end;
		total += o->size;
	}
	return most;
}
