/* Black Gate MAINMENU.EXE, resident segment 20 (file offsets 0x00ee8b to 0x00ef30, 165 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <dos.h>

/* A DOS memory control block, one paragraph ahead of the memory it owns. */
struct MemoryBlock {
	unsigned char type;     /* MEMORY_BLOCK_MORE, or MEMORY_BLOCK_LAST at the end of the chain */
	unsigned      owner;
	unsigned      size;     /* in paragraphs */
};

#define MEMORY_BLOCK_MORE  'M'
#define MEMORY_BLOCK_LAST  'Z'

/* The first block of the chain, from the word ahead of DOS's list of lists. */
struct MemoryBlock far * far GetFirstMemoryBlock(void)
{
	asm mov ah, 52h
	asm int 21h
	asm mov bx, es:[bx-2]
	return (struct MemoryBlock far *) MK_FP(_BX, 0);
}

/* Bytes free for a program: the last owned block, taken to be our own, plus the free tail. */
long far GetFreeDosMemory(void)
{
	int done = 0;
	long bytes = 0;
	struct MemoryBlock far *block = GetFirstMemoryBlock();

	while (!done) {
		switch (block->type) {
		case MEMORY_BLOCK_MORE:
			bytes = (long) block->size << 4;
			block = (struct MemoryBlock far *) MK_FP(FP_SEG(block) + block->size + 1, 0);
			break;
		case MEMORY_BLOCK_LAST:
			bytes += (long) block->size << 4;
			done = 1;
			break;
		default:
			return -1;
		}
	}
	bytes += 32;
	return bytes;
}
