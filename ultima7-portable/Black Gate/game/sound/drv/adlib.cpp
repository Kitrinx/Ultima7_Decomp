/* AdLib sound effects are not supported yet; the game never selects the AdLib device. */

#include "u7port.h"
#include "adlib.h"

void ReclaimAdlibChannels(void)
{
}

int16_t GetAdlibVoiceState(int16_t)
{
	return 0;
}

void StopAdlibVoice(int16_t)
{
}

void TickAdlibSounds()
{
}

void YieldAdlibChannels()
{
}

int16_t StartAdlibSound(uint8_t *, int16_t)
{
	return 0;
}

void SetAdlibVolume(int16_t, int16_t)
{
}

void ResetAdlib(void)
{
}
