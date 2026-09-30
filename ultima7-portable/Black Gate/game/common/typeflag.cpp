/* Black Gate U7.EXE, resident segment 81 (file offset 0x02bd2b, 0 bytes): data but no code.
 * Borland C++ 2.0 -mm -O -G rebuilds its empty code segment as shipped.
 * Its data is DS:4A96-4AB6, between easyfile.c's and flxwrite.c's, as link order places segment
 * 81.
 */

#include "u7port.h"
/* what each item class can do, one word per class */
uint16_t ItemTypeClassFlags[16] = {
	0x0000, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x00fe, 0x040a,
	0x0003, 0x0406, 0x0002, 0x000a, 0x01ee, 0x03ee, 0x0000, 0x0000
};
