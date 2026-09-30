/* Black Gate U7.EXE, resident segment 45 (file offsets 0x01eb7a to 0x01ebf9, 127 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "script.h"

struct ScriptBuffer {
	uint8_t length;
	uint8_t data[127];
};

ScriptBuffer ScriptPacket;

void AppendScriptByte(uint8_t *buf, uint8_t c)
{
	if ((int16_t) *buf < 127) {
		buf[*buf] = c;
		(*buf)++;
	}
}

void AppendScriptWord(uint8_t *buf, int16_t w)
{
	uint8_t *p = (uint8_t *) &w;

	AppendScriptByte(buf, p[0]);
	AppendScriptByte(buf, p[1]);
}

/* Packs the arguments, one byte each, up to the SCRIPT_END that ends the list. */
char *MakeScript(int32_t first, ...)
{
	uint8_t n;
	uint8_t *dst;
	int16_t arg;
	va_list args;

	n = 1;
	dst = ScriptPacket.data;
	va_start(args, first);
	for (arg = (int16_t) first; (int16_t) n < 127 && (uint16_t) arg != SCRIPT_END; arg = (int16_t) va_arg(args, int32_t)) {
		*dst++ = arg;
		n++;
	}
	va_end(args);
	ScriptPacket.length = n;
	return (char *) &ScriptPacket;
}

extern "C" void ResetScriptGlobals(void)
{
	memset(&ScriptPacket, 0, sizeof(ScriptPacket));
}
