/* Black Gate INTRO.EXE, resident segment 85 (file offsets 0x01a3f9 to 0x01a434, 59 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include <dos.h>
#include "lowlevel.h"

/* The height of the tallest frame of a shape that may live in expanded memory. */
int far pascal GetEmsMaxFrameHeight(void far *shape)
{
	void far *data = 0;

	data = MapEmsPointer(shape);
	if (FP_SEG(data))
		return GetMaxFrameHeight(data);
}
