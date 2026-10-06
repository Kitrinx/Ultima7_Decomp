/* Serpent Isle SI.EXE, resident segment 28 (file offsets 0x018db3 to 0x018e32, 127 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "script.h"

struct ScriptBuffer {
	unsigned char length;
	unsigned char data[127];
};

ScriptBuffer ScriptPacket;

void AppendScriptByte(unsigned char *buf, unsigned char c)
{
	if ((int) *buf < 127) {
		buf[*buf] = c;
		(*buf)++;
	}
}

void AppendScriptWord(unsigned char *buf, int w)
{
	unsigned char *p = (unsigned char *) &w;

	AppendScriptByte(buf, p[0]);
	AppendScriptByte(buf, p[1]);
}

/* Packs the arguments, one byte each, up to the SCRIPT_END that ends the list. */
char *MakeScript(char first, ...)
{
	unsigned char n;
	unsigned char *dst;
	int *arg;

	n = 1;
	dst = ScriptPacket.data;
	for (arg = (int *) &first; (int) n < 127 && *arg != SCRIPT_END; arg++) {
		*dst++ = *arg;
		n++;
	}
	ScriptPacket.length = n;
	return (char *) &ScriptPacket;
}
