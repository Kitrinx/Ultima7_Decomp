/* Serpent Isle SI.EXE, resident segment 53 (file offset 0x0218ea, 0 bytes): data but no code.
 * Borland C++ 2.0 -mm -O -G rebuilds its empty code segment as shipped.
 * Its data is DS:2CD4-2CF4, between easyfile.c's and flxwrite.c's, as link order places segment
 * 53.
 */

/* what each item class can do, one word per class */
unsigned ItemTypeClassFlags[16] = {
	0x0000, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x00fe, 0x040a,
	0x0003, 0x0406, 0x0002, 0x000a, 0x01ee, 0x03ee, 0x0000, 0x0000
};
