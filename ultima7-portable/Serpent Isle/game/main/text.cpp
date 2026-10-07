/* Serpent Isle SI.EXE, overlay segment 244 (file offsets 0x069ea0 to 0x06a031, 401 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
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
	char *text(uint16_t section, uint16_t n);
	char *read(int16_t n, int16_t *len, char *dst);
};

static char *const TextFileName = "text.flx";
static TextCache GameTextCache;
/* Shown for an entry out of range. */
static char *const MissingText = "NA";
/* The first entry of each section. The last is the word SI.EXE stored after the table, the "te"
 * of "text.flx". */
static const uint16_t TextSectionStart[9] = { 0, 1024, 1280, 1670, 1926, 2182, 2438, 2538, 0x6574 };

char *TextCache::text(uint16_t section, uint16_t n)
{
	char *s;

	if (section >= 0 && section <= 7) {
		/* Section 7 reads its end from past the table. */
		if (n < 0 || n >= (uint16_t)(TextSectionStart[section + 1] - TextSectionStart[section]))
			s = MissingText;
		else
			s = StorePath(get(TextSectionStart[section] + n));
	} else
		s = MissingText;
	return s;
}

char *TextCache::read(int16_t n, int16_t *len, char *dst)
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

char *GetGameText(uint8_t section, uint16_t n)
{
	if (OinkMode)
		return GameTextCache.text(3, 220);
	return GameTextCache.text(section, n);
}

void InitTextCache(void)
{
	GameTextCache.init(1024);
}

extern "C" void ResetTextGlobals(void)
{
	memset((void *)&GameTextCache, 0, sizeof(GameTextCache));
}

extern "C" void ConstructTextGlobals(void)
{
	new (&GameTextCache) TextCache();
}
