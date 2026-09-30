/* The console keyboard for the helper programs, through U7's key handling. */

#include "u7port.h"
#include "u7event.h"
#include "keys.h"

namespace Shared {

int16_t KeyHit(void)
{
	return ::KeyPressed();
}

int16_t GetKey(void)
{
	return ::ReadKey();
}

}
