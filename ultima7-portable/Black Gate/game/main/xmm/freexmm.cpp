/* The arena outlives the game, so there is nothing to give back. */

#include "u7port.h"
#include "freexmm.h"

void FreeXMM(void)
{
}

int16_t ShutdownXMM(void)
{
	FreeXMM();
	return 0;
}
