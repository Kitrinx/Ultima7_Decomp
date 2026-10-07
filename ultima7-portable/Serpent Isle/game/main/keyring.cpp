/* Serpent Isle SI.EXE, overlay segment 226 (file offsets 0x05f2d0 to 0x05f519, 585 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -Y rebuilds it byte for byte as C++.
 * Filename and folder inferred. Names and flags inferred; native unused local retained.
 */

#include "u7port.h"
#include <new>
#include "debug.h"
#include "keyring.h"

Keyring KeyRing;

void Keyring::dump()
{
	int16_t i;
	DebugPrintf("key array: ");
	for (i = 0; i < 32; i++)
		DebugPrintf("%d ", keys[i]);
	DebugPrintf("\n");
	DebugPrintf("keys: ");
	for (i = 0; i < 256; i++) {
		if (contains(i))
			DebugPrintf("%d ", i);
	}
	DebugPrintfWait("\n");
}

Keyring::Keyring()
{
	unusedFlag = 0;
	for (int16_t i = 0; i < 32; i++)
		keys[i] = 0;
}

void Keyring::add(uint8_t key)
{
	if (contains(key))
		return;
	int16_t byteIndex = key / 8;
	int16_t bitIndex = key - byteIndex * 8;
	int8_t mask = 1;
	mask = mask << bitIndex;
	char *slot = &keys[byteIndex];
	*slot = *slot ^ mask;
}

uint8_t Keyring::contains(uint8_t key)
{
	int16_t byteIndex = key / 8;
	int16_t bitIndex = key - byteIndex * 8;
	int8_t value = keys[byteIndex];
	int8_t found = 0;
	value = value << (7 - bitIndex);
	value = value >> 7;
	if (value != 0)
		return 1;
	else
		return 0;
}

uint8_t Keyring::acceptsKey(uint8_t frame, uint8_t quality)
{
	if ((frame == 21 || frame == 22) && quality == 0)
		return 1;
	return 0;
}

void Keyring::encode()
{
	for (int16_t i = 0; i <= 31; i++) {
		if (i % 2 == 0) {
			keys[i] = ~keys[i];
			keys[i] = keys[i] ^ 0xFD;
		} else {
			keys[i] = ~keys[i];
			keys[i] = keys[i] ^ 0xFE;
		}
	}
}

void Keyring::decode()
{
	for (int16_t i = 0; i <= 31; i++) {
		if (i % 2 == 0)
			keys[i] = keys[i] ^ 0xFD;
		else
			keys[i] = keys[i] ^ 0xFE;
		keys[i] = ~keys[i];
	}
}

extern "C" void ResetKeyringGlobals(void)
{
	memset((void *)&KeyRing, 0, sizeof(KeyRing));
}

extern "C" void ConstructKeyringGlobals(void)
{
	new (&KeyRing) Keyring();
}
