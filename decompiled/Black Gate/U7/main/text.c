/* Black Gate U7.EXE, overlay segment 263 (file offsets 0x07c360 to 0x07c4ed, 397 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -Y rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include <string.h>
#include "easyfile.h"
#include "flex.h"
#include "dcache.h"
#include "debug.h"
#include "u7ibuf.h"
#include "oops.h"
#include "text.h"

/* The game's text, read from text.flx. */
struct TextCache : DataCache {
	char *text(unsigned section, unsigned n);
	char *read(int n, int *len, char *dst);
};

static char *TextFileName = "text.flx";
static TextCache GameTextCache;
/* Shown for an entry out of range. */
static char *MissingText = "NA";
/* The first entry of each section. */
static unsigned TextSectionStart[7] = { 0, 1024, 1280, 1536, 1792, 2048, 2304 };

char *TextCache::text(unsigned section, unsigned n)
{
	char *s;

	if (section >= 0 && section <= 6) {
		/* section 6 reads its end from past the table */
		if (n < 0 || n >= TextSectionStart[section + 1] - TextSectionStart[section])
			s = MissingText;
		else
			s = StorePath(get(TextSectionStart[section] + n));
	} else
		s = MissingText;
	return s;
}

char *TextCache::read(int n, int *len, char *dst)
{
	Flex f;

	if (!f.open(BuildPath(StaticPath, TextFileName, 0)))
		ReportFileNotFound(BuildPath(StaticPath, TextFileName, 0));
	if (!f.readRecord(n, dst, 0))
		strcpy(dst, "");
	f.close();
	*len = strlen(dst) + 1;
	return dst;
}

char *GetGameText(unsigned char section, unsigned n)
{
	if (OinkMode)
		return "Oink";
	return GameTextCache.text(section, n);
}

void InitTextCache(void)
{
	GameTextCache.init(1024);
}
