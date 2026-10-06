#ifndef TYPEFRAM_H
#define TYPEFRAM_H

/* A type in the low ten bits, a frame in the next five, and a flip flag in the top bit. */
struct far TypeFrame {
	unsigned bits;
	TypeFrame() {}
	TypeFrame(unsigned n) { bits = n; }
	TypeFrame(unsigned type, unsigned frame) { bits = (type & 0x3ff) | ((frame << 10) & 0x7c00); }
	unsigned type() { return bits & 0x3ff; }
	unsigned frame() { return (bits & 0x7c00) >> 10; }
	unsigned char flipped() { return (bits & 0x8000) == 0x8000; }
	void setType(unsigned type) { bits = (type & 0x3ff) | (bits & 0xfc00); }
	/* Keeps the flip flag. */
	void setFrame(unsigned frame) { bits = (bits & 0x83ff) | ((frame << 10) & 0x7c00); }
	/* Takes the flip flag as frame bit 5. */
	void setFlipFrame(unsigned frame) { bits = (bits & 0x3ff) | ((frame << 10) & 0xfc00); }
};

#endif
