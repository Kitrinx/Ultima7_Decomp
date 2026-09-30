/* Black Gate ENDGAME.EXE, resident segment 35 (file offsets 0x00dad6 to 0x00dc77, 417 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include "memsys.h"
#include "ems.h"

#define LARGEST_EMS_BLOCK   0xfffcL
#define EMS_TOO_LARGE       0x305

unsigned char EmsMemory::initialized = 0;

void far *EmsMemory::allocate(long size, unsigned char)
{
	if (size > LARGEST_EMS_BLOCK)
		errorCode(EMS_TOO_LARGE);
	return AllocateEms(size);
}

void EmsMemory::release(void far **block)
{
	if (*block) {
		FreeEms(*block);
		*block = 0;
	}
}

void far *EmsMemory::lock(void far *block)
{
	return MapEmsPointer(block);
}

long EmsMemory::available()
{
	return EmsAvailable();
}

int EmsMemory::version()
{
	return EmsVersion;
}

unsigned char EmsMemory::isInitialized()
{
	return initialized;
}

void EmsMemory::shutdown()
{
	CloseEms();
}

EmsMemory::EmsMemory()
{
	if (initialized)
		error("Illegal re-initialization of EMS memory\n");
	else if (!OpenEms())
		error("Can't initialize EMS memory\n");
	initialized = 1;
}

EmsMemory::~EmsMemory()
{
	CloseEms();
}
