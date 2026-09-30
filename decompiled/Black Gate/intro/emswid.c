/* Black Gate INTRO.EXE, resident segment 84 (file offsets 0x01a3be to 0x01a3f9, 59 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include <dos.h>
#include "lowlevel.h"

/* The width of the widest frame of a shape that may live in expanded memory. */
int far pascal GetEmsMaxFrameWidth(void far *shape)
{
	void far *data = 0;

	data = MapEmsPointer(shape);
	if (FP_SEG(data))
		return GetMaxFrameWidth(data);
}
