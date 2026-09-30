#include "u7port.h"
#include "plat.h"
#include "arena.h"
#include "dosio.h"

/* Only pointers into the arena have a linear address. */
uint32_t PointerToLinear(void *p)
{
	uintptr_t at = (uintptr_t) p, base = (uintptr_t) LinearBase;

	if (p == 0)
		return 0;
	if (at < base || at - base > LinearSize)
		plat_fatal("PointerToLinear: the pointer is outside linear memory.");
	return (uint32_t) (at - base);
}
