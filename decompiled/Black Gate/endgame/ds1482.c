/* Black Gate ENDGAME.EXE, one module of resident segment 68 (file offset 0x011d30, 0 bytes): data but no code.
 * Borland C++ 2.0 -mm -O -1 -P rebuilds its empty code segment as shipped.
 * Its data is DS:1482-167E, between fillrect.asm's and outline.asm's: an identity table and a table of
 * steps. Which module owned it is not known. File name inferred from its address.
 */

/* no code reads these; the same layout sits at DS:169A */
#define ROW(n)  n, n + 1, n + 2, n + 3, n + 4, n + 5, n + 6, n + 7, \
	n + 8, n + 9, n + 10, n + 11, n + 12, n + 13, n + 14, n + 15

static unsigned char lead[34] = { 0 };
static unsigned char identity[256] = {
	ROW(0x00), ROW(0x10), ROW(0x20), ROW(0x30), ROW(0x40), ROW(0x50), ROW(0x60), ROW(0x70),
	ROW(0x80), ROW(0x90), ROW(0xa0), ROW(0xb0), ROW(0xc0), ROW(0xd0), ROW(0xe0), ROW(0xf0)
};
static unsigned char gap[66] = { 0 };
static unsigned long steps[31] = {
	0x3, 0x6, 0xc, 0x14, 0x30, 0x60, 0xb8, 0x110,
	0x240, 0x500, 0xca0, 0x1b00, 0x3500, 0x6000, 0xb400, 0x12000,
	0x20400, 0x72000, 0x90000, 0x140000, 0x300000, 0x420000, 0xd80000, 0x1200000,
	0x3880000, 0x7200000, 0x9000000, 0x14000000, 0x32800000, 0x48000000, 0xa3000000UL
};
static unsigned char tail[28] = { 0 };
