/* Black Gate U7.EXE, one routine in resident segment 0 (file offset 0x0081b5, 14 bytes).
 * Borland C++ 2.0 -mm -O rebuilds it byte for byte.
 * The runtime library's operator delete(void *), which hands the block to free.
 */

#include <alloc.h>

void DeallocateNearMemory(void *block)
{
	free(block);
}
